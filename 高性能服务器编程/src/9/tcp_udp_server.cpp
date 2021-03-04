#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <pthread.h>


#define MAX_EVENT_NUMBER (1024)
#define TCP_BUFFER_SIZE  (1024)
#define UDP_BUFFER_SIZE  (512)

int setnoblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd,int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnoblocking(fd);
}

int main(int argc, char *argv[])
{

    if (argc <= 2)
    {
        printf("usage:%s <ip_address> <port_number> \n", argv[0]);
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    int ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port= htons(port);
    inet_pton(AF_INET,ip,&address.sin_addr);
    int udpfd = socket(PF_INET,SOCK_DGRAM,0);
    assert(udpfd >= 0);

    ret = bind(udpfd,(struct sockaddr*)&address,sizeof(address));
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd);
    addfd(epollfd,listenfd);
    addfd(epollfd,udpfd);

    while(1)
    {
        int number = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
        if(number < 0)
        {
            printf("epoll failed\n");
            break;
        }

        for(int i = 0;i < number;++i)
        {
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd)
            {
                struct  sockaddr_in clinet_addr;
                socklen_t clinet_addr_len = sizeof(clinet_addr);
                int connfd = accept(listenfd,(sockaddr*)&clinet_addr,&clinet_addr_len);
                if(connect < 0)
                {
                    printf("connection errno is:%d\n",errno);
                    continue;
                }
                addfd(epollfd,connfd);
            }
            else if(sockfd == udpfd)
            {
                char buf[UDP_BUFFER_SIZE];
                memset(buf,'\0',UDP_BUFFER_SIZE);
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                ret = recv(udpfd,buf,UDP_BUFFER_SIZE-1,0);
                if(ret > 0)
                {
                    sendto(udpfd,buf,UDP_BUFFER_SIZE-1,0,(struct sockaddr*)&client_addr,client_addr_len);
                }
            }
            else if(events[i].events & EPOLLIN)
            {
                char buf[TCP_BUFFER_SIZE];
                while(1)
                {
                    memset(buf,'\0',TCP_BUFFER_SIZE);
                    ret = recv(sockfd,buf,TCP_BUFFER_SIZE-1,0);
                    if(ret < 0)
                    {
                        if(errno == EAGAIN || errno == EWOULDBLOCK)
                            break; 
                        
                        close(sockfd);
                        break;
                    }
                    else if(ret == 0)
                    {
                        close(sockfd);
                    }
                    else 
                    {
                        send(sockfd,buf,ret,0);
                    }
                }
            }
            else
            {
                printf("something else happened\n");
            }
        }
    }

    close(listenfd);
    return 0;
}