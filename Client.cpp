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
#include "Common.h"

#define CONTROLLEN CMSG_LEN(sizeof(int))
#define CL_OPEN "open"
#define MAXLINE 4096

class Client{
    int _socket;
    const char *name_server = "Server";
public:
    /*Client(const char* name){
        strcpy(name_server, name);
        sec_init();

        Request new_request;
        new_request.request_type = Request::REQ_OPEN;
        strcpy(new_request.name, "new");
        send_request(new_request);

        sec_close();
    }*/

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

    /*Client(const char* name){
        sockaddr_un un;

        _socket = check(socket(AF_UNIX, SOCK_STREAM, 0));

        memset(&un, 0, sizeof(un));
        un.sun_family = AF_UNIX;
        strcpy(un.sun_path, name);
        int len = offsetof(struct sockaddr_un, sun_path) + strlen(name);

        check(connect(_socket, (sockaddr*) &un, len));

        Request new_request;
        new_request.reqquest_type = Request::Type::REQ_OPEN;
        strcpy(new_request.name, "/home/vtc15/new");
        std::cout<< send_request(new_request);

        close(_socket);
    }*/

    int send_request(const Request& req){
        check(send(_socket, &req, sizeof(req), MSG_OOB));
        return recv_fd();
    }

    int sec_open(const char* name){
        Request request;
        request.request_type = Request::REQ_OPEN;
        strcpy(request.name, name);
        return send_request(request);

        //check(send(_socket, &request, sizeof(request), MSG_OOB));
        //return recv_fd();
    }

    int sec_unlink(const char* name){
        Request request;
        request.request_type = Request::REQ_UNLINK;
        strcpy(request.name, name);
        return send_request(request);

        //check(send(_socket, &request, sizeof(request), MSG_OOB));
        //return recv_fd();
    }

    int recv_fd(){
        int newfd;
        struct iovec iov[1];
        struct msghdr msg;
        char buf[MAXLINE];
        struct cmsghdr *cmptr = NULL;

        while(true){
            iov[0].iov_base = buf;
            iov[0].iov_len = sizeof(buf);
            msg.msg_iov = iov;
            msg.msg_iovlen = 1;
            msg.msg_name = NULL;
            msg.msg_namelen = 0;
            msg.msg_control = cmptr;
            msg.msg_controllen = CONTROLLEN;

            check(recvmsg(_socket, &msg, 0));
            newfd = *(int*) CMSG_DATA(cmptr);

            return  newfd;
        }
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