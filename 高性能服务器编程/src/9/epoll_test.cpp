#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/epoll.h>
#include <pthread.h>

#define MAX_EVENT_NUMBER (1024)
#define BUFFER_SIZE (10)

int Setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd, bool enable_et)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if (enable_et)
    {
        event.events |= EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    Setnonblocking(fd);
}

void lt(epoll_event *events, int number, int epollfd, int listenfd)
{
    char buf[BUFFER_SIZE];
    for (int i = 0; i < number; ++i)
    {
        int sockfd = events[i].data.fd;
        if (sockfd == listenfd)
        {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);
            addfd(epollfd, connfd, false);
        }
        else if (events[i].events & EPOLLIN)
        {
			//@ 只要 socket 读缓冲还有数据未读出，这段代码就会被触发
            printf("event trigger once\n");
            memset(buf, '\0', BUFFER_SIZE);
            int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
            if (ret <= 0)
            {
                close(sockfd);
                continue;
            }
            printf("get %d data: %s\n", ret, buf);
        }
        else
        {
            printf("something else happend\n");
        }
    }
}

void et(epoll_event *events, int number, int epollfd, int listenfd)
{
    char buf[BUFFER_SIZE];
    for (int i = 0; i < number; ++i)
    {
        int sockfd = events[i].data.fd;
        if (sockfd == listenfd)
        {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);
            addfd(epollfd, connfd, true);
        }
        else if (events[i].events & EPOLLIN)
        {
            //@ 这段代码不会被重复触发，因而需要循环读取数据，以确保将 socket 缓冲区的数据全部读出
            printf("event trigger once\n");
            while (1)
            {
                memset(buf, '\0', BUFFER_SIZE);
                int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
                if (ret < 0)
                {
					//@ 对于非阻塞 IO，下面的条件成立表示数据已经全部读取完毕，此后 epoll 就能再次触发 sockfd 上 EPOLLIN 事件，以驱动下一次操作
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                    {
                        printf("read later\n");
                        break;
                    }
                    close(sockfd);
                    break;
                }
                else if (ret == 0)
                {
                    close(sockfd);
                }
                else
                {
                    printf("get %d data:%s\n", ret, buf);
                }
            }
        }
        else
        {
            printf("something else happend\n");
        }
    }
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

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd, true);

    while (1)
    {
        int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (ret < 0)
        {
            perror("epoll_wait()");
            break;
        }

        lt(events, ret, epollfd, listenfd); //@ LT mode
        //et(events, ret, epollfd, listenfd); //@ ET mode
    }

    close(listenfd);

    return 0;
}
