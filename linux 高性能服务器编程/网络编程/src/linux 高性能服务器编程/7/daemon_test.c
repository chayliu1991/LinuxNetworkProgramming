#include<stdio.h>
#include<unistd.h>
#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>


typedef enum
{
    false = 0,
     true = 1,
}bool;

bool daemonize()
{
	//@ 创建子进程，关闭父进程，这样可以使程序后台运行
    pid_t pid = fork();
    if(pid < 0)
    {
        return false;
    }
    else if(pid > 0)
    {
        exit(0);
    }

	//@ 设置文件权限掩码，当进程创建新的文件
	//@ 使用 open(const char* pathname,int flags,mode_t mode) 时文件的权限将被设置成 mode & 0777
    umask(0);
	
	//@ 创建新的会话，设置本进程为进程组的首领
    pid_t sid = setsid();
    {
        if(sid < 0)
            return false;
    }

	//@ 切换工作目录
    if(chdir("/") < 0)
        return false;

	//@ 关闭标准输入，标准输出，和标准错误设备
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
	
	//@ 关闭其它打开的文件描述符
	
	//@ 将标准输入，标准输出和标准错误都定向到 /dev/null 文件
    open( "/dev/null", O_RDONLY );
    open( "/dev/null", O_RDWR );
    open( "/dev/null", O_RDWR );

}

int main()
{
    return 0;
}









