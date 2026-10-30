/*
 * 本文件用途：select 多路等待：一个服务器同时照看多个客户端。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define BUF_SIZE 1024

void ErrorHandling(char *message);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    SOCKET hServSock, hClntSock;
    SOCKADDR_IN servAdr, clntAdr;
    TIMEVAL timeout;
    fd_set reads, cpyReads;

    int adrSz;
    int strLen, fdNum, i;
    char buf[BUF_SIZE];

    if(argc!=2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    // 初始化Winsock库！Windows套接字必须执行WSAStartup
    if(WSAStartup(MAKEWORD(2, 2), &wsaData)!=0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 创建监听套接字，TCP
    hServSock=socket(PF_INET, SOCK_STREAM, 0);

    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family=AF_INET;
    servAdr.sin_addr.s_addr=htonl(INADDR_ANY);
    servAdr.sin_port=htons(atoi(argv[1]));

    // bind绑定端口
    if(bind(hServSock, (SOCKADDR*) &servAdr, sizeof(servAdr))==SOCKET_ERROR)
        // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
        ErrorHandling("bind() error");

    // listen开启监听
    if(listen(hServSock, 5)==SOCKET_ERROR)
        // 开始等待别人发起连接；这里的数字表示允许暂时排队的连接数。
        ErrorHandling("listen() error");

    FD_ZERO(&reads);
    FD_SET(hServSock, &reads);   // 将服务器监听socket加入reads读集合

    // 重复处理任务，直到程序结束或满足退出条件。
    while(1)
    {
        cpyReads=reads;   // 复制一份fd集合，select会修改集合！
        timeout.tv_sec=5;
        timeout.tv_usec=5000;

        // Windows select：第一个参数直接填0，Windows忽略这个参数！
        fdNum=select(0, &cpyReads, 0, 0, &timeout);

        if(fdNum==SOCKET_ERROR)
            break;
        if(fdNum==0)   // 返回0代表超时，没有事件发生
            continue;

        // 遍历fd_set里面所有socket，查找哪些套接字触发事件
        for(i=0; i<reads.fd_count; i++)
        {
            if(FD_ISSET(reads.fd_array[i], &cpyReads))
            {
                if(reads.fd_array[i]==hServSock)
                {
                    // ==========监听socket触发可读事件：代表有新客户端连接==========
                    adrSz=sizeof(clntAdr);
                    // 取出一个等待中的客户端连接，得到一条专门与它通信的新通道。
                    hClntSock=accept(hServSock, (SOCKADDR*)&clntAdr, &adrSz);
                    FD_SET(hClntSock, &reads);  // 将新连接客户端socket加入reads集合，后续监听这个客户端的数据
                    printf("connected client: %d \n", hClntSock);
                }
                else
                {
                    // =========普通客户端socket可读：收到客户端发来的数据=========
                    strLen=recv(reads.fd_array[i], buf, BUF_SIZE-1, 0);

                    if(strLen==0)
                    {
                        // recv返回0：客户端正常断开连接
                        FD_CLR(reads.fd_array[i], &reads);   // 将该socket从监听集合移除
                        // 关闭网络通道；关闭后不能再通过它收发数据。
                        closesocket(cpyReads.fd_array[i]);   // 关闭套接字
                        printf("closed client: %d \n", cpyReads.fd_array[i]);
                    }
                    else
                    {
                        // echo回声：收到什么，就发送回客户端
                        send(reads.fd_array[i], buf, strLen, 0);
                    }
                }
            }
        }
    }

    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hServSock);
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