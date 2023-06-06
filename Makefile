CXX = g++
CFLAGS = -std=c++17 -O2 -Wall -g
TARGET = server

${TARGET}: main.cpp ./bin/server.o ./bin/epoller.o ./bin/httpconn.o ./bin/httprequest.o ./bin/httpresponse.o ./bin/heaptimer.o \
					./bin/sqlconnpool.o ./bin/log.o ./bin/buffer.o -lpthread -lmysqlclient
	$(CXX) ${CFLAGS} $^ -o ./bin/$@

# webserver 模块
./bin/server.o: ./server/webserver.cpp ./server/webserver.h ./pool/threadpool.hpp
	$(CXX) ${CFLAGS} -c $< -o $@

# httpConn 模块
./bin/httpconn.o: ./http/httpconn.cpp ./http/httpconn.h
	$(CXX) ${CFLAGS} -c $< -o $@

# httprequest 模块
./bin/httprequest.o: ./http/httprequest.cpp ./http/httprequest.h
	$(CXX) ${CFLAGS} -c $< -o $@

# httpresponse 模块
./bin/httpresponse.o: ./http/httpresponse.cpp ./http/httpresponse.h
	$(CXX) ${CFLAGS} -c $< -o $@

./bin/heaptimer.o: ./timer/heaptimer.cpp ./timer/heaptimer.h
	$(CXX) ${CFLAGS} -c $< -o $@

# sqlConnPool
./bin/sqlconnpool.o: ./pool/sqlconnpool.cpp ./pool/sqlconnpool.h
	$(CXX) ${CFLAGS} -c $< -o $@

# epoller 模块
./bin/epoller.o: ./server/epoller.cpp ./server/epoller.h
	$(CXX) ${CFLAGS} -c $< -o $@

# 日志模块
./bin/log.o: ./logger/log.cpp ./logger/log.h ./logger/blockqueue.hpp
	$(CXX) ${CFLAGS} -c $< -o $@

# 缓冲区模块
./bin/buffer.o: ./buffer/buffer.cpp ./buffer/buffer.h
	$(CXX) ${CFLAGS} -c $< -o $@

clean:
	rm -r ./bin/*.o