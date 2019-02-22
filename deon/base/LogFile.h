#ifndef LOG_LOGFILE_H
#define LOG_LOGFILE_H

#include "FileUtil.h"
#include "MutexLock.h"
#include <string>
#include <memory>

class LogFile : noncopyable
{
    public:
        LogFile(const std::string& basename, int flushEveryN = 1024);
        ~LogFile();

        void append(const char* logline, int len);
        void flush();
        bool rollFile();
    
    private:
        const std::string basename_;
        const int flushEveryN_;

        int count_;
        std::unique_ptr<MutexLock> mutex_;
        std::unique_ptr<FileUtil> file_;
};

#endif
