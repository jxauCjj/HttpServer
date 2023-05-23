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

int main(){

    Log::getInstance()->init("./log", ".log");

    std::thread t1(printDebug);
    t1.detach();

    LOG_INFO("Port:%d, OpenLinger: %s", 3306, "true");

    // std::this_thread::sleep_for(std::chrono::seconds(6));

    return 0;
}