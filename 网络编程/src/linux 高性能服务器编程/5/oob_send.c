#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

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
	if(connect(sockfd,(struct sockaddr*)&address,sizeof(address)) < 0)
	{
		perror("connect()");
	}
	else
	{
		const char* oob_data = "abc";
		const char* normal_data = "123";
		send(sockfd,normal_data,strlen(normal_data),0);
		send(sockfd,oob_data,strlen(oob_data),MSG_OOB);
		send(sockfd,normal_data,strlen(normal_data),0);
	}
	
	close(sockfd);

	return 0;
}
