#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>


int main(int argc,char* argv[])
{
	if(argc != 2)
	{
		printf("usage:%s <host>\n",argv[0]);
		return 1;
	}
	
	char* host = argv[1];
	struct hostent* hostinfo = gethostbyname(host);
	assert(hostinfo);
	
	struct servent* servinfo = getservbyname("daytime","tcp");
	assert(servinfo);
	printf("daytime port is %d \n",ntohs(servinfo->s_port));
	
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port  = servinfo->s_port;
	address.sin_addr = *(struct in_addr*)*hostinfo->h_addr_list;
	
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	int result = connnect(sockfd,(struct sockaddr*)&address,sizeof(address));
	assert(result != -1);
	
	char buffer[128];
	result = read(sockfd,buffer,sizeof(buffer));
	assert(result > 0);
	buffer[result] = '\0';
	printf("the day time is:%s\n",buffer);
	close(fd);
		
	return 0;
}
















