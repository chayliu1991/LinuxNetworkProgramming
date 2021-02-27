#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>



int main(int argc,char* argv[])
{
	if(argc <= 2)
    {
        printf( "usage: %s ip_address port_number backlog\n", basename( argv[0] ) );
        return 1;
    }
	
	const char* ip = argv[1];
	int port = atoi(argv[2]);
	
	struct sockaddr_in address;
	bzero(&addres,sizeof(address));
	address.sin_family = PF_INET;
	inet_pton(AF_INET,ip,&address.sin_addr);
	address.sin_port = htons(port);
	
	int sock = socket(AF_INET,SCOK_STREAM,0);
	assert(sock >= 0);
	
	int ret = bind(sock,(struct sockaddr*)&address,sizeof(address)); 
	assert(ret != -1);
	
	ret = listen(sock,5);
	assert(ret != -1);
	
	sleep(20);
	
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int connfd = accept(sock,(struct sockaddr*)&client_addr,&client_addr_len);
	if(connfd < 0)
	{
		perror("accept()");
	}
	else
	{
		char remote[INET_ADDRSTRLEN];
		printf("connected with ip:%s and port:%d\n",inet_ntop(AF_INET,&client_addr.sin_addr,remote,INET_ADDRSTRLEN)，ntohs(client_addr.sin_port));
		close(connfd);
	}
	close(sock);
	return 0;
}






