## LOG的设计
### 设计思路
- 参考了[muduo](https://github.com/chenshuo/muduo)的日志设计
- 一个日志库大体可分为前端与后端。前端是提供应用程序使用的接口，并生成日志信息；后端则是负责将日志信息写到目的地。
- 每个线程都有一个前端，而整个程序共用一个后端。
- 用一个线程，收集日志信息，并写入文件。其他线程负责往"日志线程"上发送信息。

### 具体实现
- 与LOG直接相关的类有FileUtil、LogFile、LogStream、AsyncLogging、Logging
- FileUtil是最底层的写文件类，封装了打开文件，写文件（在文件后直接append），关闭文件（类析构时关闭）等功能。
- LogFile是一个对FileUtil的简单封装，当FileUtil中的append()执行到一定次数的时候，自动刷新(flush)。
- LogStream中封装了两个类：FixedBuffer类和LogStream类
	- FixedBuffer是简单的缓冲区类，其底层的实现是一个char数组。
	- LogStream通过重载 << 运算符，完成格式化输出的功能。使用FixedBuffer的缓冲区，将输出结果整合到一起。
- AsyncLogging，启动一个log线程，将已经写好的Buffer放入buffersToWrite队列中(vector),然后队列中的buffer内容，写到LogFile提供的接口中。
- AsyncLogging中准备两块缓冲，采用双缓冲的技术。
- Logging是最终封装好的接口类。提供LOG接口的功能。