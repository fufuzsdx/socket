/*
 * 本文件用途：线程基础：让多个任务同时运行并观察共享数据。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <windows.h>
#include <process.h>

#define NUM_THREAD   50
unsigned WINAPI threadInc(void * arg);
unsigned WINAPI threadDes(void * arg);

long long num = 0;   // 全局共享变量，临界资源

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    HANDLE tHandles[NUM_THREAD];
    int i;

    printf("sizeof long long: %d \n", sizeof(long long));

    // 循环创建50个线程
    for(i = 0; i < NUM_THREAD; i++)
    {
        if(i % 2)
            // 创建一个新的执行任务，让这部分工作可以和主任务同时进行。
            tHandles[i] = (HANDLE)_beginthreadex(NULL, 0, threadInc, NULL, 0, NULL);
        else
            // 创建一个新的执行任务，让这部分工作可以和主任务同时进行。
            tHandles[i] = (HANDLE)_beginthreadex(NULL, 0, threadDes, NULL, 0, NULL);
    }

    // ⭐等待全部50个线程执行完毕
    WaitForMultipleObjects(NUM_THREAD, tHandles, TRUE, INFINITE);

    printf("result: %lld \n", num);
    return 0;
}

// 递增线程：num = num +1
unsigned WINAPI threadInc(void * arg)
{
    int i;
    // 重复处理任务，直到程序结束或满足退出条件。
    for(i = 0; i < 50000000; i++)
    {
        num += 1;
    }
    return 0;
}

// 递减线程：num = num -1
unsigned WINAPI threadDes(void * arg)
{
    int i;
    // 重复处理任务，直到程序结束或满足退出条件。
    for(i = 0; i < 50000000; i++)
    {
        num -= 1;
    }
    return 0;
}