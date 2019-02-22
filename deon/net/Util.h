#ifndef NET_UTIL_H
#define NET_UTIL_H

#include <cstdlib>
#include <string>

int socket_bind_listen(int port);
void handle_for_sigpipe();
int set_socket_non_blocking(int fd);
void set_socket_no_delay(int fd);
ssize_t readn(int fd, void *buffer, ssize_t n);
ssize_t readn(int fd, std::string &inBuffer, bool &zero);
ssize_t writen(int fd, void *buff, size_t n);
ssize_t writen(int fd, std::string &sbuff);
ssize_t writenwc(int fd, std::wstring &sbuff);
std::wstring s2ws(const std::string& s);
std::string ws2s(const std::wstring& ws);

#endif
