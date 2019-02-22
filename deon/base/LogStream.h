#ifndef LOG_LOGSTREAM_H
#define LOG_LOGSTREAM_H

#include "noncopyable.h"
#include <string>
#include <string.h>
const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

template <int SIZE>
class FixedBuffer : noncopyable
{
    public:
        FixedBuffer()
            :  cur_(data_)
        {
        }

        ~FixedBuffer()
        {
        }

        void append(const char* buf, size_t len)
        {
            if(avail() > static_cast<int>(len)) //可用空间大于所需空间
            {
                memcpy(cur_, buf, len);
                cur_ += len;
            }
        }

        const char* data() const { return data_; }  //返回data_
        int length() const { return static_cast<int>(cur_ - data_); }

        char* current() { return cur_; }    //返回当前指针
        int avail() const { return static_cast<int>(end() - cur_); }    //返回data_的剩余可用长度
        void add(size_t len) { cur_ += len; }   

        void reset() { cur_ = data_; }  //重制data_
        void bzero() { memset(data_, 0, sizeof data_); }    //将data_清0

    private:
        const char* end() const { return data_ + sizeof data_; }    //返回指向data_尾部的指针
        char data_[SIZE];
        char* cur_;
};

class LogStream : noncopyable
{
    public:
        typedef FixedBuffer<kSmallBuffer> Buffer;

        //重载<<
        LogStream& operator<<(bool v)
        {
            buffer_.append(v ? "1" : "0", 1);
            return *this;
        }

        LogStream& operator<<(short);
        LogStream& operator<<(unsigned short);
        LogStream& operator<<(int);
        LogStream& operator<<(unsigned int);
        LogStream& operator<<(long);
        LogStream& operator<<(unsigned long);
        LogStream& operator<<(long long);
        LogStream& operator<<(unsigned long long);

//      LogStream& operator<<(const void*);
        
        LogStream& operator<<(float v)
        {
            *this << static_cast<double>(v);
            return *this;
        }   
        LogStream& operator<<(double);
        LogStream& operator<<(long double);

        LogStream& operator<<(char v)
        {
            buffer_.append(&v, 1);
            return *this;
        }

        LogStream& operator<<(const char* str)
        {
            if(str)
                buffer_.append(str, strlen(str));
            else
                buffer_.append("(null)", 6);
            return *this;
        }

        LogStream& operator<<(const unsigned char* str)
        {
            return operator<<(reinterpret_cast<const char*>(str));
        }   

        LogStream& operator<<(const std::string& v)
        {
            buffer_.append(v.c_str(), v.size());
            return *this;
        }

        void append(const char* data, int len) { buffer_.append(data, len); }   //向buffer_中添加数据
        const Buffer& buffer() const { return buffer_; }    //返回buffer_
        void resetBuffer() { buffer_.reset(); } //重制buffer_

    private:
        void staticCheck();

        template<typename T>
            void formatInteger(T);

        Buffer buffer_;

        static const int kMaxNumericSize = 32;
};

#endif
