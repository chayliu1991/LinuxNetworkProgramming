#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <signal.h>


#define MAX_EVENT_NUMBER (1024)
static int pipefd[2];


int setnonblocking(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}


void addfd(int epollfd,int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    //@ 信号直接写入到管道之中，以通知主循环
    send(pipefd[1],(char*)&msg,1,0);
    errno = save_errno;
}

//@ 设置信号处理函数
void add_sig(int sig)
{
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL)!=-1);
}

int main(int argc,char* argv[])
{
    if(argc <= 2)
    {
        printf("usage %s <ip_address> <port_number>\n",argv[0]);
        return 1;
    }

    char * ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET,ip,&address.sin_addr);

    int listenfd = socket(PF_INET,SOCK_STREAM,0);
    assert(listenfd >= 0);

    int ret = bind(listenfd,(struct sockaddr*)&address,sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd,5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epoll_fd = epoll_create(5);
    assert(epoll_fd != -1);
    addfd(epoll_fd,listenfd);

    ret = socketpair(PF_UNIX,SOCK_STREAM,0,pipefd);
    assert(ret != -1);
    setnonblocking(pipefd[1]);
    setnonblocking(pipefd[0]);

    //@ 设置一些信号的处理函数
    add_sig(SIGHUP);
    add_sig(SIGCHLD);
    add_sig(SIGTERM);
    add_sig(SIGINT);
    int stop_server = false;

    while(!stop_server)
    {
        int number = epoll_wait(epoll_fd,events,MAX_EVENT_NUMBER,-1);
        if((number < 0) && (errno != EINTR))
        {
            printf("epoll error\n");
            break;
        }

        for(int i = 0;i < number;++i)
        {
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd)
            {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int connfd = accept(listenfd,(struct sockaddr*)&client_addr,&client_addr_len);
                addfd(epoll_fd,connfd);
            }
            else if((sockfd == pipefd[0]) && (events[i].events &  EPOLLIN))
            {
                int sig;
                char signals[1024];
                ret = recv(pipefd[0],signals,sizeof(signals),0);
                if(ret == -1)
                    continue;
                if(ret == 0)
                    continue;
                for(int i = 0;i < ret;++i)
                {
                    switch(signals[i])
                    {
                        case SIGHUP:
                        case SIGCHLD:
                            continue;
                        case SIGTERM:
                        case SIGINT:
                            stop_server = true;
                    }
                }
            }
        }
    }

    printf("close fds");
    close(listenfd);
    close(pipefd[0]);
    close(pipefd[1]);
    return 0;
}
