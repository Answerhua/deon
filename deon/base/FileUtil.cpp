#include "FileUtil.h"
#include <string>
#include <cstdio>

FileUtil::FileUtil(std::string filename)
:   fp_(fopen(filename.c_str(), "ae"))
{
    // 为用户提供缓冲区
    setbuffer(fp_, buffer_, sizeof buffer_);
}

FileUtil::~FileUtil()
{
    fclose(fp_);
}

void FileUtil::append(const char* logline, const size_t len)
{
    size_t n = fwrite_unlocked(logline, 1, len, fp_);;
    size_t remain = len - n;
    while (remain > 0)  //将没写完的部分写入
    {
        size_t x = fwrite_unlocked(logline + n, 1, remain, fp_);;
        if (x == 0)
        {
            int err = ferror(fp_);
            if (err)
                fprintf(stderr, "FileUtil::append() failed !\n");
            break;
        }
        n += x;
        remain = len - n;
    }
}

void FileUtil::flush()
{
    fflush(fp_);    //清空文件缓存区
}
