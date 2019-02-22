#include "../Logging.h"
#include "../Thread.h"
#include <iostream>
#include <string>
#include <unistd.h>
using namespace std;

void threadFunc()
{
    for(int i = 0; i < 10000; ++i)
    {
        LOG << i;
    }
}

void type_test()
{
    // 13 lines
    cout << "----------type test-----------" << endl;
    LOG << 0;
    LOG << 1234567890123;
    LOG << 1.0f;
    LOG << 3.1415926;
    LOG << (short) 1;
    LOG << (long long) 1;
    LOG << (unsigned int) 1;
    LOG << (unsigned long) 1;
    LOG << (long double) 1.6555556;
    LOG << (unsigned long long) 1;
    LOG << 'c';
    LOG << "abcdefg";
    LOG << string("This is a string");
}

void stressing_single_thread()
{
    cout << "----------stressing test single thread-----------" << endl;
    threadFunc();
}

void multi_test()
{
    cout << "----------other test-----------" << endl;
    LOG << "Good job "<< 666 << string(" Congratulation my boy ") << 666;
}

int main()
{
    type_test();
    sleep(3);

    stressing_single_thread();
    sleep(3);

    multi_test();
    sleep(3);

    return 0;
}
