#include "connect.hpp"
#include <iostream>
#include "threadpool.hpp"
#include <stdlib.h>
using namespace std;

int main()
{
    // 设置线程池最小5个线程，最大10个线程
    ThreadPool pool(5, 10);
    Connect host;
    //开始运行
    host.epoll_mode(pool);
    return 0;
}
