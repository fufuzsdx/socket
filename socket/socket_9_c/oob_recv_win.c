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
    SOCKET hAcptSock, hRecvSock;

    SOCKADDR_IN recvAdr;
    SOCKADDR_IN sendAdr;
    int sendAdrSize, strLen;
    char buf[BUF_SIZE];
    int result;

    fd_set read, except, readCopy, exceptCopy;
    struct timeval timeout;

    if(argc!=2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
    if(WSAStartup(MAKEWORD(2, 2), &wsaData)!=0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 创建通信插座；可以把它理解成程序与网络之间的一个接口。
    hAcptSock=socket(PF_INET, SOCK_STREAM, 0);
    memset(&recvAdr, 0, sizeof(recvAdr));
    recvAdr.sin_family=AF_INET;
    recvAdr.sin_addr.s_addr=htonl(INADDR_ANY);
    recvAdr.sin_port=htons(atoi(argv[1]));

    // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
    if(bind(hAcptSock, (SOCKADDR*)&recvAdr, sizeof(recvAdr))==SOCKET_ERROR)
        // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
        ErrorHandling("bind() error");
    // 开始等待别人发起连接；这里的数字表示允许暂时排队的连接数。
    if(listen(hAcptSock, 5)==SOCKET_ERROR)
        // 开始等待别人发起连接；这里的数字表示允许暂时排队的连接数。
        ErrorHandling("listen() error");

    sendAdrSize=sizeof(sendAdr);
    // 取出一个等待中的客户端连接，得到一条专门与它通信的新通道。
    hRecvSock=accept(hAcptSock, (SOCKADDR*)&sendAdr, &sendAdrSize);

    FD_ZERO(&read);
    FD_ZERO(&except);
    FD_SET(hRecvSock, &read);    //将客户端socket加入read集合，监听普通数据
    FD_SET(hRecvSock, &except);  //将客户端socket加入except集合，监听OOB紧急数据

    // 重复处理任务，直到程序结束或满足退出条件。
    while(1)
    {
        readCopy=read;
        exceptCopy=except;

        timeout.tv_sec=5;
        timeout.tv_usec=0;

        // select：监听readCopy可读事件、exceptCopy异常事件
        result=select(0, &readCopy, 0, &exceptCopy, &timeout);

        if(result>0)
        {
            // ========= 检测except集合：OOB紧急数据到来 =========
            if(FD_ISSET(hRecvSock, &exceptCopy))
            {
                // recv 带上 MSG_OOB 读取紧急字节！
                strLen=recv(hRecvSock, buf, BUF_SIZE-1, MSG_OOB);
                buf[strLen]=0;
                printf("Urgent message: %s \n", buf);
            }

            // ========= 检测read集合：普通TCP流数据到来 =========
            if(FD_ISSET(hRecvSock, &readCopy))
            {
                // 从网络读取数据，并把收到的内容放入缓冲区。
                strLen=recv(hRecvSock, buf, BUF_SIZE-1, 0);
                if(strLen==0)
                {
                    break;
                    // 关闭网络通道；关闭后不能再通过它收发数据。
                    closesocket(hRecvSock);
                }
                else
                {
                    buf[strLen]=0;
                    puts(buf);
                }
            }
        }
    }

    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hAcptSock);
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