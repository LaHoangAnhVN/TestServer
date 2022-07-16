#include <iostream>
#include <arpa/inet.h>
#include <string>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include "check.hpp"

struct Request{
    enum Type{
        REQ_OPEN,
        REQ_UNLINK
    };
    Type request_type;
    char name[256];
};

inline bool try_send(int fd, const Request& request){
    errno = 0;
    int size = check_except(send(fd, &request, sizeof(Request), 0), EPIPE);
    return size>0;
}

inline bool try_recv(int fd, Request& request){
    errno = 0;
    int size = check_except(recv(fd, &request, sizeof(Request), MSG_WAITALL), EPIPE);
    return size>0;
}
