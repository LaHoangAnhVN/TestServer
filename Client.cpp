#include <iostream>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include "check.hpp"
#include "Common.h"

#define CONTROLLEN CMSG_LEN(sizeof(int))
#define CL_OPEN "open"
#define MAXLINE 4096

class Client{
    int _socket;
    const char *name_server = "Server";
public:

    int sec_init(){
        sockaddr_un un;

        _socket = check(socket(AF_UNIX, SOCK_STREAM, 0));

        memset(&un, 0, sizeof(un));
        un.sun_family = AF_UNIX;
        strcpy(un.sun_path, name_server);
        int len = offsetof(struct sockaddr_un, sun_path) + strlen(name_server);

        if (connect(_socket, (sockaddr*)&un, len) < 0){
            std::cout<<"Connect failed."<<std::endl;
            return -1;
        }
        else{
            std::cout<<"Connected to " << un.sun_path << std::endl;
            return 1;
        }
    }

    int sec_close(){
        return close(_socket);
    }

    int send_request(const Request& req){
        if(try_send(_socket, req))
            return recv_fd();
        return -1;
    }

    int sec_open(const char* name){
        Request request;
        request.request_type = Request::REQ_OPEN;
        strcpy(request.name, name);
        int open_fd = send_request(request);
        if(open_fd == -1){
            std::cout<<"Cant open file"<<std::endl;
            return -1;
        }
        else{
            int count = 0;
            char byte;
            while(read(open_fd, &byte, 1)){
                count++;

            }
            lseek(open_fd, 0, SEEK_SET);
            char buf[6];
            check(read(open_fd, buf, 6));
            std::cout<<buf<<std::endl;
            return 1;
        }
    }

    int sec_unlink(const char* name){
        Request request;
        request.request_type = Request::REQ_UNLINK;
        strcpy(request.name, name);
        return send_request(request);

    }

    int recv_fd(){
        int newfd;
        struct iovec iov[1];
        struct msghdr msg;
        char buf[MAXLINE];
        struct cmsghdr *cmptr = (cmsghdr*)malloc(CONTROLLEN);

        iov[0].iov_base = buf;
        iov[0].iov_len = sizeof(buf);
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_control = cmptr;
        msg.msg_controllen = CONTROLLEN;

        check(recvmsg(_socket, &msg, 0));
        if(buf[1] != 0){
            return -1;
        }
        newfd = *(int*) CMSG_DATA(cmptr);

        free(cmptr);

        return newfd;
    }
};

int main(int argc, char* argv[]){
    char pathname[256];
    int oflag;

    strcpy(pathname, argv[1]);
    oflag = atoi(argv[2]);

    Client client;
    client.sec_init();

    if(oflag == 1){
        client.sec_open(pathname);
    }
    else{
        if(oflag == 2){
            client.sec_unlink(pathname);
        }
    }

    client.sec_close();
}