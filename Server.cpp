#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include "check.hpp"
#include "Common.h"

#define CONTROLLEN CMSG_LEN(sizeof(int))

class Server{
    int _listen_socket;
    std::vector<uid_t> list_uid;
    char name_server[256];
    char pathname_catalog[256];
public:

    Server(const char* filename){
        int fd_open = open(filename, O_RDONLY);//fopen()
        auto open_file = fopen(filename, "r");
        fscanf(open_file, "%s", name_server);
        fscanf(open_file, "%s", pathname_catalog);
        char buf[256];
        while(fscanf(open_file, "%s", buf) != EOF){
            list_uid.push_back((uid_t)atoi(buf));
        }

        auto de = opendir(pathname_catalog);
        if(de == NULL){
            mkdir(pathname_catalog, S_IRWXU);
        }
        else{
            closedir(de);
        }

        struct sockaddr_un un;

        _listen_socket = check(socket(AF_UNIX, SOCK_STREAM,0));
        unlink(name_server);

        memset(&un, 0, sizeof(un));
        un.sun_family = AF_UNIX;
        strcpy(un.sun_path, name_server);
        int len = offsetof(struct sockaddr_un, sun_path) + strlen(name_server);

        check(bind(_listen_socket, (sockaddr*)&un, len));
        check(listen(_listen_socket, 1));
        std::cout<<"Server is listening ...!"<<std::endl;
    }

    void Connect_client(){
        struct sockaddr_un un;
        socklen_t len = sizeof(un);

        while(true){
            int connect_socket = check(accept(_listen_socket, (sockaddr*)&un, &len));
            int optval = 1;
            check(setsockopt(connect_socket, SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval)));

            struct ucred scred;
            socklen_t _len = sizeof(struct ucred);
            check(getsockopt(connect_socket, SOL_SOCKET, SO_PEERCRED, &scred, &_len));
            std::cout<<"Connect from uid: " << scred.uid <<std::endl;
            std::cout<<"Pathname catalog: "<< pathname_catalog<<std::endl;
            for(int i = 0; i < list_uid.size(); i++){
                std::cout<<"uid: " <<list_uid[i]<<std::endl;
            }
            Request request;


            if(check_uid(scred.uid)){
                while(Recv_Req(connect_socket, request) >= 0){
                    std::cout<<request.name<<std::endl;
                }
            }
            else {
                send_fd(connect_socket, -1);
            }

        }
    }

    bool check_uid(uid_t uid){
        bool check_result = false;
        for(int i = 0; i < 3; i++){
            if(uid == list_uid[i]) check_result = true;
        }
        return check_result;
    }

    int Recv_Req(int fd, Request& req){
        if(try_recv(fd, req)){
            if(req.request_type == Request::REQ_OPEN){
                char new_path_name[1024];
                strcpy(new_path_name, pathname_catalog);
                strcat(new_path_name, req.name);
                int newfd = open(new_path_name, O_RDONLY);
                return send_fd(fd, newfd);
            }
            else{
                if(req.request_type == Request::REQ_UNLINK){
                    char new_path_name[1024];
                    strcpy(new_path_name, pathname_catalog);
                    strcat(new_path_name, req.name);
                    unlink(new_path_name);
                    int x = 0;
                    return send(fd, &x, sizeof(int), 0);
                }
            }
        }
        return -1;
    }

    int send_fd(int connect_socket, int newfd){
        struct iovec iov[1];
        struct msghdr msg;
        char buf[2];
        struct cmsghdr *cmptr = (cmsghdr*)malloc(CONTROLLEN);

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
        int x = check(sendmsg(connect_socket, &msg, 0));
        free(cmptr);
        return x;
    }
};

int main(){
    Server NewServer("/home/vtc15/fileconfigure.txt");
    NewServer.Connect_client();
}