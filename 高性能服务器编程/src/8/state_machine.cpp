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

#define BUFFER_SIZE (4096)


//@ 主状态机
enum CHECK_STATE
{
    CHECK_STATE_REQUESTLINE = 0, //@ 分析请求行
    CHECK_STATE_HEADER = 1, //@ 分析头部字段
};

//@ 读取行从状态机
enum LINE_STATUS
{
    LINE_OK = 0, //@ 读取到一个完整的行
    LINE_BAD = 1, //@ 读取行出错
    LINE_OPEN = 2, //@ 行数据尚不完整
};

//@ 请求结果
enum HTTP_CODE 
{
    NO_REQUEST = 0, //@ 请求不完整，需要继续接收客户端数据
    GET_REQUEST = 1, //@ 获得了一个完整的客户端请求
    BAD_REQUEST = 2, //@ 客户端请求有语法错误
    FORBIDDEN_REQUEST = 3, //@ 客户端对资源没有足够的访问权限
    INTERNAL_ERROR = 4, //@ 服务器内部错误
    CLOSED_CONNECTION = 5, //@ 客户端已经发送了关闭连接
};

//@ 应答报文
static const char* szret[] = {"I get a correct result\n","something wrong\n"};


LINE_STATUS parse_line(char* buffer,int & checked_index,int& read_index)
{
    char temp;
    for(;checked_index < read_index;++checked_index)
    {
        temp = buffer[checked_index];
        if(temp == '\r')
        {
            if((checked_index+1) == read_index)
                return LINE_OPEN;
        }
        else if(buffer[checked_index+1] == '\n')
        {
            buffer[checked_index++] = '\0';
            buffer[checked_index++] = '\0';
            return LINE_OK;
        }
        else if(temp == '\n')
        {
            if((checked_index > 1) && buffer[checked_index-1] == '\r')
            {
                buffer[checked_index-1] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_OK;
            }
        }

        return LINE_BAD;
    }
    return LINE_OPEN;
}

HTTP_CODE parse_requesline(char* temp,CHECK_STATE& check_state)
{
    char* url = strpbrk(temp," \t");
    if(!url)
        return BAD_REQUEST;
    
    *url++ = '\0';
    char* method = temp;
    if(strcasecmp(method,"GET") == 0)
        printf("the request method is GET");
    else
        return BAD_REQUEST;
    
    url +=  strspn(url," \t");
    char* version = strpbrk(url," \t");
    if(!version)
        return BAD_REQUEST; 
    
    *version++ = '\0';
    version += strspn(version," \t");
    if(strcasecmp(version,"HTTP/1.1") != 0)
        return BAD_REQUEST;
    
    if(strncasecmp(url,"http://",7) == 0)
    {
        url += 7;
        url = strchr(url,'/');        
    }
    

    if(!url || url[0] != '/')
        return BAD_REQUEST;
    
    printf("the request url is :%s \n",url);
    check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

HTTP_CODE parse_headers(char* temp)
{
    if(temp[0] == '\0')
        return GET_REQUEST;
    else if(strncasecmp(temp,"Host:",5) == 0)
    {
        temp += 5;
        temp += strspn(temp," \t");
        printf("the request host is:%s \n",temp);
    }
    else
    {
        printf("I can not handle this header\n");
    }
    return NO_REQUEST;
}


HTTP_CODE parse_content(char* buffer,int& checked_index,CHECK_STATE& checkstate,int& read_index,int& start_line)
{
    LINE_STATUS linestatus = LINE_OK;
    HTTP_CODE retcode = NO_REQUEST;

    while((linestatus = parse_line(buffer,checked_index,read_index)) == LINE_OK)
    {
        char* temp = buffer + start_line;
        start_line = checked_index;
        switch (checkstate)
        {
            case  CHECK_STATE_REQUESTLINE:
            {
                retcode = parse_requesline(temp,checkstate);
                if(retcode == BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            }
            case CHECK_STATE_HEADER:
            {
                retcode = parse_headers(temp);
                if(retcode == BAD_REQUEST)
                    return BAD_REQUEST;
                else if(retcode == GET_REQUEST)
                    return GET_REQUEST;
                break;
            }
            default:
            {
                return INTERNAL_ERROR;
            }
        }
    }

    if(linestatus == LINE_OPEN)
        return NO_REQUEST;
    else
        return BAD_REQUEST;
}


int main(int argc,char* argv[])
{
    if(argc <=2)
    {
        printf("usage:%s <ip_address> <port_number> \n",argv[0]);
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET,ip,&address.sin_addr);

    int sock = socket(PF_INET,SOCK_STREAM,0);
    assert(sock >= 0);
    
    int ret = bind(sock,(struct sockaddr*)&address,sizeof(address));
    assert(ret != -1);

    ret = listen(sock,5);
    assert(ret != -1);

    struct sockaddr_in client_addr;
    socklen_t clinet_addr_len = sizeof(client_addr);
    int fd = accept(sock,(struct sockaddr*)&client_addr,&clinet_addr_len);
    if(fd < 0)
    {
        perror("accept()");
    }
    else
    {
        char buffer[BUFFER_SIZE];
        memset(buffer,'\0',BUFFER_SIZE);
        int data_read = 0;
        int read_index = 0;
        int checked_index = 0;
        int start_line = 0;

        CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;
        while(1)
        {
            data_read = recv(fd,buffer+read_index,BUFFER_SIZE-read_index,0);
            if(data_read == -1)
            {
                printf("reading failed\n");
                break;
            }
            else if(data_read == 0)
            {
                printf("remote client has closed connection\n");
                break;
            }

            read_index += data_read;
            HTTP_CODE result = parse_content(buffer,checked_index,checkstate,read_index,start_line);
            if(result == NO_REQUEST)
                continue;
            else if(result == GET_REQUEST)
            {
                send(fd,szret[0],strlen(szret[0]),0);
                break;
            }
            else
            {
                send(fd,szret[1],strlen(szret[1]),0);
                break;
            }
        }

        close(fd);
    }

    close(sock);
    return 0;
}
