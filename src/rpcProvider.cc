#include "rpcProvider.h"
#include <string>
#include "mprpcApplication.h"
#include <google/protobuf/descriptor.h>
#include "rpcHeader.pb.h"
#include <iostream>
#include "zookeeperutil.h"

void RpcProvider::NotifyService(::google::protobuf::Service* service){
    // 服务对象的描述信息
    const google::protobuf::ServiceDescriptor* pServiceDesc = service->GetDescriptor();
    const std::string service_name = pServiceDesc->name();
    int methodCnt = pServiceDesc->method_count();

    ServiceInfo serviceInfo;
    serviceInfo.m_service = service;

    std::cout << "Registering service: " << service_name << std::endl;
    for(int i = 0; i < methodCnt; ++i){
        const google::protobuf::MethodDescriptor* pMethodDesc = pServiceDesc->method(i);
        std::string method_name = pMethodDesc->name();
        std::cout << "  Registering method: " << method_name << std::endl;
        serviceInfo.m_methodMap[method_name] = pMethodDesc;
    }

    m_serviceMap[service_name] = serviceInfo; // 插入服务
}

void RpcProvider::Run(){
    std::string ip = MprpcApplication::GetConfig().Load("rpcserverip");
    uint32_t port = std::stoi(MprpcApplication::GetConfig().Load("rpcserverport"));
    muduo::net::InetAddress addr(ip, port);

    muduo::net::TcpServer server(&m_eventLoop, addr, "RpcProviderServer");

    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this ,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)); //消息回调函数留空，后续补充

    server.setThreadNum(4);

    //把当前rpc节点上要发布的服务全部注册在zk上，让rpc client可以从zk上发现服务
    //session的timeout默认为30s，zkclient的网络I/O线程1/3的timeout内不发送心跳则丢弃此节点
    ZkClient zkCli;
    zkCli.Start();//链接zkserver
    for(auto &sp:m_serviceMap)
    {
        //service_name
        std::string service_path ="/"+sp.first;//拼接路径
        zkCli.Create(service_path.c_str(),nullptr,0);//创建临时性节点
        for(auto &mp:sp.second.m_methodMap)
        {
            //service_name/method_name
            std::string method_path=service_path+"/"+mp.first;//拼接服务器路径和方法路径
            char method_path_data[128]={0};
            sprintf(method_path_data,"%s:%d",ip.c_str(),port);//向data中写入路径

            //创建节点,ZOO_EPHEMERAL表示临时节点
            zkCli.Create(method_path.c_str(),method_path_data,strlen(method_path_data),ZOO_EPHEMERAL);
        }
    }

    server.start();
    m_eventLoop.loop();
}

void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn){
    if(!conn->connected()){
        conn->shutdown();
    }
}

// header_size(4) + header_str + args_str
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn,
    muduo::net::Buffer* buffer,
    muduo::Timestamp time){
    std::string recv_buf = buffer->retrieveAllAsString();
    
    uint32_t header_size = 0;
    recv_buf.copy((char*)&header_size, 4, 0);

    std::string rpc_header_str = recv_buf.substr(4, header_size);
    mprpc::RpcHeader rpcHeader;
    if(!rpcHeader.ParseFromString(rpc_header_str)){
        std::cout << "rpcHeader parse error!" << std::endl; 
        return; 
    }

    std::string service_name = rpcHeader.service_name();
    std::string method_name = rpcHeader.method_name();
    uint32_t args_size = rpcHeader.args_size();
    
    std::string args_str = recv_buf.substr(4 + header_size, args_size);

    std::cout << "Received RPC call:" << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_size: " << args_size << std::endl;

    auto it = m_serviceMap.find(service_name);
    if(it == m_serviceMap.end()){
        std::cout << "Service not found: " << service_name << std::endl;
        return;
    }

    auto mit = it->second.m_methodMap.find(method_name);
    if(mit == it->second.m_methodMap.end()){
        std::cout << "Method not found: " << method_name << std::endl;
        return;
    }
    const google::protobuf::MethodDescriptor* pMethodDesc = mit->second;
    google::protobuf::Service* service = it->second.m_service;

    google::protobuf::Message* request = service->GetRequestPrototype(pMethodDesc).New();
    if(!request->ParseFromString(args_str)){
        std::cout << "request parse error! : " << args_str << std::endl;
        return;
    }

    google::protobuf::Message* response = service->GetResponsePrototype(pMethodDesc).New();

    auto SendRpcResponse = [](const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message* response){
        std::string response_str;
        if(!response->SerializeToString(&response_str)){
            std::cout << "response serialize error!" << std::endl;  
        }
        else{
            conn->send(response_str);
            std::cout << "sent : " << response_str << std::endl;
        }
        conn->shutdown(); //短连接服务，发送完响应后关闭连接
    };

    google::protobuf::Closure* done = google::protobuf::NewCallback<const muduo::net::TcpConnectionPtr&, google::protobuf::Message*>
        (SendRpcResponse, conn, response);

    service->CallMethod(pMethodDesc, nullptr, request, response, done);

    delete request;
    delete response;
}