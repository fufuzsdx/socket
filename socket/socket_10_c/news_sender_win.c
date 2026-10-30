/*
 * 本文件用途：广播消息：让多个接收者收到同一条网络消息。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>   // IP_MULTICAST_TTL 需要这个头文件

#define TTL 64
#define BUF_SIZE 30
void ErrorHandling(char *message);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    SOCKET hSendSock;
    SOCKADDR_IN mulAdr;
    int timeLive=TTL;
    FILE *fp;
    char buf[BUF_SIZE];

    if(argc!=3)
    {
        printf("Usage : %s <GroupIP> <PORT>\n", argv[0]);
        exit(1);
    }

    // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
    if(WSAStartup(MAKEWORD(2, 2), &wsaData)!=0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 创建UDP套接字 SOCK_DGRAM
    hSendSock=socket(PF_INET, SOCK_DGRAM, 0);

    memset(&mulAdr, 0, sizeof(mulAdr));
    mulAdr.sin_family=AF_INET;
    mulAdr.sin_addr.s_addr=inet_addr(argv[1]);  // 组播地址
    mulAdr.sin_port=htons(atoi(argv[2]));

    // ========= 设置IP多播TTL =========
    setsockopt(hSendSock, IPPROTO_IP,
        IP_MULTICAST_TTL, (void*)&timeLive, sizeof(timeLive));

    // 打开news.txt文件，逐行读取发送
    if((fp=fopen("news.txt", "r"))==NULL)
        ErrorHandling("fopen() error");

    // 重复处理任务，直到程序结束或满足退出条件。
    while(!feof(fp))
    {
        fgets(buf, BUF_SIZE, fp);
        // UDP发送：sendto发送到组播地址
        sendto(hSendSock, buf, strlen(buf),
            0, (SOCKADDR*)&mulAdr, sizeof(mulAdr));
        Sleep(2000);  // 每隔2秒发送一行
    }

    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hSendSock);
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