#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define BUFFER_SIZE (1024)

static const char* status_line[2] = {"200 OK","500 Internal server error"};

int main(int argc,char* argv[])
{
	if(argc <= 3)
	{
		printf("usage: %s <ip_address> <port_number> <filename> \n",argv[1]);
		return 1;
	}

	const char* ip = argv[1];
	int port = atoi(argv[2]);
	const char* file_name = argv[3];

	struct sockaddr_in address;
	bzero(&address,sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	inet_pton(PF_INET,ip,&address.sin_addr);

	int sock = socket(AF_INET,SOCK_STREAM,0);
	assert(sock >= 0);

	int ret = bind(sock,(struct sockaddr*)&address,sizeof(address));
	assert(ret != -1);

	ret = listen(sock,5);
	assert(ret != -1);

	struct sockaddr_in client_addr;
	socklen_t clinet_addr_len = sizeof(client_addr);
	int connfd = accept(sock,(struct sockaddr*)&client_addr,&clinet_addr_len);
	if(connfd < 0)
	{
		perror("accept()");
	}
	else
	{
		char header_buff[BUFFER_SIZE];
		memset(&header_buff,'\0',sizeof(header_buff));
		char * file_buf;
		struct stat file_stat;
		bool valid = true;
		int len = 0;
		if(stat(file_name,&file_stat) < 0)
		{
			valid = false;
		}
		else
		{
			if(S_ISDIR(file_stat.st_mode))
			{
				valid = false;
			}
			else if(file_stat.st_mode & S_IROTH)
			{
				int fd = open(file_name,O_RDONLY);
				file_buf = new char[file_stat.st_size + 1];
				memset(file_buf,'\0',file_stat.st_size + 1 );
				if(read(fd,file_buf,file_stat.st_size) < 0)
				{
					valid = false;
				}
			}
			else
			{
				valid = false;
			}
		}

		if(valid)
		{
			ret = snprintf(header_buff,BUFFER_SIZE-1,"%s %s\r\n","HTTP/1.1",status_line[0]);
			len += ret;
			ret = snprintf(header_buff+len,BUFFER_SIZE-1-len,"Content-Length:%d\r\n",file_stat.st_size);
			len += ret;
			ret = snprintf(header_buff+len,BUFFER_SIZE-1-len,"%s","\r\n");

			struct iovec iv[2];
			iv[0].iov_base = header_buff;
			iv[0].iov_len = strlen(header_buff);
			iv[1].iov_base = file_buf;
			iv[1].iov_len = file_stat.st_size;
			ret = writev(connfd,iv,2);
		}
		else
		{
			ret = snprintf(header_buff,BUFFER_SIZE-1,"%s %s\r\n","Http/1.1",status_line[1]);
			len += ret;
			ret = snprintf(header_buff,BUFFER_SIZE-1-len,"%s","\r\n");
			send(connfd,header_buff,strlen(header_buff),0);
		}

		close(connfd);
		delete [] file_buf;
	}

	close(sock);
	return 0;
}

























