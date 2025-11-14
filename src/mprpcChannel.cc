#include "mprpcChannel.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "rpcHeader.pb.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include "mprpcApplication.h"
#include <arpa/inet.h>
#include <unistd.h>
#include "mprpcController.h"
#include "zookeeperutil.h"

//header_size(4 bytes) + service_name + method_name + args
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                              google::protobuf::RpcController* controller,
                              const google::protobuf::Message* request,
                              google::protobuf::Message* response,
                              google::protobuf::Closure* done) {

    const google::protobuf::ServiceDescriptor* sd = method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();   

    std::string args_str;
    if(!request->SerializeToString(&args_str)){
        controller->SetFailed("request serialize error!");
        return;
    }

    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_str.size());
    
    std::string rpc_header_str;
    if(!rpcHeader.SerializeToString(&rpc_header_str)){
        controller->SetFailed("rpcHeader serialize error!");
        return;
    }

    uint32_t header_size = rpc_header_str.size();

    std::string send_str;
    send_str.insert(0, std::string((char*)&header_size, 4));
    send_str += rpc_header_str;
    send_str += args_str;

    std::cout << "send_str: " << send_str << std::endl;

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd < 0){
        controller->SetFailed("create socket error! errno:" + std::to_string(errno));
        return;
    }

    struct sockaddr_in server_addr;
    //std::string ip = MprpcApplication::GetConfig().Load("rpcserverip");
    //uint32_t port = std::stoi(MprpcApplication::GetConfig().Load("rpcserverport"));

    /*
    rpc调用方向调用service_name服务，需要查询zk上该服务所在的host信息
    */
    ZkClient zkCli;
    zkCli.Start();
    std::string method_path="/"+service_name+"/"+method_name;
    //获取ip地址和端口号
    std::string host_data=zkCli.GetData(method_path.c_str());
    if(host_data=="")
    {
        controller->SetFailed(method_path+" is not exist!");
        return;
    }
    int idx=host_data.find(":");//分割符
    if(idx==-1)
    {
        controller->SetFailed(method_path+" address is invalid!");
        return;
    }
    std::string ip=host_data.substr(0,idx);
    uint32_t port=atoi(host_data.substr(idx+1,host_data.size()-idx).c_str());

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if(connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        controller->SetFailed("connect error! errno:" + std::to_string(errno));;
        close(clientfd);
        return;
    }

    int send_size = send(clientfd, send_str.c_str(), send_str.size(), 0);
    if(send_size != send_str.size()){   
        controller->SetFailed("send error!");
        close(clientfd);
        return;
    }

    char buf[2048] = {0};
    uint32_t recv_size = 0;
    if((recv_size = recv(clientfd, buf, 2048, 0)) < 0){
        controller->SetFailed("recv error! errno:" + std::to_string(errno));
        close(clientfd);
        return;
    }

    if(!response->ParseFromArray(buf, recv_size)){
        controller->SetFailed("response parse error! : " + std::string(buf));
        close(clientfd);
        return;
    }
    close(clientfd);
}