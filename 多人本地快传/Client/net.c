#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <linux/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

int make_pkg_put(int sockfd, char *buf)
{
    char filename[512] = {0}; // 存储文件名的数组
    int i = 0;
    while (buf[i] != '\n' && buf[i] != '\0')
    {
        filename[i] = buf[i]; // 逐个字符读取文件名
        i++;
    }
    printf("filename :%s size:%ld\n", filename, strlen(filename));

    int ret;
    int pkg_len = 5; // 封包的大小
    char re_pkg[15000] = {0}; // 封包数组
    i = 5;
    int len = 0;

    int fd;
    fd = open(filename, O_RDONLY); // 打开文件
    if (fd == -1)
    {
        perror("open error");
    }
    unsigned char get_value[4096]; // 存储从文件中读取的数据
    while (1)
    {
        len = read(fd, get_value, sizeof(get_value)); // 从文件中读取数据
        if (len == 0)
        {
            break;
        }
        else if (len > 0)
        {
            for (int k = 0; k < len; k++)
            {
                re_pkg[i++] = get_value[k]; // 将读取的数据存入封包数组中
            }
        }
        pkg_len += len; // 更新封包的大小
    }

    re_pkg[0] = 0xc0; // 包头
    re_pkg[i] = 0xc0; // 包尾
    pkg_len++;

    re_pkg[1] = pkg_len & 0xff; // 封包大小的低字节
    re_pkg[2] = (pkg_len >> 8) & 0xff; // 封包大小的次低字节
    re_pkg[3] = (pkg_len >> 16) & 0xff; // 封包大小的次高字节
    re_pkg[4] = (pkg_len >> 24) & 0xff; // 封包大小的高字节

    printf("pkg_len: %d\n", pkg_len);

    // 客户端ip 和 端口
    struct sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_port = htons(6698);
    client.sin_addr.s_addr = inet_addr("172.11.0.117");

    ret = sendto(sockfd, re_pkg, pkg_len, 0, (struct sockaddr *)&client, sizeof(client)); // 发送封包
    if (ret == -1)
    {
        perror("client send error");
        return -1;
    }
    printf("已发送: %d\n", ret);
}

int analysis_pkg(int sockfd)
{
    int ret;
    unsigned char ch;
    char buf[50];

    do
    {
        // 从套接字接收一个字节的数据
        ret = recvfrom(sockfd, &ch, 1, 0, NULL, NULL);
        if (ret == 0)
        {
            return -1;
        }
    } while (ch != 0xc0);

    // 跳过连续的包头
    while (ch == 0xc0)
    {
        recvfrom(sockfd, &ch, 1, 0, NULL, NULL);
    }

    unsigned char cmd[512] = {0};
    int i = 0;
    // 读取封包数据直到遇到包尾
    while (ch != 0xc0)
    {
        cmd[i++] = ch;
        recvfrom(sockfd, &ch, 1, 0, NULL, NULL);
    }

    // 解析封包大小
    int pkg_len = (cmd[0] & 0xff) | ((cmd[1] & 0xff) << 8) | ((cmd[2] & 0xff) << 16) | ((cmd[3] & 0xff) << 24);
    // 检查数据是否完整
    if (pkg_len - 2 != i)
    {
        printf("data error!\n");
        printf("pkg_len:%d i:%d\n", pkg_len, i);
        return -1;
    }
    // 打印封包数据
    printf("%s\n", &cmd[4]);
}

int analysis_pkg_get(int sockfd, char *filename)
{
    // 设置接收超时时间
    // struct timeval timeout;
    // timeout.tv_sec = 2;
    // timeout.tv_usec = 0;
    // setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    int ret;
    unsigned char ch;
    char buf[14998];

    // 打印文件名和大小
    printf("filename: %s size: %ld\n", filename, strlen(filename));

    // 打开文件
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1)
    {
        perror("open error");
    }

    unsigned long int filesize = 0;

    while (1)
    {
        do
        {
            // 从套接字接收一个字节的数据
            // recvfrom(sockfd, &ch, 1, 0, NULL, NULL);
            ret = recvfrom(sockfd, &ch, 1, 0, NULL, NULL);
            if (ret == 0)
            {
                return -1;
            }
        } while (ch != 0xc0);

        // 跳过连续的包头
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

        // 接收封包数据
        ret = recvfrom(sockfd, buf, 14995, 0, NULL, NULL);
        filesize += ret - 1;
        printf("ret = %d\n", ret);
        if (ret == -1)
        {
            break;
        }
        i += ret;

        // 写入文件
        if (buf[ret - 1] != -64)
        {
            printf("ok\n");
            write(fd, buf, ret);
        }
        else if (buf[ret - 1] == -64)
        {
            write(fd, buf, ret - 1);
        }

        // 解析封包大小
        int pkg_len = (cmd[0] & 0xff) | ((cmd[1] & 0xff) << 8) | ((cmd[2] & 0xff) << 16) | ((cmd[3] & 0xff) << 24);
        if (pkg_len - 1 != i)
        {
            printf("pkg_len error\n");
        }
        printf("pkg_len: %d i: %d filesize: %ld\n", pkg_len, i, filesize);
        if (pkg_len < 15000)
        {
            break;
        }
    }

    //printf("%s\n",&cmd[4]);
}

//创建客户端套接字，完成连接主机的功能，返回套接字
/**
 * @brief 创建套接字并连接到服务器
 * 
 * @return int 返回套接字描述符
 */
int creat_fd()
{
    // 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("client socket error");
    }

    // 设置服务器地址
    struct sockaddr_in host;
    host.sin_family = AF_INET;
    host.sin_port = htons(6698);
    host.sin_addr.s_addr = inet_addr("172.11.0.117");

    // 设置客户端地址
    struct sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_port = htons(7788);
    client.sin_addr.s_addr = inet_addr("172.11.0.117");

    int optval = 0;
    int len = sizeof(optval);

    // 绑定客户端地址
    int ret = bind(sockfd, (struct sockaddr *)&client, sizeof(client));
    if (ret == -1)
    {
        perror("client bind error");
    }

    // 连接到服务器
    ret = connect(sockfd, (struct sockaddr *)&host, sizeof(host));
    if (-1 == ret)
    {
        perror("client connet error");
    }

    return sockfd;
}

/*
    @buf:要发送的命令
    @sockfd:客户端套接字
*/
void connect_client(char *buf,int sockfd)
{
    // int sockfd = creat_fd();//测试用
    int ret;
    // char buf[4096] = {0};//数据包
    int pkg_len = 5;//封包的大小
    char re_pkg[15000] = {0};//封包
    int i = 5;
    int j = 0;
    int len = strlen(buf);

    pkg_len+=len;
    for(i;i<pkg_len;i++)
    {
        //printf("buf[%d]:%c\n",j,buf[j]);
        re_pkg[i] = buf[j++];
    }

    re_pkg[0] = 0xc0;//包头
    re_pkg[i] = 0xc0;//包尾
    pkg_len++;

    re_pkg[1] = pkg_len & 0xff;
    re_pkg[2] = (pkg_len >> 8) & 0xff;
    re_pkg[3] = (pkg_len >> 16) & 0xff;
    re_pkg[4] = (pkg_len >> 24) & 0xff;

    //printf("pkg_len: %d\n",pkg_len);
    ret = send(sockfd,re_pkg,pkg_len,0);
    if(ret == -1)
    {
        perror("client send error");
    }

}
