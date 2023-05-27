#include <iostream>
#include <thread>
#include <chrono>

#include "logger/log.h"

void printDebug(){
    
    for(int i = 0; i < 5; ++i){
        LOG_DEBUG("Port:%d, OpenLinger: %s", 3306, "true");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void printInfo(){
    for(int i = 0; i < 5; ++i){
        LOG_INFO("Port:%d, OpenLinger: %s", 3306, "true");
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
}

int main(){

    Log::getInstance()->init("./log", ".log", 0);

    std::thread t1(printDebug);
    std::thread t2(printInfo);
    t1.join();
    t2.join();

    LOG_INFO("Port:%d, OpenLinger: %s", 3306, "true");

    return 0;
}