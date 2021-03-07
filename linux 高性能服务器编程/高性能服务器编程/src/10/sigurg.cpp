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
#include <signal.h>

#define BUF_SIZE (1024)
static int connfd;

void sig_urg(int sig)
{
    int save_errno = errno;
    char buf[BUF_SIZE];
    memset(&buf, '\0', BUF_SIZE);
    int ret = recv(connfd, buf, BUF_SIZE - 1, MSG_OOB);
    printf("get %d bytes of oob data: %s", ret, buf);
    errno = save_errno;
}

void addsig(int sig, void (*sig_handler)(int))
{
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL) != -1);
}

int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("usage %s <ip_address> <port_number>\n", argv[0]);
        return 1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    connfd = accept(sock,(struct sockaddr*)&client,&client_len);
    if(connfd < 0)
    {
        perror("accept");
        close(sock);
        return 1;
    }

    addsig(SIGURG,sig_urg);
    //@ 使用 SIGURG 之前必须设置 socket 的宿主进程或进程组
    fcntl(connfd,F_SETOWN,getpid());
    char buf[BUF_SIZE];
    while(1)
    {
        memset(buf,'\0',BUF_SIZE);
        ret = recv(connfd,buf,BUF_SIZE-1,0);
        if(ret <= 0)
            break;
        printf("get %d bytes normal data %s\n",ret,buf);
    }

    close(sock);
    return 0;
}
