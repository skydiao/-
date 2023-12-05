#include "connect.hpp"
using namespace std;

Connect::Connect(unsigned int port_,string addr_):port(port_),addr(addr_)
{
    // 创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("client socket error");
    }

    // 设置服务器地址
    struct sockaddr_in host;
    host.sin_family = AF_INET;
    host.sin_port = htons(host_port);
    host.sin_addr.s_addr = inet_addr(host_addr);

    // 设置客户端地址
    struct sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_port = htons(port);
    client.sin_addr.s_addr = inet_addr(addr.c_str());

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
}

/*
    @buf:要发送的命令
    @sockfd:客户端套接字
*/
void Connect::send_orders(char *buf)
{
    int ret;
    int pkg_len = 5;//封包的大小
    char re_pkg[15000] = {0};//封包
    int i = 5;
    int j = 0;
    int len = strlen(buf);

    pkg_len+=len;
    for(;i<pkg_len;i++)
    {
        re_pkg[i] = buf[j++];
    }

    re_pkg[0] = 0xc0;//包头
    re_pkg[i] = 0xc0;//包尾
    pkg_len++;

    re_pkg[1] = pkg_len & 0xff;
    re_pkg[2] = (pkg_len >> 8) & 0xff;
    re_pkg[3] = (pkg_len >> 16) & 0xff;
    re_pkg[4] = (pkg_len >> 24) & 0xff;

    ret = send(sockfd,re_pkg,pkg_len,0);
    if(ret == -1)
    {
        perror("client send error");
    }
}

void Connect::make_orders()
{
    int ret;
    while(1)
    {
        cout << "输入你的命令: " << endl;
        char buf[50] = {0};
        fgets(buf,50,stdin);
        if(!strncmp(buf,"ls",2))
        {
            send_orders(buf);
            ret = analysis_pkg(sockfd);
            if(ret == -1)
            {
                cout << "client analysis_pkg error" << endl;
            }
        }
        else if(!strncmp(buf,"get",3))
        {
            send_orders(buf);
            char filename[512];
            int i = 0;
            while((buf[4+i] >= 'a' && buf[4+i] <= 'z') || buf[4+i] == '.' || (buf[4+i] >= '0' && buf[4+i] <= '9') || (buf[4+i] >= 'A' && buf[4+i] <= 'Z'))
            {
                filename[i] = buf[4+i];
                i++;
            }
            ret = analysis_pkg_get(sockfd,filename);
            if(ret == -1)
            {
                cout << "client analysis_pkg_get error" << endl;
            }
        }
        else if(!strncmp(buf,"cd",2))
        {
            send_orders(buf);
        }
        else if(!strncmp(buf,"exit",4))
        {
            exit(0);
        }
        else if(!strncmp(buf,"remove",6))
        {
            send_orders(buf);
        }
    }
}

int Connect::analysis_pkg(int sockfd) //解析发送过来的封包
{   
    int ret;
    unsigned char ch;
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
        cout << "data error!" << endl;
        cout << "pkg_len: " << pkg_len << " i: " << i << endl;
        //printf("pkg_len:%d i:%d\n", pkg_len, i);
        return -1;
    }
    // 打印封包数据
    cout << &cmd[4] << endl;
    //printf("%s\n", &cmd[4]);
    return 1;
}

int Connect::analysis_pkg_get(int sockfd,char *filename)//解析发送过来的封包 专门处理大型文件
{
    int ret;
    unsigned char ch;
    char buf[14998];

    // 打印文件名和大小
    cout << "filename: " << filename << " size: " << strlen(filename) << endl;
    //printf("filename: %s size: %ld\n", filename, strlen(filename));

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
        //printf("ret = %d\n", ret);
        cout << " ret = " << ret << endl;
        if (ret == -1)
        {
            break;
        }
        i += ret;

        // 写入文件
        if (buf[ret - 1] != -64)
        {
            //printf("ok\n");
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
            cout << "pkg_len error" << endl;
           // printf("pkg_len error\n");
        }
        cout << "pkg_len: " << pkg_len << " i: " << i << " filesize: " << filesize << endl;
        //printf("pkg_len: %d i: %d filesize: %ld\n", pkg_len, i, filesize);
        if (pkg_len < 15000)
        {
            break;
        }
    }

    //printf("%s\n",&cmd[4]);

    return 1;
}





