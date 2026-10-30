/*
 * 本文件用途：网络基础：地址转换、字节顺序，以及最简单的 TCP 客户端和服务器。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

/*
 * Windows 平台下的简单 TCP 回显/Hello World 服务器示例。
 * 程序启动后会监听指定端口，并接受第一个客户端连接，
 * 然后向客户端发送 "Hello World!" 字符串并关闭连接。
 */

void ErrorHandling(char* message);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char* argv[])
{
    WSADATA wsaData;              // Winsock 库初始化所需的结构体
    SOCKET hServSock, hClntSock;  // 服务器套接字、客户端套接字
    SOCKADDR_IN servAddr, clntAddr;  // 服务器/客户端地址结构体

    int szClntAddr;               // 客户端地址结构体大小
    char message[] = "Hello World!";  // 发送给客户端的消息内容

    // 命令行参数校验：程序必须提供监听端口号
    if(argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    // 初始化 Winsock 2.2 库，确保 socket API 可用
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 创建 TCP 监听套接字，使用 IPv4 地址族和流式套接字
    hServSock = socket(PF_INET, SOCK_STREAM, 0);
    if(hServSock == INVALID_SOCKET)
        // 创建通信插座；可以把它理解成程序与网络之间的一个接口。
        ErrorHandling("socket() error");

    // 将服务器地址结构体清零并填充监听端口和地址
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(atoi(argv[1]));

    // 绑定服务器地址到套接字，允许本机任意网卡接受连接
    if(bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
        // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
        ErrorHandling("bind() error");

    // 开始监听，允许最多 5 个连接请求排队
    if(listen(hServSock, 5) == SOCKET_ERROR)
        // 开始等待别人发起连接；这里的数字表示允许暂时排队的连接数。
        ErrorHandling("listen() error");

    szClntAddr = sizeof(clntAddr);

    // 阻塞等待客户端连接；accept 返回已建立连接的客户端套接字
    hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &szClntAddr);
    if(hClntSock == INVALID_SOCKET)
        // 取出一个等待中的客户端连接，得到一条专门与它通信的新通道。
        ErrorHandling("accept() error");

    // 向客户端发送字符串消息
    send(hClntSock, message, sizeof(message), 0);

    // 关闭客户端套接字和监听套接字，释放资源
    closesocket(hClntSock);
    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hServSock);
    // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
    WSACleanup();

    return 0;
}

/*
 * 统一的错误处理函数：输出错误信息并退出程序。
 * 该函数用于集中处理 socket 相关函数失败的场景。
 */
void ErrorHandling(char* message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}