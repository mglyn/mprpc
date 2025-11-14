#include <iostream>
#include "mprpcApplication.h"
#include "user.pb.h"

int main(int argc, char** argv){
    MprpcApplication::Init(argc, argv);
     
    proto::UserServiceRpc_Stub stub(new MprpcChannel());

    proto::LoginRequest request;
    request.set_name("zhangsan");
    request.set_password("123456");
    proto::LoginResponse response;

    MprpcController controller;

    stub.Login(&controller, &request, &response, nullptr);    

    if(controller.Failed()){
        std::cout << "rpc login failed! error:" << controller.ErrorText() << std::endl;
    }
    else{
        if(response.result().errcode() == 0){
            std::cout << "rpc login response success: " << response.success() <<std::endl;
        } else {
            std::cout << "rpc login response failed" << response.result().errmsg() << std::endl;
        }
    }
}