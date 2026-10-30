/*
 * 本文件用途：主机名和地址查询：把机器名称与网络地址互相转换。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

void ErrorHandling(char *message);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    int i;
    struct hostent *host;
    SOCKADDR_IN addr;

    if(argc!=2) {
        printf("Usage : %s <IP>\n", argv[0]);
        exit(1);
    }

    // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
    if(WSAStartup(MAKEWORD(2, 2), &wsaData)!=0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    memset(&addr, 0, sizeof(addr));
    // 将点分十进制IP字符串转为网络字节序二进制IP
    addr.sin_addr.s_addr = inet_addr(argv[1]);

    // ✅核心函数 gethostbyaddr
    // 参数1：二进制IP地址指针，参数2：IP长度(IPv4是4字节)，参数3：地址族AF_INET
    host=gethostbyaddr((char*)&addr.sin_addr, 4, AF_INET);

    if(!host)
        ErrorHandling("gethost... error");

    printf("Official name: %s \n", host->h_name);
    // 重复处理任务，直到程序结束或满足退出条件。
    for(i=0; host->h_aliases[i]; i++)
        printf("Aliases %d: %s \n", i+1, host->h_aliases[i]);

    printf("Address type: %s \n",
        (host->h_addrtype==AF_INET)?"AF_INET":"AF_INET6");

    // 重复处理任务，直到程序结束或满足退出条件。
    for(i=0; host->h_addr_list[i]; i++)
        printf("IP addr %d: %s \n", i+1,
            inet_ntoa(*(struct in_addr*)host->h_addr_list[i]));

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