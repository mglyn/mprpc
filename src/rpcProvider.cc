#include "rpcProvider.h"
#include <string>
#include "mprpcApplication.h"
#include <google/protobuf/descriptor.h>
#include "rpcHeader.pb.h"
#include <iostream>
#include "zookeeperutil.h"

void RpcProvider::NotifyService(::google::protobuf::Service* service){
    // 你原来的代码，完全不变
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

    m_serviceMap[service_name] = serviceInfo;
}

void RpcProvider::Run(){
    // 你原来的代码，完全不变
    std::string ip = MprpcApplication::GetConfig().Load("rpcserverip");
    uint32_t port = std::stoi(MprpcApplication::GetConfig().Load("rpcserverport"));
    muduo::net::InetAddress addr(ip, port);

    muduo::net::TcpServer server(&m_eventLoop, addr, "RpcProviderServer");

    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this ,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    server.setThreadNum(4);

    ZkClient zkCli;
    zkCli.Start();
    for(auto &sp:m_serviceMap)
    {
        std::string service_path ="/"+sp.first;
        zkCli.Create(service_path.c_str(),nullptr,0);
        for(auto &mp:sp.second.m_methodMap)
        {
            std::string method_path=service_path+"/"+mp.first;
            char method_path_data[128]={0};
            sprintf(method_path_data,"%s:%d",ip.c_str(),port);

            zkCli.Create(method_path.c_str(),method_path_data,strlen(method_path_data),ZOO_EPHEMERAL);
        }
    }

    server.start();
    m_eventLoop.loop();
}

void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn){
    // 你原来的代码，完全不变
    if(!conn->connected()){
        m_connectionBufferMap.erase(conn);
        conn->shutdown();
    }
    else{
        conn->setTcpNoDelay(true);
    }
}

void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn,
                muduo::net::Buffer* buffer,
                muduo::Timestamp time) {
    // 1. 数据存入连接专属缓存
    std::string& conn_buf = m_connectionBufferMap[conn];
    conn_buf.append(buffer->peek(), buffer->readableBytes());
    buffer->retrieveAll();

    // 2. 循环解析完整包（header_size → header → args）
    while (true) {
        // 第一步：判断是否有足够的 header_size（4字节）
        if (conn_buf.size() < 4) {
            break; // 不够4字节，等后续数据
        }

        // 第二步：解析 header_size
        uint32_t header_size = 0;
        memcpy(&header_size, conn_buf.data(), 4);

        // 安全校验（针对 header_size，避免非法值）
        if (header_size > 1024 * 1024) { // 限制 header 最大1MB
            std::cout << "Invalid header size: " << header_size << ", close connection" << std::endl;
            conn->shutdown();
            m_connectionBufferMap.erase(conn);
            break;
        }

        // 第三步：判断是否有足够的 header 内容（4字节 + header_size 字节）
        if (conn_buf.size() < 4 + header_size) {
            break; // header 不完整，等后续数据
        }

        // 第四步：解析 header，获取 args_size
        std::string rpc_header_str = conn_buf.substr(4, header_size);
        mprpc::RpcHeader rpcHeader;
        if (!rpcHeader.ParseFromString(rpc_header_str)) {
            std::cout << "rpcHeader parse error! Close connection" << std::endl;
            conn->shutdown();
            m_connectionBufferMap.erase(conn);
            break;
        }
        uint32_t args_size = rpcHeader.args_size();

        // 第五步：判断是否有足够的完整数据（4+header_size+args_size）
        uint32_t total_needed_len = 4 + header_size + args_size;
        if (conn_buf.size() < total_needed_len) {
            break; // 完整包数据不够，等后续
        }

        // 第六步：截取完整包，从缓存中剥离（避免重复处理）
        std::string complete_pkg = conn_buf.substr(0, total_needed_len);
        conn_buf.erase(0, total_needed_len);

        // 第七步：调用你原来的解析逻辑（完全不变）
        ParseCompleteRpcPackage(complete_pkg, conn);
    }
}

// 你原来的 ParseCompleteRpcPackage 函数，完全不变！
void RpcProvider::ParseCompleteRpcPackage(const std::string& complete_pkg, const muduo::net::TcpConnectionPtr& conn) {
    uint32_t header_size = 0;
    complete_pkg.copy((char*)&header_size, 4, 0);

    std::string rpc_header_str = complete_pkg.substr(4, header_size);
    mprpc::RpcHeader rpcHeader;
    if (!rpcHeader.ParseFromString(rpc_header_str)) {
        std::cout << "rpcHeader parse error!" << std::endl;
        return;
    }

    std::string service_name = rpcHeader.service_name();
    std::string method_name = rpcHeader.method_name();
    uint32_t args_size = rpcHeader.args_size();
    std::string args_str = complete_pkg.substr(4 + header_size, args_size);

    std::cout << "Received RPC call:" << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_size: " << args_size << std::endl;

    auto it = m_serviceMap.find(service_name);
    if (it == m_serviceMap.end()) {
        std::cout << "Service not found: " << service_name << std::endl;
        return;
    }
    auto mit = it->second.m_methodMap.find(method_name);
    if (mit == it->second.m_methodMap.end()) {
        std::cout << "Method not found: " << method_name << std::endl;
        return;
    }

    const google::protobuf::MethodDescriptor* pMethodDesc = mit->second;
    google::protobuf::Service* service = it->second.m_service;

    google::protobuf::Message* request = service->GetRequestPrototype(pMethodDesc).New();
    if (!request->ParseFromString(args_str)) {
        std::cout << "request parse error! : " << args_str << std::endl;
        delete request;
        return;
    }

    google::protobuf::Message* response = service->GetResponsePrototype(pMethodDesc).New();

    auto SendRpcResponse = [](const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message* response) {
        std::string response_str;
        if (!response->SerializeToString(&response_str)) {
            std::cout << "response serialize error!" << std::endl;
        } else {
            conn->send(response_str);
            std::cout << "sent response: " << response_str << std::endl;
        }
        conn->shutdown();
        delete response;
    };

    google::protobuf::Closure* done = google::protobuf::NewCallback
    <const muduo::net::TcpConnectionPtr&, google::protobuf::Message*>
    (SendRpcResponse, conn, response);

    service->CallMethod(pMethodDesc, nullptr, request, response, done);

    delete request;
}