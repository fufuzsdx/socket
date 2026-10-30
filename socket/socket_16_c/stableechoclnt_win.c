/*
 * 本文件用途：完成端口和完成例程：处理大量并发网络连接。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")   // 自动链接ws2_32.lib

#define BUF_SIZE 1024
void ErrorHandling(char *message);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    SOCKET hSocket;
    SOCKADDR_IN servAdr;
    char message[BUF_SIZE];
    int strLen, readLen;

    if(argc != 3)
    {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 创建通信插座；可以把它理解成程序与网络之间的一个接口。
    hSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(hSocket == INVALID_SOCKET)
        // 创建通信插座；可以把它理解成程序与网络之间的一个接口。
        ErrorHandling("socket() error");

    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = inet_addr(argv[1]);
    servAdr.sin_port = htons(atoi(argv[2]));

    // 主动联系服务器；只有连接成功后，双方才能按 TCP 方式稳定交流。
    if(connect(hSocket, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
        // 主动联系服务器；只有连接成功后，双方才能按 TCP 方式稳定交流。
        ErrorHandling("connect() error!");
    else
        puts("Connected...........");

    // 重复处理任务，直到程序结束或满足退出条件。
    while(1)
    {
        fputs("Input message(Q to quit): ", stdout);
        fgets(message, BUF_SIZE, stdin);

        if(!strcmp(message,"q\n") || !strcmp(message,"Q\n"))
            break;

        strLen = strlen(message);
        // 把缓冲区中的数据交给网络发送给对方。
        send(hSocket, message, strLen, 0);

        readLen = 0;
        // 重复处理任务，直到程序结束或满足退出条件。
        while(1)
        {
            // 从已经读到的位置继续接收
            int ret = recv(hSocket, &message[readLen], BUF_SIZE - 1 - readLen, 0);
            if(ret <= 0)   // 【补漏洞】断开或者出错直接退出
            {
                ErrorHandling("recv error or server closed");
                // 关闭网络通道；关闭后不能再通过它收发数据。
                closesocket(hSocket);
                // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
                WSACleanup();
                exit(1);
            }
            readLen += ret;
            if(readLen >= strLen)
                break;
        }
        message[strLen] = 0;  // 字符串结束符
        printf("Message from server: %s", message);
    }

    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hSocket);
    // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
    WSACleanup();
    return 0;
}

// 补全错误处理函数
void ErrorHandling(char *message)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0,
        NULL
    );
    printf("%s : %s\n", message, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
    exit(EXIT_FAILURE);
}