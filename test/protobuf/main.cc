#include <iostream>
#include <string>
#include "test.pb.h"

int main(){

    test::GetFriendLsitResponse response;
    test::User *user1 = response.add_friend_list();
    
}

int main1(){

    test::LoginRequest req;
    req.set_name("user1");
    req.set_password("password123");

    
    if(std::string output; req.SerializeToString(&output)){
        std::cout << "Serialized LoginRequest: " << output << std::endl;
    } else {
        std::cerr << "Failed to serialize LoginRequest." << std::endl;
    }

    test::LoginRequest req2;
    if(req2.ParseFromString(req.SerializeAsString())){
        std::cout << "Deserialized LoginRequest - Name: " << req2.name()
                  << ", Password: " << req2.password() << std::endl;
    } else {
        std::cerr << "Failed to deserialize LoginRequest." << std::endl;
    }

}