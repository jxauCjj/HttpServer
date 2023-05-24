CXX = g++
CFLAGS = -std=c++17 -O2 -Wall -g
TARGET = server

${TARGET}: server.cpp ./bin/log.o
	$(CXX) ${CFLAGS} $^ -o ./bin/$@

# 日志模块
./bin/log.o: ./logger/log.cpp ./logger/log.h ./logger/blockqueue.hpp
	$(CXX) ${CFLAGS} -c $< -o $@