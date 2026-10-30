/*
 * 本文件用途：重叠 I/O：让 Windows 在后台完成收发操作。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#define BUF_SIZE 1024
void CALLBACK CompRoutine(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
void ErrorHandling(char *message);

WSABUF dataBuf;
char buf[BUF_SIZE];
DWORD recvBytes = 0;

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char* argv[])
{
    WSADATA wsaData;
    SOCKET hLisnSock, hRecvSock;
    SOCKADDR_IN lisnAdr, recvAdr;

    WSAOVERLAPPED overlapped;
    WSAEVENT evObj;
    DWORD flags=0;
    int idx, recvAdrSz = 0;

    if(argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 创建支持重叠IO的流式套接字
    hLisnSock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    memset(&lisnAdr, 0, sizeof(lisnAdr));
    lisnAdr.sin_family = AF_INET;
    lisnAdr.sin_addr.s_addr = htonl(INADDR_ANY);
    lisnAdr.sin_port = htons(atoi(argv[1]));

    // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
    if(bind(hLisnSock, (SOCKADDR*) &lisnAdr, sizeof(lisnAdr)) == SOCKET_ERROR)
        // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
        ErrorHandling("bind() error");
    // 开始等待别人发起连接；这里的数字表示允许暂时排队的连接数。
    if(listen(hLisnSock, 5) == SOCKET_ERROR)
        // 开始等待别人发起连接；这里的数字表示允许暂时排队的连接数。
        ErrorHandling("listen() error");

    recvAdrSz = sizeof(recvAdr);
    // 取出一个等待中的客户端连接，得到一条专门与它通信的新通道。
    hRecvSock = accept(hLisnSock, (SOCKADDR*)&recvAdr, &recvAdrSz);
    if(hRecvSock == INVALID_SOCKET)
        // 取出一个等待中的客户端连接，得到一条专门与它通信的新通道。
        ErrorHandling("accept() error");

    memset(&overlapped, 0, sizeof(overlapped));
    dataBuf.len = BUF_SIZE;
    dataBuf.buf = buf;
    // 创建一个“通知牌”；一个任务举牌后，其他等待者才继续。
    evObj = WSACreateEvent();    // 注释：Dummy event object，傀儡事件，这里本身不用它通知

    // ✅ 重点：最后一个参数传入 APC 回调函数 CompRoutine
    if(WSARecv(hRecvSock, &dataBuf, 1, &recvBytes, &flags, &overlapped, CompRoutine) == SOCKET_ERROR)
    {
        if(WSAGetLastError() == WSA_IO_PENDING)
        {
            puts("Background data receive");
        }
    }

    // fAlertable = TRUE 【可提醒等待】，这是APC版本核心！
    idx = WSAWaitForMultipleEvents(1, &evObj, FALSE, WSA_INFINITE, TRUE);
    if(idx == WAIT_IO_COMPLETION)
    {
        puts("Overlapped I/O Completed");
    }
    else
    {
        // 从网络读取数据，并把收到的内容放入缓冲区。
        ErrorHandling("WSARecv() error");
    }

    WSACloseEvent(evObj);
    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hRecvSock);
    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hLisnSock);
    // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
    WSACleanup();
    return 0;
}

// APC 完成回调例程 CompRoutine
void CALLBACK CompRoutine(DWORD dwError, DWORD szRecvBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
{
    if(dwError != 0)
    {
        ErrorHandling("CompRoutine error");
    }
    else
    {
        recvBytes = szRecvBytes;
        printf("Received message: %s \n", buf);
    }
}

// 统一处理错误，避免每个位置重复写相同的退出代码。
void ErrorHandling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}