#ifndef LOG_NONCOPYABLE_H
#define LOG_NONCOPYABLE_H

class noncopyable
{
  public:
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;

  protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

#endif
