/*
 * 本文件用途：重叠 I/O：让 Windows 在后台完成收发操作。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

void ErrorHandling(char *msg);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    SOCKET hSocket;
    SOCKADDR_IN sendAdr;

    WSABUF dataBuf;
    char msg[] = "Network is Computer!";
    DWORD sendBytes = 0;

    WSAEVENT evObj;
    WSAOVERLAPPED overlapped;

    // 参数检查：需要传入 目标IP + 端口
    if (argc != 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // 初始化Winsock2.2库
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 创建【重叠属性】的TCP套接字，WSA_FLAG_OVERLAPPED代表支持重叠IO
    hSocket = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    memset(&sendAdr, 0, sizeof(sendAdr));
    sendAdr.sin_family = AF_INET;
    sendAdr.sin_addr.s_addr = inet_addr(argv[1]);
    sendAdr.sin_port = htons(atoi(argv[2]));

    // 建立TCP连接
    if (connect(hSocket, (SOCKADDR*)&sendAdr, sizeof(sendAdr)) == SOCKET_ERROR)
        // 主动联系服务器；只有连接成功后，双方才能按 TCP 方式稳定交流。
        ErrorHandling("connect() error!");

    // 创建一个“通知牌”；一个任务举牌后，其他等待者才继续。
    evObj = WSACreateEvent();          // 创建手动重置WSA事件
    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = evObj;         // 把事件绑定到重叠结构
    dataBuf.len = strlen(msg) + 1;     // +1带上字符串结束符'\0'
    dataBuf.buf = msg;                 // 绑定发送缓冲区

    // 发起重叠发送 WSASend
    if (WSASend(hSocket, &dataBuf, 1, &sendBytes, 0, &overlapped, NULL) == SOCKET_ERROR)
    {
        // 判断是不是【正常后台提交】
        if (WSAGetLastError() == WSA_IO_PENDING)
        {
            puts("Background data send");
            // 阻塞等待事件变为有信号（IO完成）
            WSAWaitForMultipleEvents(1, &evObj, TRUE, WSA_INFINITE, FALSE);
            // 获取本次重叠IO最终结果、实际发送字节
            WSAGetOverlappedResult(hSocket, &overlapped, &sendBytes, FALSE, NULL);
        }
        else
        {
            // 真正的错误，提交IO失败
            ErrorHandling("WSASend() error");
        }
    }
    // 如果WSASend直接返回0：代表IO立刻同步完成，sendBytes已经有效

    printf("Send data size: %d \n", sendBytes);
    WSACloseEvent(evObj);    // 释放事件内核对象
    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hSocket);
    // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
    WSACleanup();           // 清理winsock库
    return 0;
}

// 统一处理错误，避免每个位置重复写相同的退出代码。
void ErrorHandling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}