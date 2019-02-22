#ifndef LOG_FILEUTIL_H
#define LOG_FILEUTIL_H

#include "noncopyable.h"
#include <string>

class FileUtil : noncopyable
{
    public:
        explicit FileUtil(std::string filename);
        ~FileUtil();
        // append 会向文件写
        void append(const char *logline, const size_t len);
        void flush();
    private:
        size_t write(const char *logline, size_t len);
        FILE* fp_;
        char buffer_[64*1024];
};

#endif
