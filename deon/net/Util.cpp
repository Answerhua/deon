#include "../base/Logging.h"
#include "Util.h"
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

const int MAX_BUFF = 4096;
const int LISTENQ = 2048;

int socket_bind_listen(int port)
{
    /*端口不在设定范围内*/
    if(port < 0 || port > 65535)
        return -1;

    /*创建socket*/
    int listen_fd = 0;
    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return -1;

    /*消除bind时"Address already in use"错误*/
    int optval = 1;
    if(setsockopt(listen_fd, SOL_SOCKET,  SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        return -1;

    /*设置ip并绑定端口*/
    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)port);
    if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        return -1;

    if (listen(listen_fd, LISTENQ) < 0)
        return -1;

    /*无效监听描述符*/
    if(listen_fd == -1)
    {
        close(listen_fd);
        return -1;
    }

    return listen_fd;

}

/*处理sigpipe信号*/
void handle_for_sigpipe()
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, NULL))
        return;
}

/*将文件描述符设置为非阻塞*/
int set_socket_non_blocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag == -1)
        return -1;
    flag |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flag) == -1)
        return -1;
    return 0;
}

/*启用nodelay,禁用nagle算法*/
void set_socket_no_delay(int fd)
{
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));
}

/*在指定fd中读取n个字符*/
ssize_t readn(int fd, void *buffer, ssize_t n)
{
    ssize_t nleft = n;
    ssize_t nread = 0;
    ssize_t readSum = 0;
    char *ptr = (char*) buffer;
    while(nleft > 0)
    {
        if((nread = read(fd, ptr, nleft)) < 0)
    {
        if(errno == EINTR)
            nread = 0;
        else if(errno == EAGAIN)
        {
        return readSum;
        }
        else
        {
        return -1;
        }               
    }
    else if(nread == 0)
        break;
    readSum += nread;
    nleft -= nread;
    ptr += nread;   
    }
    return readSum;
    
}

ssize_t readn(int fd, std::string &inBuffer, bool &zero)
{
    ssize_t nread = 0;
    ssize_t readSum = 0;
    for(;;)
    {
        char buff[MAX_BUFF];
        if ((nread = read(fd, buff, MAX_BUFF)) < 0)
        {
            if(errno == EINTR)
                continue;
            else if (errno == EAGAIN)
            {
                return readSum;
            }
            else
            {
                return -1;
            }
        }
        else if(nread == 0)
        {
            zero = true;
            break;
        }
        readSum += nread;
        inBuffer += std::string(buff, buff + nread);
    }   
    return readSum;
}

ssize_t writen(int fd, void *buff, size_t n)
{
    ssize_t nleft = n;
    ssize_t nwrite = 0;
    ssize_t writeSum = 0;
    char *ptr = (char*)buff;
    while(nleft > 0)
    {
        if((nwrite = write(fd, ptr, nleft)) <= 0)
        {
            if(nwrite < 0)
            {
                if(errno == EINTR)
                {
                    nwrite = 0;
                    continue;
                }
            }
        }
        writeSum += nwrite;
        nleft -= nwrite;
        ptr += nwrite;
    }
    return writeSum;
}

ssize_t writen(int fd, std::string &sbuff)
{
    size_t nremain = sbuff.size();
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;
    const char *ptr = sbuff.c_str();
    while (nremain > 0)
    {
        if ((nwritten = write(fd, ptr, nremain)) <= 0)
        {
            if (nwritten < 0)
            {
                if (errno == EINTR)
                {
                    nwritten = 0;
                    continue;
                }
                else if (errno == EAGAIN)
                    break;
                else
                    return -1;
            }
        }
        writeSum += nwritten;
        nremain -= nwritten;
        ptr += nwritten;
    }
    if (writeSum == static_cast<int>(sbuff.size()))
        sbuff.clear();
    else
        sbuff = sbuff.substr(writeSum);
    return writeSum;
}

ssize_t writenwc(int fd, std::wstring& sbuff)
{
    size_t nremain = sbuff.size();
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;
    const wchar_t *ptr = sbuff.c_str();
    while (nremain > 0)
    {
        if ((nwritten = write(fd, ptr, nremain)) <= 0)
        {
            if (nwritten < 0)
            {
                if (errno == EINTR)
                {
                    nwritten = 0;
                    continue;
                }
                else if (errno == EAGAIN)
                    break;
                else
                    return -1;
            }
        }
        writeSum += nwritten;
        nremain -= nwritten;
        ptr += nwritten;
    }
    if (writeSum == static_cast<int>(sbuff.size()))
        sbuff.clear();
    else
        sbuff = sbuff.substr(writeSum);
    return writeSum;  
}

std::wstring s2ws(const std::string& s)
{
    setlocale(LC_ALL, "chs");
    const char* _Source = s.c_str();
    size_t _Dsize = s.size() + 1;
    wchar_t *_Dest = new wchar_t[_Dsize];
    wmemset(_Dest, 0, _Dsize);
    mbstowcs(_Dest,_Source,_Dsize);
    std::wstring result = _Dest;
    delete []_Dest;
    setlocale(LC_ALL, "C");
    return result;
}

std::string ws2s(const std::wstring& ws)
{
    std::string curLocale = setlocale(LC_ALL, NULL);        // curLocale = "C";
    setlocale(LC_ALL, "chs");
    const wchar_t* _Source = ws.c_str();
    size_t _Dsize = 2 * ws.size() + 1;
    char *_Dest = new char[_Dsize];
    memset(_Dest,0,_Dsize);
    wcstombs(_Dest,_Source,_Dsize);
    std::string result = _Dest;
    delete []_Dest;
    setlocale(LC_ALL, curLocale.c_str());
    return result;
}

