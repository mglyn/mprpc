#pragma once

#include "lockQueue.h"

#include <iostream>

enum LogLevel {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL
};

class Logger {
public: 

    Logger(const Logger&) = delete; 
    Logger(const Logger&&) = delete; 
    Logger& operator= (const Logger&) = delete; 
    Logger& operator= (const Logger&&) = delete;

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    void log(LogLevel level, const std::string& msg) {

        std::string levelStr;

        switch (level) {
            case LOG_DEBUG:
                levelStr = "DEBUG";
                break;
            case LOG_INFO:
                levelStr = "INFO";
                break;
            case LOG_WARNING:
                levelStr = "WARNING";
                break;
            case LOG_ERROR:
                levelStr = "ERROR";
                break;
            case LOG_FATAL:
                levelStr = "FATAL";
                break;
            default:
                break;
        }

        std::string logMessage = levelStr + ": " + msg;
        logQueue.push(logMessage);
    }

private:

    Logger(){
        std::thread([this]() {
            while (true) {

                time_t now = time(0);
                struct tm tstruct;
                char fileNameStr[128];
                tstruct = *localtime(&now);
                snprintf(fileNameStr, sizeof(fileNameStr), "%d-%d-%d-log.txt", tstruct.tm_year + 1900, tstruct.tm_mon + 1, tstruct.tm_mday);

                char raw_file_name[256];
                snprintf(raw_file_name, sizeof(raw_file_name), "[%s] ", fileNameStr);

                FILE* fp = fopen(raw_file_name, "a+");
                if(fp == nullptr) {
                    std::cerr << "Failed to open log file: " << raw_file_name << std::endl;
                    break;
                }

                char timeStr_[128];
                snprintf(timeStr_, sizeof(timeStr_), "%d-%d-%d %02d:%02d:%02d", 
                    tstruct.tm_year + 1900, tstruct.tm_mon + 1, tstruct.tm_mday, tstruct.tm_hour, tstruct.tm_min, tstruct.tm_sec);
                
                std::string logMessage = logQueue.pop();
                std::string full = std::string(timeStr_) + " " + logMessage;
                fprintf(fp, "%s\n", full.c_str());
                fclose(fp);

            }   
        }).detach();
    }

    LockQueue<std::string> logQueue; // 异步IO消息队列
};

#define LOG_DEBUG(logmsgformat, ...) \
    do{ \
        Logger& logger = Logger::getInstance(); \
        char logmsg[1024] = {0}; \
        snprintf(logmsg, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.log(LOG_DEBUG, logmsg); \
    }while(0);

#define LOG_INFO(logmsgformat, ...) \
    do{ \
        Logger& logger = Logger::getInstance(); \
        char logmsg[1024] = {0}; \
        snprintf(logmsg, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.log(LOG_INFO, logmsg); \
    }while(0);

#define LOG_WARNING(logmsgformat, ...) \
    do{ \
        Logger& logger = Logger::getInstance(); \
        char logmsg[1024] = {0}; \
        snprintf(logmsg, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.log(LOG_WARNING, logmsg); \
    }while(0);

#define LOG_ERROR(logmsgformat, ...) \
    do{ \
        Logger& logger = Logger::getInstance(); \
        char logmsg[1024] = {0}; \
        snprintf(logmsg, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.log(LOG_ERROR, logmsg); \
    }while(0);    

#define LOG_FATAL(logmsgformat, ...) \
    do{ \
        Logger& logger = Logger::getInstance(); \
        char logmsg[1024] = {0}; \
        snprintf(logmsg, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.log(LOG_FATAL, logmsg); \
    }while(0);