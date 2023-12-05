#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <condition_variable>
#include <iostream>

using namespace std;
using callback = void(*)(void *);

// 任务队列类
// 成员函数介绍：参数1：任务执行函数；参数2：任务执行函数的参数
class Task
{
public:
    callback function;
    void* arg;
public:
    Task(callback f, void* arg)
    { 
        function = f; 
        this->arg = arg; 
    }
    ~Task()
    {
        cout << "任务完成" << endl;
    }
};


// 线程池类
class ThreadPool
{
public:
    ThreadPool(int min, int max);
    // 添加任务
    void Add(callback f, void* arg);
    void Add(Task task);
    // 忙线程个数
    int Busynum();
    // 存活线程个数
    int Alivenum();
    ~ThreadPool();

private:
    // 任务队列
    queue<Task> taskQ;
    thread managerID;   //管理者线程ID
    vector<thread> threadIDs;   //
    int minNum;   //最小线程数
    int maxNum;   //最大线程数
    int busyNum;   //忙的线程数
    int liveNum;    //存活的线程数
    int exitNum;    //要销毁的线程数

    mutex mutexPool;    //整个线程池的锁
    condition_variable cond;//任务队列是否为空,阻塞工作者线程
    bool shutdown;    //是否销毁线程池，销毁为1，不销毁为0
    static void manager(void* arg);   //管理者线程
    static void worker(void* arg);   //工作线程
};

#endif
