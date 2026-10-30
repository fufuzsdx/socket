/*
 * 本文件用途：完成端口和完成例程：处理大量并发网络连接。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <winsock2.h>
#include <windows.h>

#define BUF_SIZE 100
#define READ     3
#define WRITE    5

// PER_HANDLE_DATA：per-socket 每个连接一份（CompletionKey）
typedef struct   // socket info
{
    SOCKET hClntSock;
    SOCKADDR_IN clntAdr;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

// PER_IO_DATA：per-I/O 每一次WSARecv / WSASend 一份，附带overlapped
typedef struct   // buffer info
{
    OVERLAPPED overlapped;
    WSABUF wsaBuf;
    char buffer[BUF_SIZE];
    int rwMode;   // READ or WRITE
} PER_IO_DATA, *LPPER_IO_DATA;

unsigned WINAPI EchoThreadMain(LPVOID CompletionPortIO);
void ErrorHandling(char *message);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char* argv[])
{
    WSADATA wsaData;
    HANDLE hComPort;
    SYSTEM_INFO sysInfo;
    LPPER_IO_DATA ioInfo;
    LPPER_HANDLE_DATA handleInfo;

    SOCKET hServSock;
    SOCKADDR_IN servAdr;
    DWORD recvBytes, flags=0;
    int i;

    // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 1. 创建IOCP完成端口对象
    hComPort=CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    // 获取系统逻辑CPU数量
    GetSystemInfo(&sysInfo);
    // 根据逻辑CPU数，创建等量工作线程 _beginthreadex
    for(i=0; i<sysInfo.dwNumberOfProcessors; i++)
        // 创建一个新的执行任务，让这部分工作可以和主任务同时进行。
        _beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);

    // 创建重叠IO版本监听socket
    hServSock=WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family=AF_INET;
    servAdr.sin_addr.s_addr=INADDR_ANY;
    servAdr.sin_port=htons(atoi(argv[1]));

    // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
    bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr));
    // 开始等待别人发起连接；这里的数字表示允许暂时排队的连接数。
    listen(hServSock, 5);

    // 重复处理任务，直到程序结束或满足退出条件。
    while(1)
    {
        SOCKET hClntSock;
        SOCKADDR_IN clntAdr;
        int addrLen=sizeof(clntAdr);
        // 【注意：这里是阻塞accept，不是AcceptEx，教材简化写法】
        hClntSock=accept(hServSock, (SOCKADDR*)&clntAdr, &addrLen);

        // 分配 per-handle 数据：这个客户端连接的信息，作为CompletionKey
        handleInfo=(LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
        handleInfo->hClntSock=hClntSock;
        memcpy(&(handleInfo->clntAdr), &clntAdr, addrLen);

        // 将新客户端socket绑定到IOCP，传入handleInfo作为CompletionKey
        CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (ULONG_PTR)handleInfo, 0);

        // 分配 per-IO 数据，准备投递第一次WSARecv（读）
        ioInfo=(LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
        memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
        ioInfo->wsaBuf.len=BUF_SIZE;
        ioInfo->wsaBuf.buf=ioInfo->buffer;
        ioInfo->rwMode=READ;

        // 投递异步读，交给内核等待客户端发数据
        WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf),
            1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
    }
    return 0;
}

// IOCP工作线程入口函数
unsigned WINAPI EchoThreadMain(LPVOID pComPort)
{
    HANDLE hComPort=(HANDLE)pComPort;
    SOCKET sock;
    DWORD bytesTrans;
    LPPER_HANDLE_DATA handleInfo;
    LPPER_IO_DATA ioInfo;
    DWORD flags=0;

    // 重复处理任务，直到程序结束或满足退出条件。
    while(1)
    {
        // 阻塞等待IO完成包
        GetQueuedCompletionStatus(hComPort, &bytesTrans,
            (ULONG_PTR*)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);

        sock=handleInfo->hClntSock;

        if(ioInfo->rwMode==READ)
        {
            puts("message received!");
            if(bytesTrans==0)   // 收到EOF，客户端正常关闭连接
            {
                // 关闭网络通道；关闭后不能再通过它收发数据。
                closesocket(sock);
                free(handleInfo);
                free(ioInfo);
                continue;
            }
            // 读完成，准备写回（回声），复用本次ioInfo的overlapped和buffer
            memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
            ioInfo->wsaBuf.len=bytesTrans;
            ioInfo->rwMode=WRITE;
            // 投递异步发送 WSASend
            WSASend(sock, &(ioInfo->wsaBuf),
                1, NULL, 0, &(ioInfo->overlapped), NULL);
        }
        else
        {
            // WRITE模式：发送完成
            puts("message sent!");
            // 本次写IO完成，释放旧ioInfo
            free(ioInfo);

            // 分配新的PER_IO_DATA，再次投递READ，等待下一条消息
            ioInfo=(LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
            memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
            ioInfo->wsaBuf.len=BUF_SIZE;
            ioInfo->wsaBuf.buf=ioInfo->buffer;
            ioInfo->rwMode=READ;
            // 从网络读取数据，并把收到的内容放入缓冲区。
            WSARecv(sock, &(ioInfo->wsaBuf),
                1, NULL, &flags, &(ioInfo->overlapped), NULL);
        }
    }
    return 0;
}

// 统一处理错误，避免每个位置重复写相同的退出代码。
void ErrorHandling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}