#ifndef LOG_CURRENTTHREAD_H
#define LOG_CURRENTTHREAD_H

namespace CurrentThread
{
    extern __thread int t_cachedTid;    //线程实际ID的缓存，减少多次调用syscall(SYS_gettid)获取线程实际ID
    extern __thread char t_tidString[32];   //线程tid的字符串形式
    extern __thread int t_tidStringLength;  //线程tid的字符串形式的长度
    extern __thread const char* t_threadName;   //线程的名字
    void cacheTid();
    inline int tid()
    {
        if (__builtin_expect(t_cachedTid == 0, 0))  //gcc的优化语句，表明t_cachedTid大概率不为0
        {
            cacheTid();
        }
        return t_cachedTid;
    }
    
    inline const char* tidString() // for logging
    {
        return t_tidString;
    }

    inline int tidStringLength() // for logging
    {
        return t_tidStringLength;
    }
    
    inline const char* name()
    {
        return t_threadName;
    }

}

#endif
