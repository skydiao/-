# FTP文件传输项目说明文档:

​																																	作者:黄程远

#### 目录:

#### 1.项目功能说明



#### 2.项目设计过程



#### 3.设计遇到的问题和解决办法



#### 4.总结



## 项目功能说明

**1.   首先，FTP客户端通过TCP协议与主机端建立连接**

**2.   客户端进入命令界面，选择合适的命令对主机进行操作**

**3.   选择ls命令，显示主机端当前路径下的所有文件名并发送到客户端**

**4.   选择cd命令，切换到你指定的文件夹路径**

**5.   选择get命令，将你指定的文件下载到客户端中去**

**6.   选择remove命令，将你指定的主机端文件删除**

**7.   选择put命令，将你指定的客户端文件上传到主机端的当前文件夹中**

**8.   选择quit命令，退出客户端的控制台并且断开与主机端的连接**





## 项目设计流程

**1.   首先画出了思维导图，确定了项目的难点和必须的功能，并且创建了相应的文件**

![1695089089077](C:\Users\ok\AppData\Roaming\Typora\typora-user-images\1695089089077.png)

**2.   编写TCP的初始化函数，并将其封装起来**

```c
//创建客户端套接字，完成连接主机的功能，返回套接字
int creat_fd()
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

    //客户端ip 和 端口
    struct sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_port = htons(7788);
    client.sin_addr.s_addr = inet_addr("172.11.0.117");

    int optval = 0;
    int len = sizeof(optval);
    int ret = bind(sockfd, (struct sockaddr *)&client,sizeof(client));
    if(ret == -1)
    {
        perror("client bind error");
    }

    ret = connect(sockfd,(struct sockaddr *)&host,sizeof(host));
    if(-1 == ret)
    {
        perror("client connet error");
    }

    return sockfd;
}
```

**3.   正式开始项目功能的搭建，首先完成主机端和客户端的连接**

![1695089305005](C:\Users\ok\AppData\Roaming\Typora\typora-user-images\1695089305005.png)

**4.   开始封包功能的编写和测试**

```c
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
```

**5.   开始ls功能的编写和测试**

```c
int make_pkg_ls(int sockfd)
{
    //以下是封包初始化
    int ret;
    int pkg_len = 5;//封包的大小
    char re_pkg[15000] = {0};//封包
    int i = 5;
    //int j = 0;
    int len = 0;

    DIR *file = NULL;
    char now_path[128];
    getcwd(now_path,sizeof(now_path));
    printf("now_path:%s\n",now_path);
    file = opendir(now_path);
    if(!file)
    {
        perror("open file error");
        return -1;
    }
    struct dirent *dirp = NULL;
    char filename[512];
    while(dirp = readdir(file))
    {
        if(!strncmp(dirp->d_name,".",sizeof(dirp->d_name)) || !strncmp(dirp->d_name,"..",sizeof(dirp->d_name)))
        {
            continue;
        }
        len = sprintf(filename,"%s ",dirp->d_name);
        pkg_len += len;
        for(int k = 0;k<len;k++)
        {
            re_pkg[i++] = filename[k];
            //printf("i : %d re_pkg[i-1] :%c\n",i,re_pkg[i-1]);
        }
        printf("%s\n",filename);
    }

    re_pkg[0] = 0xc0;//包头
    re_pkg[i] = 0xc0;//包尾
    pkg_len++;

    re_pkg[1] = pkg_len & 0xff;
    re_pkg[2] = (pkg_len >> 8) & 0xff;
    re_pkg[3] = (pkg_len >> 16) & 0xff;
    re_pkg[4] = (pkg_len >> 24) & 0xff;
    

    printf("pkg_len: %d\n",pkg_len);

    //客户端ip 和 端口
    struct sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_port = htons(7777);
    client.sin_addr.s_addr = inet_addr("172.11.0.117");

    ret = sendto(sockfd,re_pkg,pkg_len,0,(struct sockaddr *)&client,sizeof(client));
    if(ret == -1)
    {
        perror("client send error");
        return -1;
    }
}
```

![1695089656107](C:\Users\ok\AppData\Roaming\Typora\typora-user-images\1695089656107.png)

**6.   开始get功能的编写**

**这里get支持几乎所有类型文件，包括图片，.c , .h 可执行文件等**

```c
int make_pkg_get(int sockfd,char *buf)
{
    char filename[512] = {0};
    int i = 0;
    while(buf[i] != '\n' && buf[i] != '\0')
    {
        filename[i] = buf[i];
        i++;
    }
    printf("filename :%s size:%ld\n",filename,strlen(filename));
    //以下是封包初始化
    int ret;
    int pkg_len = 5;//封包的大小
    char re_pkg[15000] = {0};//封包
    i = 5;
    //int j = 0;
    int len = 0;

    int fd;
    fd = open(filename,O_RDONLY);
    if(fd == -1)
    {
        perror("open error");
    }
    unsigned char get_value[4096];
    while(1)
    {
        len = read(fd,get_value,sizeof(get_value));
        if(len == 0)
        {
            break;
        }
        else if(len > 0)
        {
            for(int k = 0;k<len;k++)
            {
                re_pkg[i++] = get_value[k];
                //printf("re_pkg :%c \n",re_pkg[i-1]);
            }
        }
        pkg_len += len;
    }

    re_pkg[0] = 0xc0;//包头
    re_pkg[i] = 0xc0;//包尾
    pkg_len++;

    re_pkg[1] = pkg_len & 0xff;
    re_pkg[2] = (pkg_len >> 8) & 0xff;
    re_pkg[3] = (pkg_len >> 16) & 0xff;
    re_pkg[4] = (pkg_len >> 24) & 0xff;
    

    printf("pkg_len: %d\n",pkg_len);

    //客户端ip 和 端口
    struct sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_port = htons(7777);
    client.sin_addr.s_addr = inet_addr("172.11.0.117");

    ret = sendto(sockfd,re_pkg,pkg_len,0,(struct sockaddr *)&client,sizeof(client));
    if(ret == -1)
    {
        perror("client send error");
        return -1;
    }
    printf("已发送: %d\n",ret);
}
```

**7.   在get的基础上添加了put功能，代码类似**

**8.   添加了quit功能，这里直接断开与主机端的连接再调用exit()函数即可**

**9.   添加了remove功能，这里直接调用了remove函数，可以直接删除指定文件**

**10.   填了cd功能，这里直接调用了chdir函数，可以直接切换到指定的文件路径**

**11.   将函数都完成封装，整合到一起，至此项目基本完成编写**

![1695090088894](C:\Users\ok\AppData\Roaming\Typora\typora-user-images\1695090088894.png)





## 问题和解决办法

**1   在封包解包的时候，我发现解包后的数据和客户端发来的数据有出入。**

**解决办法:   经过自我排查和上网翻阅资源，我发现我判断包头包尾的方式出现了问题，我直接用ch==包尾来判断，这样是不对的，我改变了判断方式就好了**

**2   在发送命令给主机端申请下载文件，总是提示命令错误或者无此文件夹**

**解决办法:   通过打印和strcmp函数我发现我发过去的字符串带有\n \0等字符，所以在主机端接收到命令后，我对命令做了处理，剔除了多余了字符**





## 总结

**1   做项目前做好规划，写好思维导图，一步一步完成功能，完成某个功能后要多次测试，避免出错**

**2   每完成一次更新，都要对这次更新进行一次备份，以免下次更新后产生无法挽回的错误**