#pragma once

#include <google/protobuf/service.h>
#include <muduo/net/EventLoop.h> 
#include <muduo/net/TcpServer.h> 
#include <muduo/net/InetAddress.h> 
#include <map>
#include <string>

class RpcProvider{
public:
    void NotifyService(::google::protobuf::Service* service);
    void Run();
private:
    muduo::net::EventLoop m_eventLoop;

    void OnConnection(const muduo::net::TcpConnectionPtr& conn);
    void OnMessage(const muduo::net::TcpConnectionPtr& conn,
                   muduo::net::Buffer* buffer,
                   muduo::Timestamp time);
    void ParseCompleteRpcPackage(const std::string& complete_pkg, const muduo::net::TcpConnectionPtr& conn);
    
    struct ServiceInfo{
        ::google::protobuf::Service* m_service; // 服务对象
        std::map<std::string, const ::google::protobuf::MethodDescriptor*> m_methodMap; // 存储服务方法
    };

    std::map<std::string, ServiceInfo> m_serviceMap; // 存储注册的服务
    std::map<muduo::net::TcpConnectionPtr, std::string> m_connectionBufferMap; // 存储每个连接的缓冲区
};
