#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <linux/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
/**
 * @brief 分析封包并将数据写入文件
 * 
 * @param sockfd 套接字描述符
 * @param filename 文件名
 * @return int 返回操作结果，-1表示失败，0表示成功
 */
int analysis_pkg_put(int sockfd, char *filename)
{
    // 设置接收超时时间为1秒
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

    int ret;
    unsigned char ch;
    char buf[5000];

    // 打开文件
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1)
    {
        perror("open error");
    }

    // 接收封包头部
    do
    {
        ret = recvfrom(sockfd, &ch, 1, 0, NULL, NULL);
        if (ret == 0)
        {
            return -1;
        }
    } while (ch != 0xc0);

    // 跳过连续的0xc0字节
    while (ch == 0xc0)
    {
        recvfrom(sockfd, &ch, 1, 0, NULL, NULL);
    }

    unsigned char cmd[512] = {0};
    int i = 0;
    // 读取封包头部
    while (i < 3)
    {
        cmd[i++] = ch;
        recvfrom(sockfd, &ch, 1, 0, NULL, NULL);
    }
    cmd[i++] = ch;

    // 接收封包数据并写入文件
    while (ret = recvfrom(sockfd, buf, 5000, 0, NULL, NULL))
    {
        printf("ret = %d\n", ret);
        if (ret == -1)
        {
            break;
        }
        i += ret;
        printf("buf[ret-1]:%d\n", buf[ret - 1]);
        if (buf[ret - 1] != -64)
        {
            printf("ok\n");
            write(fd, buf, ret);
        }
        else if (buf[ret - 1] == -64)
        {
            write(fd, buf, ret - 1);
        }
    }

    // 解析封包大小
    int pkg_len = (cmd[0] & 0xff) | ((cmd[1] & 0xff) << 8) | ((cmd[2] & 0xff) << 16) | ((cmd[3] & 0xff) << 24);
    if (pkg_len - 1 != i)
    {
        printf("pkg_len error\n");
    }
    printf("pkg_len:%d i:%d\n", pkg_len, i);
    //printf("%s\n",&cmd[4]);
}
/*
    @sockfd:客户端的套接字
    @buf:文件的名字
*/
int make_pkg_get(int sockfd, char *buf)
{
    int ret;
    char filename[512] = {0};
    int i = 0;
    // 从缓冲区中读取文件名
    while (buf[i] != '\n' && buf[i] != '\0')
    {
        filename[i] = buf[i];
        i++;
    }

    // 客户端IP和端口
    struct sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_port = htons(7777);
    client.sin_addr.s_addr = inet_addr("172.11.0.117");

    struct stat statbuf;
    ret = stat(filename, &statbuf);
    if (ret == -1)
    {
        perror("stat error");
    }
    printf("filename :%s size:%ld\n ", filename, statbuf.st_size);
    unsigned long int filesize = statbuf.st_size;

    int fd;
    fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        perror("open error");
    }

    ret = -1;
    while (1)
    {
        // 以下是封包初始化
        int pkg_len = 5; // 封包的大小
        char re_pkg[15000] = {0}; // 封包
        i = 5;
        int len = 0;

        unsigned char get_value[14994] = {0};

        // 从文件中读取数据
        len = read(fd, get_value, sizeof(get_value));
        for (int k = 0; k < len; k++)
        {
            re_pkg[i++] = get_value[k];
            // printf("re_pkg :%c \n",re_pkg[i-1]);
        }
        pkg_len += len;

        re_pkg[0] = 0xc0; // 包头
        re_pkg[i] = 0xc0; // 包尾
        pkg_len++;

        re_pkg[1] = pkg_len & 0xff;
        re_pkg[2] = (pkg_len >> 8) & 0xff;
        re_pkg[3] = (pkg_len >> 16) & 0xff;
        re_pkg[4] = (pkg_len >> 24) & 0xff;

        printf("pkg_len: %d\n", pkg_len);

        // 发送封包给服务器
        ret = sendto(sockfd, re_pkg, pkg_len, 0, (struct sockaddr *)&client, sizeof(client));
        if (ret == -1)
        {
            perror("client send error");
            return -1;
        }
        filesize -= len;
        if (filesize == 0)
        {
            break;
        }
        usleep(1000 * 10);
    }
    printf("已发送\n");
}

int make_pkg_ls(int sockfd)
{
    int ret;
    int pkg_len = 5; // 封包的大小
    char re_pkg[15000] = {0}; // 封包
    int i = 5;
    int len = 0;

    // 获取当前路径
    char now_path[128];
    getcwd(now_path, sizeof(now_path));
    printf("now_path: %s\n", now_path);

    // 打开当前路径
    DIR *file = opendir(now_path);
    if (!file)
    {
        perror("open file error");
        return -1;
    }

    struct dirent *dirp = NULL;
    char filename[512];
    while (dirp = readdir(file))
    {
        // 过滤掉当前目录和上级目录
        if (!strncmp(dirp->d_name, ".", sizeof(dirp->d_name)) || !strncmp(dirp->d_name, "..", sizeof(dirp->d_name)))
        {
            continue;
        }
        len = sprintf(filename, "%s ", dirp->d_name);
        pkg_len += len;
        for (int k = 0; k < len; k++)
        {
            re_pkg[i++] = filename[k];
            //printf("i : %d re_pkg[i-1] :%c\n",i,re_pkg[i-1]);
        }
        printf("%s\n", filename);
    }

    re_pkg[0] = 0xc0; // 包头
    re_pkg[i] = 0xc0; // 包尾
    pkg_len++;

    re_pkg[1] = pkg_len & 0xff;
    re_pkg[2] = (pkg_len >> 8) & 0xff;
    re_pkg[3] = (pkg_len >> 16) & 0xff;
    re_pkg[4] = (pkg_len >> 24) & 0xff;

    printf("pkg_len: %d\n", pkg_len);

    // 客户端IP和端口
    struct sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_port = htons(7777);
    client.sin_addr.s_addr = inet_addr("172.11.0.117");

    // 发送封包给服务器
    ret = sendto(sockfd, re_pkg, pkg_len, 0, (struct sockaddr *)&client, sizeof(client));
    if (ret == -1)
    {
        perror("client send error");
        return -1;
    }
}

int analysis_pkg(char *pkg,int sockfd)
{
    //初始化
    int ret;
    unsigned char ch;
    char buf[50] = {0};
    do//去掉杂乱数据
    {
        ret = recv(sockfd,&ch,1,0);
        if(ret == 0)
        {
            return -1;
        }
    }while(ch != 0xc0);
    //去包头
    while(ch == 0xc0)
    {
        recv(sockfd,&ch,1,0);
        //printf("ch:%c\n",ch);
    }
    //初始化
    unsigned char cmd[512] = {0};
    int i = 0;
    while(ch != 0xc0)
    {
        cmd[i++] = ch;
        recv(sockfd,&ch,1,0);
        //printf("ch:%c\n",ch);
    }
    //提取包大小
    int pkg_len = (cmd[0]&0xff) | ((cmd[1]&0xff) <<8) | ((cmd[2]&0xff) <<16) |((cmd[3]&0xff) <<24);
    if(pkg_len-2 != i)
    {
        printf("data error!\n");
        printf("pkg_len:%d i:%d\n",pkg_len,i);
        return -1;
    }
    //提取命令为ls
    if(!strncmp(&cmd[4],"ls",2))
    {
        ret = make_pkg_ls(sockfd);
        if(ret == -1)
        {
            printf("host make_pkg_ls error\n");
        }
    }//提取命令为get
    else if(!strncmp(&cmd[4],"get",3))
    {
        ret = make_pkg_get(sockfd,&cmd[8]);
        if(ret == -1)
        {
            printf("host make_pkg_ls error\n");
        }
    }//提取命令为cd
    else if(!strncmp(&cmd[4],"cd",2))
    {
        int i = 0;
        while((cmd[7+i] >= 'a' && cmd[7+i] <= 'z') || cmd[7+i] == '.' || (cmd[7+i] >= '0' && cmd[7+i] <= '9') || (cmd[7+i] >= 'A' && cmd[7+i] <= 'Z') || cmd[7+i] == '/' || cmd[7+i] == '-')
        {
            buf[i] = cmd[7+i];
            i++;
        }
        printf("buf:%s\n",buf);
        ret = chdir(buf);
        if(ret == -1)
        {
            perror("chdir error!");
        }
    }//提取命令为put
    else if(!strncmp(&cmd[4],"put",3))
    {
        printf("put start!\n");
        int i = 0;
        while((cmd[8+i] >= 'a' && cmd[8+i] <= 'z') || cmd[8+i] == '.' || (cmd[8+i] >= '0' && cmd[8+i] <= '9') || (cmd[8+i] >= 'A' && cmd[8+i] <= 'Z'))
        {
            buf[i] = cmd[8+i];
            i++;
        }
        //printf("buf:%s\n",buf);
        analysis_pkg_put(sockfd,buf);
    }//提取命令为remove
    else if(!strncmp(&cmd[4],"remove",6))
    {
        printf("remove start!\n");
        int i = 0;
        while((cmd[11+i] >= 'a' && cmd[11+i] <= 'z') || cmd[11+i] == '.' || (cmd[11+i] >= '0' && cmd[11+i] <= '9') || (cmd[11+i] >= 'A' && cmd[11+i] <= 'Z'))
        {
            buf[i] = cmd[11+i];
            i++;
        }
        printf("buf:%s\n",buf);
        ret = remove(buf);
    }
    printf("%s\n",&cmd[4]);
}

int main()
{
    int sockfd = socket(AF_INET,SOCK_STREAM,0);//创建一个套接字
    if(sockfd == -1)
    {
        perror("client socket error");
    }

    //主机ip 和 端口
    struct sockaddr_in host;
    host.sin_family = AF_INET;
    host.sin_port = htons(6698);
    host.sin_addr.s_addr = inet_addr("172.11.0.117");

    int ret = bind(sockfd, (struct sockaddr *)&host,sizeof(host));
    if(ret == -1)
    {
        perror("host bind error");
    }

    //进入监听模式，最多允许连接10台设备
    listen(sockfd,10);

    //初始化epoll相应代码
    int epfd = epoll_create(1);//初始化epoll，返回一个描述符

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;

    epoll_ctl(epfd,EPOLL_CTL_ADD,sockfd,&ev);
    struct epoll_event ep_ev[1024];
    char buf[50] = {0};
    while(1)
    {
        ret = epoll_wait(epfd,ep_ev,1024,-1);
        if(ret == 0)
        {
            printf("time out\n");
        }
        else if(ret > 0)
        {   
            for(int i = 0;i < ret;i++)
            {
                int fd = ep_ev[i].data.fd;
                if(fd == sockfd)
                {
                    //用来保存客户端的结构体
                    struct sockaddr_in client;
                    socklen_t len = sizeof(client);
                    struct epoll_event client_ep;

                    int cfd = accept(sockfd,(struct sockaddr *)&client,&len);
                    client_ep.data.fd = cfd;
                    client_ep.events = EPOLLIN;
                    ret = epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&client_ep);
                    if(ret == -1)
                    {
                        perror("epoll_ctl error");
                    }

                    printf("%s port %d new connection established\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));
                }
                else if(ep_ev[i].events & EPOLLIN)
                {
                    ret = analysis_pkg(buf,fd);
                    if(ret == -1)
                    {
                        epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
                        shutdown(fd,SHUT_RDWR);
                        continue;
                    }
                }
            }
        }
    }

    return 0;
}