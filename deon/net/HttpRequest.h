#ifndef NET_HTTPREQUEST_H
#define NET_HTTPREQUEST_H

#include <memory>
#include <unistd.h>
#include <map>

class EventLoop;
class TimerNode;
class Channel;
enum Method
{
    M_INVALID, M_POST, M_GET, M_HEAD
};
enum Version
{
    UNKNOWN, HTTP_10, HTTP_11
};


class HttpRequest
{
    public:
        HttpRequest()
            : method_(M_INVALID),
              version_(UNKNOWN)
        {

        }

        void requestInit()
        {
            method_ = M_INVALID;
            version_ = UNKNOWN;
            uri_.clear();
            headers_.clear();
        }
        
        void setMethod(Method method)
        {
            method_ = method;
        }
         
        void setVersion(Version version)
        {
            version_ = version;
        }
        
        void setUri(std::string uri)
        {
            uri_ = uri;
        }
        
        void setHeader(std::string key, std::string value)
        {
            headers_[key] = value;
        }

        Method getMethod()
        {
            return method_;
        }

        Version getVersion()
        {
            return version_;
        }

        std::string getUri()
        {
            return uri_;
        }

        std::map<std::string, std::string> getHeaders()
        {
            return headers_;
        }
    private:
        Method method_;
        Version version_;
        std::string uri_;
        std::map<std::string, std::string> headers_;
        int fd_;
        std::weak_ptr<TimerNode> timer_;
};

#endif
