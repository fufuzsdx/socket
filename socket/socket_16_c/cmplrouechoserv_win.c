/*
 * 本文件用途：完成端口和完成例程：处理大量并发网络连接。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#define BUF_SIZE 1024
void CALLBACK ReadCompRoutine(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
void CALLBACK WriteCompRoutine(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
void ErrorHandling(char *message);

// 自定义每一个IO的上下文结构体！【本代码最核心考点】
typedef struct
{
    SOCKET hClntSock;
    char buf[BUF_SIZE];
    WSABUF wsaBuf;
} PER_IO_DATA, *LPPER_IO_DATA;

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char* argv[])
{
    WSADATA wsaData;
    SOCKET hLisnSock, hRecvSock;
    SOCKADDR_IN lisnAdr, recvAdr;
    LPWSAOVERLAPPED lpOvLp;
    DWORD recvBytes,flagInfo=0;
    LPPER_IO_DATA hbInfo;
    u_long mode=1;
    int recvAdrSz=0;
    

    if(argc!=2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 创建支持重叠IO的套接字
    hLisnSock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    ioctlsocket(hLisnSock, FIONBIO, &mode);  // 监听套接字设置为非阻塞

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
    // 重复处理任务，直到程序结束或满足退出条件。
    while(1)
    {
        // SleepEx(...,TRUE)：进入【可提醒等待 alertable wait】，APC触发条件
        SleepEx(100, TRUE);
        // 取出一个等待中的客户端连接，得到一条专门与它通信的新通道。
        hRecvSock = accept(hLisnSock, (SOCKADDR*)&recvAdr,&recvAdrSz);
        if(hRecvSock == INVALID_SOCKET)
        {
            if(WSAGetLastError() == WSAEWOULDBLOCK)
                continue;
            else
                // 取出一个等待中的客户端连接，得到一条专门与它通信的新通道。
                ErrorHandling("accept() error");
        }
        puts("Client connected......");

        // 动态分配重叠结构 WSAOVERLAPPED
        lpOvLp = (LPWSAOVERLAPPED)malloc(sizeof(WSAOVERLAPPED));
        memset(lpOvLp, 0, sizeof(WSAOVERLAPPED));

        // 分配本次IO的上下文信息结构体 PER_IO_DATA
        hbInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
        hbInfo->hClntSock = (DWORD)hRecvSock;
        hbInfo->wsaBuf.buf = hbInfo->buf;
        hbInfo->wsaBuf.len = BUF_SIZE;

        // ✅ 关键 trick：把自定义上下文结构体指针 存入 overlapped->hEvent
        // hEvent本来是用来放事件句柄的，这里复用它存我们自己数据的指针！
        lpOvLp->hEvent = (HANDLE)hbInfo;

        // 投递异步接收，注册读完成回调 ReadCompRoutine
        WSARecv(hRecvSock, &(hbInfo->wsaBuf),
            1, &recvBytes, &flagInfo, lpOvLp, ReadCompRoutine);
    }

    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hRecvSock);
    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hLisnSock);
    // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
    WSACleanup();
    return 0;
}

// 读IO完成回调：收到数据之后执行
void CALLBACK ReadCompRoutine(
    DWORD dwError, DWORD szRecvBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
{
    // 从overlapped.hEvent取出我们之前存的PER_IO_DATA指针！核心
    LPPER_IO_DATA hbInfo=(LPPER_IO_DATA)(lpOverlapped->hEvent);
    SOCKET hSock=hbInfo->hClntSock;
    LPWSABUF bufInfo=&(hbInfo->wsaBuf);
    DWORD sentBytes;

    if(szRecvBytes == 0)
    {
        // 收到0字节：对端关闭连接，释放内存
        closesocket(hSock);
        free(lpOverlapped->hEvent);
        free(lpOverlapped);
        puts("Client disconnected......");
    }
    else
    {
        // 回声：收到多少，就发回多少
        bufInfo->len = szRecvBytes;
        // 投递异步发送，注册写完成回调 WriteCompRoutine
        WSASend(hSock, bufInfo, 1, &sentBytes, 0, lpOverlapped, WriteCompRoutine);
    }
}

// 写IO完成回调：发送完成之后执行
void CALLBACK WriteCompRoutine(
    DWORD dwError, DWORD szSendBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
{
    LPPER_IO_DATA hbInfo=(LPPER_IO_DATA)(lpOverlapped->hEvent);
    SOCKET hSock=hbInfo->hClntSock;
    LPWSABUF bufInfo=&(hbInfo->wsaBuf);
    DWORD recvBytes;
    DWORD flagInfo=0;

    // 发送完数据，立刻再次投递WSARecv，等待下一轮客户端消息
    WSARecv(hSock, bufInfo,
        1, &recvBytes, &flagInfo, lpOverlapped, ReadCompRoutine);
}

// 统一处理错误，避免每个位置重复写相同的退出代码。
void ErrorHandling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}