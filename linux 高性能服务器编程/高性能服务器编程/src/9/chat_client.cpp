#define _GNU_SOURECE 1
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
#include <poll.h>
#include <fcntl.h>

#define BUFFER_SIZE (64)

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

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int ret = connect(sockfd, (struct sockaddr *)&address, sizeof(address));
    if (ret < 0)
    {
        printf("connection failed\n");
        close(sockfd);
        return 1;
    }

    pollfd fds[2];
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents = 0;

    char rbuf[BUFFER_SIZE];
    int pipefd[2];
    ret = pipe(pipefd);
    assert(ret != -1);

    while(1)
    {
        ret = poll(fds,2,-1);
        if(ret < 0)
        {
            printf("poll failed\n");
            break;
        }

        if(fds[1].revents & POLLRDHUP)
        {
            printf("server close the connetion\n");
            break;
        }
        else if(fds[1].revents & POLLIN)
        {
            memset(rbuf,'\0',BUFFER_SIZE);
            recv(fds[1].fd,rbuf,BUFFER_SIZE-1,0);
            printf("Got:%s\n",rbuf);
        }


        if(fds[0].revents & POLLIN)
        {
            ret = splice(0,NULL,pipefd[1],NULL,32768,SPLICE_F_MORE | SPLICE_F_MOVE);
            ret = splice(pipefd[0],NULL,sockfd,NULL,32768,SPLICE_F_MORE | SPLICE_F_MOVE);
        }
    }

    close(sockfd);

    return 0;
}
