#include "connect.hpp"
using namespace std;
//制作封包 关于ls命令的
void make_pkg_ls(void* arg)
{
    int sockfd = *(int *)arg;
    int ret;
    int pkg_len = 5; // 封包的大小
    char re_pkg[15000] = {0}; // 封包
    int i = 5;
    int len = 0;

    // 获取当前路径
    char now_path[128];
    getcwd(now_path, sizeof(now_path));
    cout << "now_path: " << now_path << endl;
    //printf("now_path: %s\n", now_path);

    // 打开当前路径
    DIR *file = opendir(now_path);
    if (!file)
    {
        perror("open file error");
        cout << "host make_pkg_ls error\n" << endl;
        return;
    }

    struct dirent *dirp = NULL;
    char filename[512];
    while ((dirp = readdir(file)))
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

    //printf("pkg_len: %d\n", pkg_len);
    cout << "pkg_len: " << pkg_len << endl;
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
        cout << "host make_pkg_ls error\n" << endl;
        return;
    }
}
//制作封包 关于get命令的
void make_pkg_get(void* arg)
{
    Data_get *Get = (Data_get *)arg;
    int sockfd = Get->sockfd;
    unsigned char *buf = Get->file;
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
        cout << "host make_pkg_get error\n" << endl;
    }
    cout << "filename : " << filename << " size : " << statbuf.st_size << endl;
    unsigned long int filesize = statbuf.st_size;

    int fd;
    fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        perror("open error");
        cout << "host make_pkg_get error\n" << endl;
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
        }
        pkg_len += len;

        re_pkg[0] = 0xc0; // 包头
        re_pkg[i] = 0xc0; // 包尾
        pkg_len++;

        re_pkg[1] = pkg_len & 0xff;
        re_pkg[2] = (pkg_len >> 8) & 0xff;
        re_pkg[3] = (pkg_len >> 16) & 0xff;
        re_pkg[4] = (pkg_len >> 24) & 0xff;

        cout << "pkg_len: " << pkg_len << endl;

        // 发送封包给服务器
        ret = sendto(sockfd, re_pkg, pkg_len, 0, (struct sockaddr *)&client, sizeof(client));
        if (ret == -1)
        {
            perror("client send error");
            cout << "host make_pkg_get error\n" << endl;
            return;
        }
        filesize -= len;
        if (filesize == 0)
        {
            break;
        }
        usleep(1000 * 10);
    }
    cout << "已发送" << endl;
}
//解析封包
int Connect::analysis_pkg(ThreadPool & pool,int sockfd)
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
    }
    //初始化
    unsigned char cmd[512] = {0};
    int i = 0;
    while(ch != 0xc0)
    {
        cmd[i++] = ch;
        recv(sockfd,&ch,1,0);
    }
    //提取包大小
    int pkg_len = (cmd[0]&0xff) | ((cmd[1]&0xff) <<8) | ((cmd[2]&0xff) <<16) |((cmd[3]&0xff) <<24);
    if(pkg_len-2 != i)
    {
        cout << "data error!" << endl;
        cout << "pkg_len: " << pkg_len << " real_len: " << i << endl;
        return -1;
    }
    //提取命令为ls
    if(!strncmp((const char *)&cmd[4],"ls",2))
    {
        pool.Add(make_pkg_ls,(void*)&sockfd);
    }//提取命令为get
    else if(!strncmp((const char *)&cmd[4],"get",3))
    {
        Data_get Get;
        Get.sockfd = sockfd;
        Get.file = &cmd[8];
        pool.Add(make_pkg_get,(void*)&Get);
    }//提取命令为cd
    else if(!strncmp((const char *)&cmd[4],"cd",2))
    {
        int i = 0;
        while((cmd[7+i] >= 'a' && cmd[7+i] <= 'z') || cmd[7+i] == '.' || (cmd[7+i] >= '0' && cmd[7+i] <= '9') || (cmd[7+i] >= 'A' && cmd[7+i] <= 'Z') || cmd[7+i] == '/' || cmd[7+i] == '-')
        {
            buf[i] = cmd[7+i];
            i++;
        }
        cout << "buf: " << buf << endl;
        ret = chdir(buf);
        if(ret == -1)
        {
            perror("chdir error!");
        }
    }//提取命令为put
    else if(!strncmp((const char *)&cmd[4],"remove",6))
    {
        cout << "remove start!" << endl;
        int i = 0;
        while((cmd[11+i] >= 'a' && cmd[11+i] <= 'z') || cmd[11+i] == '.' || (cmd[11+i] >= '0' && cmd[11+i] <= '9') || (cmd[11+i] >= 'A' && cmd[11+i] <= 'Z'))
        {
            buf[i] = cmd[11+i];
            i++;
        }
        cout << "buf: " << buf << endl;
        ret = remove(buf);
    }
    cout << &cmd[4] << endl;
    return 1;
}
//构造函数 连接
Connect::Connect()
{
    sockfd = socket(AF_INET,SOCK_STREAM,0);//创建一个套接字
    if(sockfd == -1)
    {
        perror("client socket error");
    }

    //主机ip 和 端口
    struct sockaddr_in host;
    host.sin_family = AF_INET;
    host.sin_port = htons(host_port);
    host.sin_addr.s_addr = inet_addr(host_addr);

    int ret = bind(sockfd, (struct sockaddr *)&host,sizeof(host));
    if(ret == -1)
    {
        perror("host bind error");
    }

    //进入监听模式，最多允许连接20台设备
    listen(sockfd,10);

    //初始化epoll相应代码
    epfd = epoll_create(1);//初始化epoll，返回一个描述符
    if(epfd == -1)
    {
        perror("epoll_create error: ");
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;

    ret = epoll_ctl(epfd,EPOLL_CTL_ADD,sockfd,&ev);
    if(ret == -1)
    {
        perror("epoll_ctl error: ");
    }
    else 
    {
        cout << "host init success" <<endl;
    }
}
//启用epoll
void Connect::epoll_mode(ThreadPool & pool)//进入epoll模式 开始连接并且管理设备
{
    int ret = 0;
    while(1)
    {
        ret = epoll_wait(epfd,ep_ev,1024,-1);
        if(ret == 0)
        {
            cout << "time out" << endl;
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

                    cout << "addr: " <<  inet_ntoa(client.sin_addr) << " port: " << ntohs(client.sin_port) << " new connection established" << endl;
                }
                else if(ep_ev[i].events & EPOLLIN)//收到数据了
                {
                    ret = analysis_pkg(pool,fd);//处理数据
                    if(ret == -1)//如果返回值为-1,说明已经断开连接,则删除该套接字
                    {
                        epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
                        shutdown(fd,SHUT_RDWR);
                        continue;
                    }
                }
            }
        }
        else if(ret < 0)
        {
            perror("epoll wait error: ");
            return;
        }
    }
}