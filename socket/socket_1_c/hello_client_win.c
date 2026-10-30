/*
 * 本文件用途：网络基础：地址转换、字节顺序，以及最简单的 TCP 客户端和服务器。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

/*
 * Windows 平台下的简单 TCP 客户端示例。
 * 程序会连接到指定的服务器 IP 和端口，
 * 接收服务器发送的字符串消息，并打印出来。
 */

void ErrorHandling(char* message);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char* argv[])
{
    WSADATA wsaData;              // Winsock 库初始化所需的结构体
    SOCKET hSocket;              // 客户端套接字
    SOCKADDR_IN servAddr;        // 服务器地址结构体

    char message[30];           // 保存服务器发送的数据
    int strLen;                 // 接收数据的长度

    // 命令行参数校验：程序必须提供目标 IP 和端口号
    if(argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // 初始化 Winsock 2.2 库，确保 socket API 可用
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 创建 TCP 套接字，使用 IPv4 地址族和流式套接字
    hSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(hSocket == INVALID_SOCKET)
        // 创建通信插座；可以把它理解成程序与网络之间的一个接口。
        ErrorHandling("socket() error");

    // 填充服务器地址结构体并准备连接
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(argv[1]);
    servAddr.sin_port = htons(atoi(argv[2]));

    // 向服务器发起连接请求
    if(connect(hSocket, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
        // 主动联系服务器；只有连接成功后，双方才能按 TCP 方式稳定交流。
        ErrorHandling("connect() error!");

    // 从服务器接收消息；接收缓冲区留一个位置给字符串终止符
    strLen = recv(hSocket, message, sizeof(message) - 1, 0);
    if(strLen == -1)
        ErrorHandling("read() error!");

    // 确保接收到的字符串以 '\0' 结尾后再输出
    message[strLen] = '\0';
    printf("Message from server: %s \n", message);

    // 关闭套接字并清理 Winsock 资源
    closesocket(hSocket);
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