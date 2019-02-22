set(SRCS
    Channel.cpp
    EpollPoller.cpp
    EventLoop.cpp
    EventLoopThread.cpp
    EventLoopThreadPool.cpp
	Http.cpp
	Deon.cpp
    Server.cpp
    Timer.cpp
    Util.cpp
)

include_directories(../base)

add_executable(deon ${SRCS})
target_link_libraries(deon libserver_base)

