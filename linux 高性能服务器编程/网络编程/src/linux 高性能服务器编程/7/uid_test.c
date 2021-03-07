#include <unistd.h>
#include <stdio.h>

/
*
before call this app:
sudo chown root:root app  //@ 修改目标文件的所有者
sudo chmod +s app 		  //@  设置目标文件的 set-user-id 标志

*/

int main()
{
    uid_t uid = getuid();
    uid_t euid = geteuid();
    printf("user id is:%u,effective  user id is:%u\n",uid,euid);


    gid_t gid = getgid();
    gid_t guid = getegid();
    printf("group id is:%u,effective  group id is:%u\n",gid,guid);

    return 0;
}
