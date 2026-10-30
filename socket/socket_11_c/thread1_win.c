/*
 * 本文件用途：线程基础：让多个任务同时运行并观察共享数据。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


// thread1_win.c
#include <stdio.h>
#include <windows.h>
#include <process.h>   // _beginthreadex, _endthreadex

unsigned WINAPI ThreadFunc(void *arg);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    HANDLE hThread;
    unsigned threadID;
    int param = 5;

    // 创建子线程
    hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadFunc, (void*)&param, 0, &threadID);

    if(hThread == 0)
    {
        // 创建一个新的执行任务，让这部分工作可以和主任务同时进行。
        puts("_beginthreadex() error");
        return -1;
    }

    Sleep(3000);
    puts("end of main");
    return 0;
}

// 子线程入口函数
unsigned WINAPI ThreadFunc(void *arg)
{
    int i;
    int cnt = *((int*)arg);   // 读取传入参数
    // 重复处理任务，直到程序结束或满足退出条件。
    for(i = 0; i < cnt; i++)
    {
        Sleep(1000);
        puts("running thread");
    }
    return 0;
}