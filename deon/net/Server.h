#ifndef NET_SERVER_H
#define NET_SERVER_H

#include "EventLoop.h"
#include "Channel.h"
#include "EventLoopThreadPool.h"

class Server
{
    public:
         Server(EventLoop *loop, int threadNum, int port, std::string root);
         ~Server() = default;
         EventLoop* getLoop() const { return loop_; }
         void start();
    
    private:
        void newConnection();
        void registerConnection();
        EventLoop *loop_;
        int threadNum_;
        int port_;
        std::string root_;
        std::shared_ptr<EventLoopThreadPool> threadPool_;
        bool started_;
        int listenFd_;
        std::shared_ptr<Channel> acceptChannel_;
};

#endif

