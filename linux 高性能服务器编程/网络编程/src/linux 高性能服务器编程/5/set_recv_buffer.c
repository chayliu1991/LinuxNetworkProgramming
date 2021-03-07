#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


#define BUFFER_SIZE (1024)

int main(int argc,char* argv[])
{
	if(argc <= 2)
    {
        printf( "usage: %s ip_address port_number backlog\n", argv[0]);
        return 1;
    }
	
	const char* ip = argv[1];
	int port = atoi(argv[2]);
	
	struct sockaddr_in  address;
	bzero(&address,sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET,ip,&address.sin_addr);
	address.sin_port = (htons(port));
	
	int sock = socket(PF_INET,SOCK_STREAM,0);
	assert(sock >= 0);
	
	int recvbuf = atoi(argv[3]);
	int len = sizeof(recvbuf);
	
	//@ 先设置 TCP 发送缓冲区的大小，然后立即读取之
	setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&recvbuf,sizeof(recvbuf));
	getsockopt(sock,SOL_SOCKET,SO_RCVBUF,&recvbuf,(socklen_t*)&len);
	printf("the tcp receive buffer size after setting is %d \n",recvbuf);
	
	int ret = bind(sock,(struct sockaddr*)&address,sizeof(address));
	assert(ret != -1);
	
	ret = listen(sock,5);
	assert(ret != -1);
	
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int connfd = accept(sock,(struct sockaddr*)&client_addr,&client_addr_len);
	if(connfd < 0)
	{
		perror("accept()");
	}
	else
	{
		char buffer[BUFFER_SIZE];
		memset(buffer,'\0',BUFFER_SIZE);
		while(recv(connfd,buffer,BUFFER_SIZE-1,0) > 0)
		{
			;
		}
		close(connfd);
	}
	
	close(sock);
	return 0;
}























