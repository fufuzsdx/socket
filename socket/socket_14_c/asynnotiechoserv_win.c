/*
 * 本文件用途：异步网络通信：等待网络事件时不阻塞主流程。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <string.h>
#include <winsock2.h>

#define BUF_SIZE 100

void CompressSockets(SOCKET hSockArr[], int idx, int total);
void CompressEvents(WSAEVENT hEventArr[], int idx, int total);
void ErrorHandling(char *msg);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    SOCKET hServSock, hClntSock;
    SOCKADDR_IN servAdr, clntAdr;

    // 套接字数组、事件数组：一一对应！
    SOCKET hSockArr[WSA_MAXIMUM_WAIT_EVENTS];
    WSAEVENT hEventArr[WSA_MAXIMUM_WAIT_EVENTS];
    WSAEVENT newEvent;
    WSANETWORKEVENTS netEvents;

    int numOfClntSock = 0;
    int strLen, i;
    int posInfo, startIdx;
    int clntAdrLen;
    char msg[BUF_SIZE];

    // 参数检查：必须传入端口号
    if(argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // 初始化Winsock 2.2
    if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 1. 创建监听socket
    hServSock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAdr.sin_port = htons(atoi(argv[1]));

    // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
    bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr));
    // 开始等待别人发起连接；这里的数字表示允许暂时排队的连接数。
    listen(hServSock, 5);

    // 创建事件，把监听socket和事件绑定，只监听FD_ACCEPT（新连接）
    newEvent = WSACreateEvent();
    WSAEventSelect(hServSock, newEvent, FD_ACCEPT);

    // 把监听socket和对应的事件存入数组
    hSockArr[numOfClntSock] = hServSock;
    hEventArr[numOfClntSock] = newEvent;
    numOfClntSock ++;


    // 主循环：持续等待网络事件
    while(1)
    {
        // 等待事件数组里任意一个事件变为有信号，无限阻塞
        posInfo = WSAWaitForMultipleEvents(
            numOfClntSock,
            hEventArr,
            FALSE,
            WSA_INFINITE,
            FALSE
        );
        startIdx = posInfo - WSA_WAIT_EVENT_0;

        // 从触发的下标开始向后遍历（防止多个事件同时就绪）
        for(i = startIdx; i < numOfClntSock; i++)
        {
            // 等待后台网络操作完成，完成后再读取结果。
            int sigEventIdx = WSAWaitForMultipleEvents(1, &hEventArr[i], TRUE, 0, FALSE);
            if(sigEventIdx == WSA_WAIT_FAILED || sigEventIdx == WSA_WAIT_TIMEOUT)
            {
                continue;
            }
            else
            {
                sigEventIdx = i;
                // 查询这个socket上到底发生了什么网络事件；同时自动重置manual事件
                WSAEnumNetworkEvents(
                    hSockArr[sigEventIdx],
                    hEventArr[sigEventIdx],
                    &netEvents
                );

                // ========== 事件1：FD_ACCEPT 有新客户端连接 ==========
                if(netEvents.lNetworkEvents & FD_ACCEPT)
                {
                    if(netEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
                    {
                        puts("Accept Error");
                        break;
                    }
                    clntAdrLen = sizeof(clntAdr);
                    // 取出一个等待中的客户端连接，得到一条专门与它通信的新通道。
                    hClntSock = accept(hSockArr[sigEventIdx], (SOCKADDR*)&clntAdr, &clntAdrLen);

                    // 给新客户端socket创建事件，注册：可读FD_READ + 关闭FD_CLOSE
                    newEvent = WSACreateEvent();
                    WSAEventSelect(hClntSock, newEvent, FD_READ | FD_CLOSE);

                    // 新的socket、事件加入数组
                    hSockArr[numOfClntSock] = hClntSock;
                    hEventArr[numOfClntSock] = newEvent;
                    numOfClntSock ++;
                    puts("connected new client...");
                }

                // ========== 事件2：FD_READ 收到客户端数据 ==========
                if(netEvents.lNetworkEvents & FD_READ)
                {
                    if(netEvents.iErrorCode[FD_READ_BIT] != 0)
                    {
                        puts("Read Error");
                        break;
                    }
                    // 从网络读取数据，并把收到的内容放入缓冲区。
                    strLen = recv(hSockArr[sigEventIdx], msg, sizeof(msg), 0);
                    // 把缓冲区中的数据交给网络发送给对方。
                    send(hSockArr[sigEventIdx], msg, strLen, 0); // echo回发
                }

                // ========== 事件3：FD_CLOSE 客户端断开连接 ==========
                if(netEvents.lNetworkEvents & FD_CLOSE)
                {
                    if(netEvents.iErrorCode[FD_CLOSE_BIT] != 0)
                    {
                        puts("Close Error");
                        break;
                    }
                    // 释放资源：关闭事件、关闭socket
                    WSACloseEvent(hEventArr[sigEventIdx]);
                    // 关闭网络通道；关闭后不能再通过它收发数据。
                    closesocket(hSockArr[sigEventIdx]);
                    numOfClntSock --;
                    // 数组压缩：把后面元素往前挪，填补空缺
                    CompressSockets(hSockArr, sigEventIdx, numOfClntSock);
                    CompressEvents(hEventArr, sigEventIdx, numOfClntSock);
                }
            }
        }
    }

    // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
    WSACleanup();
    return 0;
}

// 删除下标idx的元素：socket数组前移覆盖
void CompressSockets(SOCKET hSockArr[], int idx, int total)
{
    int i;
    // 重复处理任务，直到程序结束或满足退出条件。
    for(i = idx; i < total; i++)
        hSockArr[i] = hSockArr[i+1];
}

// 删除下标idx的元素：event数组前移覆盖
void CompressEvents(WSAEVENT hEventArr[], int idx, int total)
{
    int i;
    // 重复处理任务，直到程序结束或满足退出条件。
    for(i = idx; i < total; i++)
        hEventArr[i] = hEventArr[i+1];
}

// 统一处理错误，避免每个位置重复写相同的退出代码。
void ErrorHandling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}