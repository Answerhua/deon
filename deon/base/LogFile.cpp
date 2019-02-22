#include "LogFile.h"
#include "FileUtil.h"
#include <string>

LogFile::LogFile(const std::string &basename, int flushEveryN)
    : basename_(basename),
    flushEveryN_(flushEveryN),
    count_(0),
    mutex_(new MutexLock)
{
    file_.reset(new FileUtil(basename));
}

LogFile::~LogFile()
{ }

void LogFile::append(const char* logline, int len)
{
    MutexLockGuard lock(*mutex_);
    file_->append(logline, len);
    ++count_;
    if (count_ >= flushEveryN_)
    {
        count_ = 0;
        file_->flush();
    }
}

void LogFile::flush()
{
    MutexLockGuard lock(*mutex_);
    file_->flush();
}


