#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net.h"

int main()
{
    int ret = 0;
    int sockfd = creat_fd();
    while(1)
    {
        printf("输入你的命令: ");
        char buf[500] = {0};
        //scanf("%s",buf);
        fgets(buf,50,stdin);
        //printf("buf :%s\n",buf);
        if(!strncmp(buf,"ls",2))
        {
            connect_client(buf,sockfd);
            ret = analysis_pkg(sockfd);
            if(ret == -1)
            {
                printf("client analysis_pkg error\n");
            }
        }
        else if(!strncmp(buf,"get",3))
        {
            connect_client(buf,sockfd);
            char filename[512];
            int i = 0;
            while((buf[4+i] >= 'a' && buf[4+i] <= 'z') || buf[4+i] == '.' || (buf[4+i] >= '0' && buf[4+i] <= '9'))
            {
                filename[i] = buf[4+i];
                i++;
            }
            //printf("filename: %s\n",filename);
            ret = analysis_pkg_get(sockfd,filename);
            if(ret == -1)
            {
                printf("client analysis_pkg_get error\n");
            }
        }
        else if(!strncmp(buf,"cd",2))
        {
            connect_client(buf,sockfd);
        }
        else if(!strncmp(buf,"put",3))
        {
            connect_client(buf,sockfd);
            //printf("ok\n");
            make_pkg_put(sockfd,&buf[4]);
        }
        else if(!strncmp(buf,"exit",4))
        {
            exit(0);
        }
        else if(!strncmp(buf,"remove",6))
        {
            connect_client(buf,sockfd);
        }
    }
    return 0;
}