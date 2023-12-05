#ifndef __CONNECT_H__
#define __CONNECT_H__

#include <string>
#include <iostream>
using namespace std;

#include <string>
#include <iostream>

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
#include <iostream>
#include <string>

#define host_port 6698 //服务端的端口
#define host_addr "172.11.0.117" //服务端的地址

class Connect
{
private:

unsigned int port; //客户端的端口
string addr; //客户端的地址
int sockfd;

public:
Connect(unsigned int port_ = 0011,string addr_ = "172.11.0.117");

void send_orders(char *buf); //将输入的命令制作成封包发送出去

void make_orders(); //循环输入命令

int analysis_pkg(int sockfd); //解析发送过来的封包 处理ls后发来的信息

int analysis_pkg_get(int sockfd,char *filename);//解析发送过来的封包 专门处理大型文件

};

















#endif