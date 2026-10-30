/*
 * 本文件用途：聊天程序：分别用线程负责发送和接收消息。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

unsigned WINAPI HandleClnt(void * arg);
void SendMsg(char * msg, int len);
void ErrorHandling(char * msg);

int clntCnt = 0;
SOCKET clntSocks[MAX_CLNT];   // 保存所有已连接客户端socket的共享数组
HANDLE hMutex;                 // 互斥量，保护共享变量 clntCnt、clntSocks

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    SOCKET hServSock, hClntSock;
    SOCKADDR_IN servAdr, clntAdr;
    int clntAdrSz;
    HANDLE hThread;

    if(argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
    // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
    if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 创建一把互斥锁；同一时刻只允许一个线程修改共享内容。
    hMutex = CreateMutex(NULL, FALSE, NULL);   // 创建互斥量
    // 创建通信插座；可以把它理解成程序与网络之间的一个接口。
    hServSock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAdr.sin_port = htons(atoi(argv[1]));

    // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
    if(bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
        // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
        ErrorHandling("bind() error");
    // 开始等待别人发起连接；这里的数字表示允许暂时排队的连接数。
    if(listen(hServSock,5) == SOCKET_ERROR)
        // 开始等待别人发起连接；这里的数字表示允许暂时排队的连接数。
        ErrorHandling("listen() error");

    // 重复处理任务，直到程序结束或满足退出条件。
    while(1)
    {
        clntAdrSz = sizeof(clntAdr);
        // 取出一个等待中的客户端连接，得到一条专门与它通信的新通道。
        hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &clntAdrSz);

        // =====临界区开始：修改共享客户端数组=====
        WaitForSingleObject(hMutex, INFINITE);
        clntSocks[clntCnt++] = hClntSock;
        ReleaseMutex(hMutex);
        // =====临界区结束=====

        // 创建一个新的执行任务，让这部分工作可以和主任务同时进行。
        hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClnt, (void*)&hClntSock, 0, NULL);
        printf("Connected client IP: %s \n", inet_ntoa(clntAdr.sin_addr));
    }

    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hServSock);
    // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
    WSACleanup();
    return 0;
}

// 每个客户端对应的子线程函数
unsigned WINAPI HandleClnt(void * arg)
{
    SOCKET hClntSock = *((SOCKET*)arg);
    int strLen = 0, i;
    char msg[BUF_SIZE];

    // 循环接收当前客户端发来的消息
    while( (strLen = recv(hClntSock, msg, sizeof(msg), 0)) != 0 )
    {
        SendMsg(msg, strLen);
    }

    // =====临界区开始：客户端断开连接，从共享数组删除该socket=====
    WaitForSingleObject(hMutex, INFINITE);
    // 重复处理任务，直到程序结束或满足退出条件。
    for(i = 0; i < clntCnt; i++)
    {
        if(hClntSock == clntSocks[i])
        {
            // 将后面元素向前移动一位，覆盖断开的客户端
            while(i++ < clntCnt -1)
                clntSocks[i] = clntSocks[i+1];
            break;
        }
    }
    clntCnt--;
    ReleaseMutex(hMutex);
    // =====临界区结束=====

    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hClntSock);
    return 0;
}

// 广播函数：把收到的消息发送给全部在线客户端
void SendMsg(char * msg, int len)
{
    int i;
    // 等待另一个任务或同步对象发出“可以继续”的通知。
    WaitForSingleObject(hMutex, INFINITE);
    // 重复处理任务，直到程序结束或满足退出条件。
    for(i = 0; i < clntCnt; i++)
    {
        // 把缓冲区中的数据交给网络发送给对方。
        send(clntSocks[i], msg, len, 0);
    }
    ReleaseMutex(hMutex);
}

// 统一处理错误，避免每个位置重复写相同的退出代码。
void ErrorHandling(char * msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}