/*
 * 本文件用途：线程同步：使用临界区、事件、互斥量和信号量协调任务。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <windows.h>
#include <process.h>

#define STR_LEN   100

unsigned WINAPI NumberOfA(void *arg);
unsigned WINAPI NumberOfOthers(void *arg);

static char str[STR_LEN];
static HANDLE hEvent;

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    HANDLE hThread1, hThread2;
    // 创建一个“通知牌”；一个任务举牌后，其他等待者才继续。
    // CreateEvent(安全属性, bManualReset=TRUE手动重置, 初始状态FALSE无信号, 名字NULL)
    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // 创建一个新的执行任务，让这部分工作可以和主任务同时进行。
    hThread1 = (HANDLE)_beginthreadex(NULL, 0, NumberOfA, NULL, 0, NULL);
    // 创建一个新的执行任务，让这部分工作可以和主任务同时进行。
    hThread2 = (HANDLE)_beginthreadex(NULL, 0, NumberOfOthers, NULL, 0, NULL);

    fputs("Input string: ", stdout);
    fgets(str, STR_LEN, stdin);

    // 发出通知，让正在等待这个事件的任务继续执行。
    SetEvent(hEvent);   // 将事件设置为有信号状态

    // 等待另一个任务或同步对象发出“可以继续”的通知。
    WaitForSingleObject(hThread1, INFINITE);
    // 等待另一个任务或同步对象发出“可以继续”的通知。
    WaitForSingleObject(hThread2, INFINITE);

    // 收回事件通知，恢复为需要继续等待的状态。
    ResetEvent(hEvent);  // 手动重置事件为无信号
    // 关闭 Windows 句柄，释放它代表的系统资源。
    CloseHandle(hEvent);
    return 0;
}

// 统计字符'A'的数量
unsigned WINAPI NumberOfA(void *arg)
{
    int i, cnt = 0;
    // 等待另一个任务或同步对象发出“可以继续”的通知。
    WaitForSingleObject(hEvent, INFINITE);
    // 重复处理任务，直到程序结束或满足退出条件。
    for(i = 0; str[i] != 0; i++)
    {
        if(str[i] == 'A')
            cnt++;
    }
    printf("Num of A: %d \n", cnt);
    return 0;
}

// 统计除'A'以外字符的数量
unsigned WINAPI NumberOfOthers(void *arg)
{
    int i, cnt = 0;
    // 等待另一个任务或同步对象发出“可以继续”的通知。
    WaitForSingleObject(hEvent, INFINITE);
    // 重复处理任务，直到程序结束或满足退出条件。
    for(i = 0; str[i] != 0; i++)
    {
        if(str[i] != 'A')
            cnt++;
    }
    printf("Num of others: %d \n", cnt-1);
    return 0;
}