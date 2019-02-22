#include "../base/Logging.h"
#include "Server.h"
#include "Util.h"
#include <functional>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

Server::Server(EventLoop *loop, int threadNum, int port, std::string root)
    :   loop_(loop),
        threadNum_(threadNum),
        port_(port),
        root_(root),
        threadPool_(new EventLoopThreadPool(loop_, threadNum)),
        started_(false),
        listenFd_(socket_bind_listen(port_)),
        acceptChannel_(new Channel(loop_, listenFd_))
{
    handle_for_sigpipe();
    if(set_socket_non_blocking(listenFd_) < 0)
    {
        LOG << "set listen_fd  non blocking error";
        abort();
    }
}

void Server::start()
{
    threadPool_->start();
    acceptChannel_->setEvents(EPOLLIN | EPOLLET);
    acceptChannel_->setReadCallback(std::bind(&Server::newConnection, this));
    acceptChannel_->setRegisterCallback(std::bind(&Server::registerConnection, this));
    loop_->addChannel(acceptChannel_);
    started_ = true;
}

void Server::newConnection()
{
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);
    int accept_fd = 0;
    while((accept_fd = accept(listenFd_, (struct sockaddr*)&client_addr, &client_addr_len)) > 0)
    {
        EventLoop *ioLoop = threadPool_->getNextLoop();
        LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port)
            << " fd:" << accept_fd;        
        if (set_socket_non_blocking(accept_fd) < 0)
        {
            LOG << "Set accept_fd non block failed!";
            return;
        }

        set_socket_no_delay(accept_fd);
        std::shared_ptr<Http> httpData(new Http(ioLoop, accept_fd, root_));
        httpData->getChannel()->tie(httpData);
        ioLoop->runInLoop(std::bind(&Http::registerHttp, httpData));
    }
}

void Server::registerConnection()
{
    acceptChannel_->setEvents(EPOLLIN | EPOLLET);
    loop_->updateChannel(acceptChannel_);
}
