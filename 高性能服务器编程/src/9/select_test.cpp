#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


int main(int argc,char* argv[])
{
    if(argc <=2)
    {
        printf("usage :%s <ip_address> <port_number>\n",argv[0]);
        return 1;
    }


    const char* ip = argv[1];
    int port = atoi(argv[2]);


    int ret = 0;
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET,ip,&address.sin_addr);

    int sock = socket(PF_INET,SOCK_STREAM,0);
    assert(sock >= 0);

    ret = bind(sock,(struct sockaddr*)&address,sizeof(address));
    assert(ret != -1);

    ret = listen(sock,5);
    assert(ret != -1);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
	int connfd = accept(sock,(struct sockaddr*)&client_addr,&client_addr_len);
    if(connfd < 0)
    {
        perror("connect()");
        close(sock);
        return 1;
    }

    char buf[1024];
    fd_set read_fds;
    fd_set exception_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&exception_fds);

    while(1)
    {
        memset(buf,'\0',sizeof(buf));
        FD_SET(connfd,&read_fds);
        FD_SET(connfd,&exception_fds);
        ret = select(connfd+1,&read_fds,NULL,&exception_fds,NULL);
        if(ret < 0)
        {
            printf("select error\n");
            break;
        }

        if(FD_ISSET(connfd,&read_fds))
        {
            ret = recv(connfd,buf,sizeof(buf)-1,0);
            if(ret <= 0)
                break;
            printf("get %d bytes of normal data:%s",ret,buf);
        }        
        else if(FD_ISSET(connfd,&exception_fds))
        {
            ret = recv(connfd,buf,sizeof(buf)-1,MSG_OOB);
            if(ret <= 0)
            {
                break;
            }
            printf("get %d bytes data :%s\n",ret,buf);
        }
    }

    close(sock);
    close(connfd);

    return 0;
}










































































