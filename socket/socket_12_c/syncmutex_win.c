/*
 * 本文件用途：线程同步：使用临界区、事件、互斥量和信号量协调任务。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <windows.h>
#include <process.h>

#define NUM_THREAD  50
unsigned WINAPI threadInc(void * arg);
unsigned WINAPI threadDes(void * arg);

long long num = 0;
HANDLE hMutex;    // ✅互斥量句柄，Mutex是Windows内核对象

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    HANDLE tHandles[NUM_THREAD];
    int i;

    // 创建互斥量
    hMutex = CreateMutex(NULL, FALSE, NULL);

    // 重复处理任务，直到程序结束或满足退出条件。
    for(i = 0; i < NUM_THREAD; i++)
    {
        if(i % 2)
            // 创建一个新的执行任务，让这部分工作可以和主任务同时进行。
            tHandles[i] = (HANDLE)_beginthreadex(NULL, 0, threadInc, NULL, 0, NULL);
        else
            // 创建一个新的执行任务，让这部分工作可以和主任务同时进行。
            tHandles[i] = (HANDLE)_beginthreadex(NULL, 0, threadDes, NULL, 0, NULL);
    }

    // 等待全部50个子线程执行完毕
    WaitForMultipleObjects(NUM_THREAD, tHandles, TRUE, INFINITE);

    // 关闭 Windows 句柄，释放它代表的系统资源。
    CloseHandle(hMutex);    // 关闭内核对象句柄
    printf("result: %lld \n", num);
    return 0;
}

// 递增线程
unsigned WINAPI threadInc(void * arg)
{
    int i;
    // ✅申请互斥锁：等待互斥量变为有信号状态
    WaitForSingleObject(hMutex, INFINITE);
    // 重复处理任务，直到程序结束或满足退出条件。
    for(i = 0; i < 50000000; i++)
    {
        num += 1;
    }
    ReleaseMutex(hMutex);    // ✅释放互斥锁
    return 0;
}

// 递减线程
unsigned WINAPI threadDes(void * arg)
{
    int i;
    // 等待另一个任务或同步对象发出“可以继续”的通知。
    WaitForSingleObject(hMutex, INFINITE);
    // 重复处理任务，直到程序结束或满足退出条件。
    for(i = 0; i < 50000000; i++)
    {
        num -= 1;
    }
    ReleaseMutex(hMutex);
    return 0;
}