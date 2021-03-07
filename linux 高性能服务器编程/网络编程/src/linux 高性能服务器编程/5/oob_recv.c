#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


#define BUF_SIZE (1024)

int main(int argc,char* argv[])
{
	if(argc <= 2)
    {
        printf( "usage: %s ip_address port_number backlog\n", argv[0]);
        return 1;
    }
	const char* ip = argv[1];
	int port = atoi(argv[2]);

	struct sockaddr_in address;
	bzero(&address,sizeof(address));
	address.sin_family = PF_INET;
	inet_pton(AF_INET,ip,&address.sin_addr);
	address.sin_port = htons(port);
	
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	assert(sockfd >= 0);
	
	int ret = bind(sockfd,(struct sockaddr*)&address,sizeof(address));
	assert(ret != -1);
	
	ret = listen(sockfd,5);
	assert(ret != -1);
	
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int connfd = accept(sockfd,(struct sockaddr*)&client_addr,&client_addr_len);	
	if(connfd < 0)
	{
		perror("accept()");
	}
	else
	{
		char buffer[BUF_SIZE];
		memset(buffer,'\0',BUF_SIZE);
		ret = recv(connfd,buffer,BUF_SIZE-1,0);
		printf("got %d bytes of normal data %s:\n",ret,buffer);
		
		
		memset(buffer,'\0',BUF_SIZE);
		ret = recv(connfd,buffer,BUF_SIZE-1,MSG_OOB);
		printf("got %d bytes of oob data %s:\n",ret,buffer);
		
		memset(buffer,'\0',BUF_SIZE);
		ret = recv(connfd,buffer,BUF_SIZE-1,0);
		printf("got %d bytes of normal data %s:\n",ret,buffer);
	}
	
	close(sockfd);

	return 0;
}
