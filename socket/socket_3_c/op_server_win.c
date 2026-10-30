/*
 * 本文件用途：TCP 计算服务：客户端发送操作请求，服务器返回计算结果。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define BUF_SIZE 1024
#define OPSZ 4

void ErrorHandling(char *message);
int calculate(int opnum, int opnds[], char op);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    SOCKET hServSock, hClntSock;
    char opinfo[BUF_SIZE];
    int result, opndCnt, i;
    int recvCnt, recvLen;
    SOCKADDR_IN servAdr, clntAdr;
    int clntAdrSize;

    if(argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 创建通信插座；可以把它理解成程序与网络之间的一个接口。
    hServSock = socket(PF_INET, SOCK_STREAM, 0);
    if(hServSock == INVALID_SOCKET)
        // 创建通信插座；可以把它理解成程序与网络之间的一个接口。
        ErrorHandling("socket() error");

    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAdr.sin_port = htons(atoi(argv[1]));

    // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
    if(bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
        // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
        ErrorHandling("bind() error");
    // 开始等待别人发起连接；这里的数字表示允许暂时排队的连接数。
    if(listen(hServSock, 5) == SOCKET_ERROR)
        // 开始等待别人发起连接；这里的数字表示允许暂时排队的连接数。
        ErrorHandling("listen() error");
    clntAdrSize = sizeof(clntAdr);

    // 重复处理任务，直到程序结束或满足退出条件。
    for(i = 0; i < 5; i++)
    {
        opndCnt = 0;
        // 取出一个等待中的客户端连接，得到一条专门与它通信的新通道。
        hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &clntAdrSize);

        // 从网络读取数据，并把收到的内容放入缓冲区。
        recv(hClntSock,(char*)&opndCnt, 1, 0);
        recvLen = 0;
        // 重复处理任务，直到程序结束或满足退出条件。
        while( (opndCnt*OPSZ + 1) > recvLen )
        {
            // 从网络读取数据，并把收到的内容放入缓冲区。
            recvCnt = recv(hClntSock, &opinfo[recvLen], BUF_SIZE-1, 0);
            recvLen += recvCnt;
        }
        result = calculate(opndCnt, (int*)opinfo, opinfo[recvLen-1]);
        // 把缓冲区中的数据交给网络发送给对方。
        send(hClntSock, (char*)&result, sizeof(result), 0);
        // 关闭网络通道；关闭后不能再通过它收发数据。
        closesocket(hClntSock);
    }
    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hServSock);
    // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
    WSACleanup();
    return 0;
}

// 这个函数负责完成“calculate”这一步工作。
int calculate(int opnum, int opnds[], char op)
{
    int result = opnds[0], i;
    switch(op)
    {
        case '+':
            // 重复处理任务，直到程序结束或满足退出条件。
            for(i = 1; i < opnum; i++) result += opnds[i];
            break;
        case '-':
            // 重复处理任务，直到程序结束或满足退出条件。
            for(i = 1; i < opnum; i++) result -= opnds[i];
            break;
        case '*':
            // 重复处理任务，直到程序结束或满足退出条件。
            for(i = 1; i < opnum; i++) result *= opnds[i];
            break;
    }
    return result;
}

// 统一处理错误，避免每个位置重复写相同的退出代码。
void ErrorHandling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}