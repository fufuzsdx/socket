/*
 * 本文件用途：广播消息：让多个接收者收到同一条网络消息。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>   // struct ip_mreq

#define BUF_SIZE 30
void ErrorHandling(char *message);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    SOCKET hRecvSock;
    SOCKADDR_IN adr;
    struct ip_mreq joinAdr;   // ✅组播加入结构体 ip_mreq
    char buf[BUF_SIZE];
    int strLen;

    if(argc!=3)
    {
        printf("Usage : %s <GroupIP> <PORT>\n", argv[0]);
        exit(1);
    }

    // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
    if(WSAStartup(MAKEWORD(2, 2), &wsaData)!=0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 创建UDP套接字
    hRecvSock=socket(PF_INET, SOCK_DGRAM, 0);

    memset(&adr, 0, sizeof(adr));
    adr.sin_family=AF_INET;
    adr.sin_addr.s_addr=htonl(INADDR_ANY);
    adr.sin_port=htons(atoi(argv[2]));

    // bind绑定端口，接收UDP报文
    if(bind(hRecvSock, (SOCKADDR*) &adr, sizeof(adr))==SOCKET_ERROR)
        // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
        ErrorHandling("bind() error");

    // ========== 加入多播组核心代码 ==========
    joinAdr.imr_multiaddr.s_addr=inet_addr(argv[1]);   //想要加入的组播地址
    joinAdr.imr_interface.s_addr=htonl(INADDR_ANY);     //本机任意网卡
    setsockopt(hRecvSock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
        (void*)&joinAdr, sizeof(joinAdr));

    // 重复处理任务，直到程序结束或满足退出条件。
    while(1)
    {
        // recvfrom接收UDP组播报文
        strLen=recvfrom(hRecvSock, buf, BUF_SIZE-1, 0, NULL, 0);
        if(strLen<0)
            break;
        buf[strLen]=0;
        fputs(buf, stdout);
    }

    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hRecvSock);
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