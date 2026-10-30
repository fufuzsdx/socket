/*
 * 本文件用途：线程同步：使用临界区、事件、互斥量和信号量协调任务。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <windows.h>
#include <process.h>

unsigned WINAPI Read(void * arg);
unsigned WINAPI Accu(void * arg);

static HANDLE semOne;
static HANDLE semTwo;
static int num;

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    HANDLE hThread1, hThread2;
    // 创建信号量
    semOne = CreateSemaphore(NULL, 0, 1, NULL);
    // 创建计数信号；用数量表示还允许多少个任务进入。
    semTwo = CreateSemaphore(NULL, 1, 1, NULL);

    // 创建一个新的执行任务，让这部分工作可以和主任务同时进行。
    hThread1 = (HANDLE)_beginthreadex(NULL, 0, Read, NULL, 0, NULL);
    // 创建一个新的执行任务，让这部分工作可以和主任务同时进行。
    hThread2 = (HANDLE)_beginthreadex(NULL, 0, Accu, NULL, 0, NULL);

    // 等待两个子线程全部执行完毕
    WaitForSingleObject(hThread1, INFINITE);
    // 等待另一个任务或同步对象发出“可以继续”的通知。
    WaitForSingleObject(hThread2, INFINITE);

    // 关闭 Windows 句柄，释放它代表的系统资源。
    CloseHandle(semOne);
    // 关闭 Windows 句柄，释放它代表的系统资源。
    CloseHandle(semTwo);
    return 0;
}

// 读取线程：负责输入num
unsigned WINAPI Read(void * arg)
{
    int i;
    // 重复处理任务，直到程序结束或满足退出条件。
    for(i = 0; i < 5; i++)
    {
        fputs("Input num: ", stdout);
        // 等待另一个任务或同步对象发出“可以继续”的通知。
        WaitForSingleObject(semTwo, INFINITE);
        scanf("%d", &num);
        ReleaseSemaphore(semOne, 1, NULL);
    }
    return 0;
}

// 累加线程：读取num并且累加求和
unsigned WINAPI Accu(void * arg)
{
    int sum = 0, i;
    // 重复处理任务，直到程序结束或满足退出条件。
    for(i = 0; i < 5; i++)
    {
        // 等待另一个任务或同步对象发出“可以继续”的通知。
        WaitForSingleObject(semOne, INFINITE);
        sum += num;
        ReleaseSemaphore(semTwo, 1, NULL);
    }
    printf("Result: %d \n", sum);
    return 0;
}