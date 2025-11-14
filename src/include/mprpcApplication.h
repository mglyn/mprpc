#pragma once

#include "mprpcConfig.h"
#include "mprpcChannel.h"
#include "mprpcController.h"

class MprpcApplication{
public:
    static void Init(int argc, char** argv);
    static MprpcApplication& GetInstance();
    static MprpcConfig& GetConfig(){
        return m_config;
    }

private:

    static MprpcConfig m_config;

    MprpcApplication();
    MprpcApplication(const MprpcApplication&) = delete;
    MprpcApplication& operator=(const MprpcApplication&) = delete;
};