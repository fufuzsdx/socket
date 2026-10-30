/*
 * 本文件用途：TCP 带外数据：演示优先处理的特殊消息。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#define BUF_SIZE 30
void ErrorHandling(char *message);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    SOCKET hSocket;
    SOCKADDR_IN sendAdr;

    if(argc!=3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 创建通信插座；可以把它理解成程序与网络之间的一个接口。
    hSocket=socket(PF_INET, SOCK_STREAM, 0);

    memset(&sendAdr, 0, sizeof(sendAdr));
    sendAdr.sin_family=AF_INET;
    sendAdr.sin_addr.s_addr=inet_addr(argv[1]);
    sendAdr.sin_port=htons(atoi(argv[2]));

    // 主动联系服务器；只有连接成功后，双方才能按 TCP 方式稳定交流。
    if(connect(hSocket, (SOCKADDR*)&sendAdr, sizeof(sendAdr))==SOCKET_ERROR)
        // 主动联系服务器；只有连接成功后，双方才能按 TCP 方式稳定交流。
        ErrorHandling("connect() error!");

    // 把缓冲区中的数据交给网络发送给对方。
    send(hSocket, "123", 3, 0);        //普通数据
    // 把缓冲区中的数据交给网络发送给对方。
    send(hSocket, "4", 1, MSG_OOB);    //✅发送TCP紧急OOB数据
    // 把缓冲区中的数据交给网络发送给对方。
    send(hSocket, "567", 3, 0);        //普通数据
    // 把缓冲区中的数据交给网络发送给对方。
    send(hSocket, "890", 3, MSG_OOB);   //✅再次发送TCP紧急OOB数据

    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hSocket);
    // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
    WSACleanup();
    return 0;
}

// 统一处理错误，避免每个位置重复写相同的退出代码。
void ErrorHandling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}