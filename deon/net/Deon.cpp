#include "../base/Logging.h"
#include "Server.h"
#include <iostream>
#include <getopt.h>
#include <string>

const std::string version = "0.1";
static void usage() {
   fprintf(stdout,
    "Deon [option]... \n"
    "  -t The number of threads\n"
    "  -p The port number\n"
    "  -l The logfile path\n"
    "  -r The root of server"
    "  -h Display the instructions\n"
    "  -v Display the version\n"
    );
}

int main(int argc, char *argv[])
{
    int threadNum = 4;
    int port = 80;
    std::string root = "";
    std::string logPath = "./deon.log";

    int opt;
    const char *str = "t:p:l:r:hv";
    while ((opt = getopt(argc, argv, str))!= -1)
    {
        switch(opt)
        {
            case 't':
            {
                threadNum = atoi(optarg);
                break;
            }
            case 'p':
            {
                port = atoi(optarg);
                break;
            }
            case 'l':
            {
                logPath = optarg;
                break;
            }
            case 'r':
            {
                root = optarg;
                if(root[root.size() - 1] != '/')
                {
                    root += '/';
                }
                break;
            }
            case 'h':
            {
                usage();
                return 0;
            }
            case 'v':
            {
                std::cout<<"Deon version: "<< version << std::endl;
                return 0;
            }
            default:
            {
                break;
            }
        }
    }

    Logger::setLogFileName(logPath);
    EventLoop mainLoop;
    Server httpServer(&mainLoop, threadNum, port, root);
    httpServer.start();
    mainLoop.loop();
    return 0;
}
