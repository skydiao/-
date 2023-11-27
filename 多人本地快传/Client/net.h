#ifndef __NET_H__
#define __NET_H__

void connect_client(char *buf,int sockfd);

int creat_fd();

int analysis_pkg(int sockfd);

int analysis_pkg_get(int sockfd,char *filename);

int make_pkg_put(int sockfd,char *buf);

#endif