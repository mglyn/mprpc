#include <iostream>
#include <string>
#include "user.pb.h"
#include "mprpcApplication.h"
#include "rpcProvider.h"
#include "logger.h"


class UserService: public proto::UserServiceRpc{
public:
    bool Login(const std::string& username, const std::string& password){
        std::cout << "Local Login called with username: " << username 
                  << " and password: " << password << std::endl;
        return true;
    }

    void Login(::google::protobuf::RpcController* controller,
                       const ::proto::LoginRequest* request,
                       ::proto::LoginResponse* response,
                       ::google::protobuf::Closure* done) override {
        std::cout << "RPC Login called with username: " << request->name() << " and password: " << request->password() << std::endl;
        
        bool login_result = Login(request->name(), request->password());
        
        proto::ResultCode* code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_result);

        done->Run();
    }

private:
};

int main(int argc, char** argv) {

    LOG_INFO("UserService Start");

    MprpcApplication::Init(argc, argv);

    RpcProvider provider;
    provider.NotifyService(new UserService());

    provider.Run();

    return 0;
}