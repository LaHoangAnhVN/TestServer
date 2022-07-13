#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "check.hpp"
#include "Common.h"

#define CONTROLLEN CMSG_LEN(sizeof(int))

class Server{
    int _listen_socket;
    uid_t list_uid[3];
    char name_server[256];
    char pathname_catalog[256];
public:

    Server(const char* name){
        strcpy(name_server, name);
        list_uid[0] = 1000;
        list_uid[1] = 2000;
        list_uid[2] = 3000;
        strcpy(pathname_catalog, "/home/vtc15");

        struct sockaddr_un un;

        _listen_socket = check(socket(AF_UNIX, SOCK_STREAM,0));
        unlink(name);

        memset(&un, 0, sizeof(un));
        un.sun_family = AF_UNIX;
        strcpy(un.sun_path, name);
        int len = offsetof(struct sockaddr_un, sun_path) + strlen(name);

        check(bind(_listen_socket, (sockaddr*)&un, len));
        check(listen(_listen_socket, 1));
        std::cout<<"Server is listening ...!"<<std::endl;
    }

    void Connect_client(){
        struct sockaddr_un un;
        socklen_t len = sizeof(un);

        check(accept(_listen_socket, (sockaddr*)&un, &len));
        int optval = 1;
        check(setsockopt(_listen_socket, SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval)));

        struct ucred scred;
        socklen_t _len = sizeof(struct ucred);
        check(getsockopt(_listen_socket, SOL_SOCKET, SO_PEERCRED, &scred, &_len));
        std::cout<<"Connect from uid: " << scred.uid <<std::endl;
        Request request;

        while(true){
            check(Recv_Req(request));
            std::cout<<recv;
        }
    }

    bool check_uid(int uid){
        bool check_result = false;
        for(int i = 0; i < 3; i++){
            if(uid == list_uid[i]) check_result = true;
        }
        return check_result;
    }

    int Recv_Req(Request& req){
        if(try_recv(_listen_socket, req)){
            if(req.request_type == Request::REQ_OPEN){
                int newfd = open(req.name, O_RDONLY);
                return send_fd(newfd);
            }
            else{
                if(req.request_type == Request::REQ_UNLINK){
                    char* new_path_name ;
                    strcpy(new_path_name, pathname_catalog);
                    strcat(new_path_name, req.name);
                    return unlink(new_path_name);
                }
            }
        }
        return -1;
    }

    int Revc_req_with_uid(Request& req, uid_t uid){
        if(check_uid(uid)){
            check(recv(_listen_socket, &req, sizeof(req), MSG_WAITALL));
            if(req.request_type == Request::REQ_UNLINK){
                char* new_path_name ;
                strcpy(new_path_name, pathname_catalog);
                strcat(new_path_name, req.name);
                return unlink(new_path_name);
            }
            int newfd = open(req.name, O_RDONLY);
            return send_fd(newfd);
        }
    }

    int send_fd(int newfd){
        struct iovec iov[1];
        struct msghdr msg;
        char buf[2];
        struct cmsghdr *cmptr = NULL;

        iov[0].iov_base = buf;
        iov[0].iov_len = 2;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;
        msg.msg_name = NULL;
        msg.msg_namelen = 0;

        if(newfd < 0){
            msg.msg_control = NULL;
            msg.msg_controllen = 0;
            buf[1] = - newfd;
            if(buf[1] == 0) buf[1] = 1;
        }
        else{
            cmptr->cmsg_level = SOL_SOCKET;
            cmptr->cmsg_type = SCM_RIGHTS;
            cmptr->cmsg_len = CONTROLLEN;
            msg.msg_control = cmptr;
            msg.msg_controllen = CONTROLLEN;

            *(int*) CMSG_DATA(cmptr) = newfd;
            buf[1] = 0;
        }

        buf[0] = 0;
        return check(sendmsg(_listen_socket, &msg, 0));
    }
};

int main(){
    Server NewServer("Server");
    NewServer.Connect_client();
}