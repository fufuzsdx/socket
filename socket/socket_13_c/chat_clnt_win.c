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
#define NAME_SIZE 20

unsigned WINAPI SendMsg(void * arg);
unsigned WINAPI RecvMsg(void * arg);
void ErrorHandling(char * msg);

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    SOCKET hSock;
    SOCKADDR_IN servAdr;
    HANDLE hSndThread, hRcvThread;

    if(argc !=4) {
        printf("Usage : %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }
    // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
    if(WSAStartup(MAKEWORD(2,2), &wsaData) !=0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    sprintf(name, "[%s]", argv[3]);
    // 创建通信插座；可以把它理解成程序与网络之间的一个接口。
    hSock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = inet_addr(argv[1]);
    servAdr.sin_port = htons(atoi(argv[2]));

    // 主动联系服务器；只有连接成功后，双方才能按 TCP 方式稳定交流。
    if(connect(hSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
        // 主动联系服务器；只有连接成功后，双方才能按 TCP 方式稳定交流。
        ErrorHandling("connect() error");

    // 创建两个线程：发送线程 + 接收线程
    hSndThread = (HANDLE)_beginthreadex(NULL, 0, SendMsg, (void*)&hSock, 0, NULL);
    // 创建一个新的执行任务，让这部分工作可以和主任务同时进行。
    hRcvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&hSock, 0, NULL);

    // 等待另一个任务或同步对象发出“可以继续”的通知。
    WaitForSingleObject(hSndThread, INFINITE);
    // 等待另一个任务或同步对象发出“可以继续”的通知。
    WaitForSingleObject(hRcvThread, INFINITE);

    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hSock);
    // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
    WSACleanup();
    return 0;
}

// 发送线程：读取控制台输入，发送消息到服务器
unsigned WINAPI SendMsg(void * arg)
{
    SOCKET hSock = *((SOCKET*)arg);
    char nameMsg[NAME_SIZE + BUF_SIZE];

    // 重复处理任务，直到程序结束或满足退出条件。
    while(1)
    {
        fgets(msg, BUF_SIZE, stdin);
        // 如果输入 q\n 或者 Q\n 退出聊天室
        if(!strcmp(msg,"q\n") || !strcmp(msg,"Q\n"))
        {
            // 关闭网络通道；关闭后不能再通过它收发数据。
            closesocket(hSock);
            exit(0);
        }
        sprintf(nameMsg, "%s %s", name, msg);
        // 把缓冲区中的数据交给网络发送给对方。
        send(hSock, nameMsg, strlen(nameMsg), 0);
    }
    return 0;
}

// 接收线程：接收服务器广播过来的消息，打印到控制台
unsigned WINAPI RecvMsg(void * arg)
{
    int hSock = *((SOCKET*)arg);
    char nameMsg[NAME_SIZE + BUF_SIZE];
    int strLen;

    // 重复处理任务，直到程序结束或满足退出条件。
    while(1)
    {
        // 从网络读取数据，并把收到的内容放入缓冲区。
        strLen = recv(hSock, nameMsg, NAME_SIZE + BUF_SIZE - 1, 0);
        if(strLen == -1)
            return -1;
        nameMsg[strLen] = 0;
        fputs(nameMsg, stdout);
    }
    return 0;
}

// 统一处理错误，避免每个位置重复写相同的退出代码。
void ErrorHandling(char * msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}