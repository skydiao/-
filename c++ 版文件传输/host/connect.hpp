#ifndef __CONNECT_H__
#define __CONNECT_H__

#include "ThreadPool.hpp"

#include <string>
#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

using namespace std;

#define host_port 6698 //服务端的端口
#define host_addr "172.11.0.117" //服务端的地址
//这里用结构体保存get命令需要的变量
typedef struct
{
    int sockfd;
    unsigned char *file;
}Data_get;

class Connect
{
private:

int sockfd;//客户端的套接字
int epfd;
struct epoll_event ep_ev[1024];//用来保存客户端数据的结构体

public:
Connect();

void epoll_mode(ThreadPool & pool);//进入epoll模式 开始连接并且管理设备

int analysis_pkg(ThreadPool & pool,int sockfd);//处理发送过来的封包
};


#endif