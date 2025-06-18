#pragma once
#include <stdbool.h>
#include "Dispatcher.h"
#include "ChannelMap.h"
#include <pthread.h>

extern struct Dispatcher EpollDispatcher;
extern struct Dispatcher PollDispatcher;
extern struct Dispatcher SelectDispatcher;

// 处理该节点中的channel的方式
enum ElemType{ADD, DELETE, MODIFY};
// 定义任务队列的节点
struct ChannelElement
{
    int type;   // 如何处理该节点中的channel  type描绘要对前面的channel(还是当前channel的？)：三件事：添加一个新的节点、 删除、修改事件
    struct Channel* channel;
    struct ChannelElement* next;
};
struct Dispatcher;
struct EventLoop
{
    bool isQuit;
    struct Dispatcher* dispatcher;
    void* dispatcherData;
    // 任务队列
    struct ChannelElement* head;
    struct ChannelElement* tail;
    // map
    struct ChannelMap* channelMap;
    // 线程id, name, mutex
    pthread_t threadID;
    char threadName[32];
    pthread_mutex_t mutex; //用来保护任务队列的
    int socketPair[2];  // 存储本地通信的fd 通过socketpair 初始化
};

// 初始化
struct EventLoop* eventLoopInit();//主线程 有特殊的名字 
struct EventLoop* eventLoopInitEx(const char* threadName); //子线程的    在c语言中没有函数的重载
// 启动反应堆模型 需要告诉启动的是谁
int eventLoopRun(struct EventLoop* evLoop);
// 处理激活的文件fd    event是看是读事件还是写事件
int eventActivate(struct EventLoop* evLoop, int fd, int event);
// 添加任务到任务队列
int eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type);
// 处理任务队列中的任务
int eventLoopProcessTask(struct EventLoop* evLoop);
// 处理dispatcher中的节点
int eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel);
int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel);
int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel);
// 释放channel
int destroyChannel(struct EventLoop* evLoop, struct Channel* channel);

