/*
 * 本文件用途：网络数据缓冲：观察一次接收操作得到的数据。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

void ErrorHandling(char *message);
void ShowSocketBufSize(SOCKET sock);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    SOCKET hSock;
    int sndBuf, rcvBuf, state;

    // 初始化Winsock库
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 创建TCP套接字
    hSock = socket(PF_INET, SOCK_STREAM, 0);

    // ✅第一次打印：创建socket之后，系统默认的收发缓冲区大小
    ShowSocketBufSize(hSock);

    // 设置想要的缓冲区大小：3*1024 = 3072字节
    sndBuf = 1024*3;
    rcvBuf = 1024*3;

    // setsockopt：设置发送缓冲区 SO_SNDBUF
    state = setsockopt(hSock, SOL_SOCKET, SO_SNDBUF, (char*)&sndBuf, sizeof(sndBuf));
    if(state == SOCKET_ERROR)
        ErrorHandling("setsockopt() error!");

    // setsockopt：设置接收缓冲区 SO_RCVBUF
    state = setsockopt(hSock, SOL_SOCKET, SO_RCVBUF, (char*)&rcvBuf, sizeof(rcvBuf));
    if(state == SOCKET_ERROR)
        ErrorHandling("setsockopt() error!");

    // ✅第二次打印：调用setsockopt修改之后的缓冲区大小
    ShowSocketBufSize(hSock);

    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hSock);   //关闭套接字
    // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
    WSACleanup();         //释放winsock资源
    return 0;
}

// 打印套接字发送缓冲区、接收缓冲区大小
void ShowSocketBufSize(SOCKET sock)
{
    int sndBuf, rcvBuf, state, len;

    len = sizeof(sndBuf);
    // getsockopt读取SO_SNDBUF（发送缓冲区）
    state = getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&sndBuf, &len);
    if(state == SOCKET_ERROR)
        ErrorHandling("getsockopt() error");

    len = sizeof(rcvBuf);
    // getsockopt读取SO_RCVBUF（接收缓冲区）
    state = getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&rcvBuf, &len);
    if(state == SOCKET_ERROR)
        ErrorHandling("getsockopt() error");

    printf("Input buffer size: %d \n", rcvBuf);
    printf("Output buffer size: %d \n", sndBuf);
}

// 错误处理函数：输出错误信息，终止程序
void ErrorHandling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}