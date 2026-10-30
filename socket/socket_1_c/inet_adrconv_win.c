/*
 * 本文件用途：网络基础：地址转换、字节顺序，以及最简单的 TCP 客户端和服务器。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <stdlib.h>

void ErrorHandling(char* message);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
    if(WSAStartup(MAKEWORD(2, 2), &wsaData)!=0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    /* inet_addr函数调用示例：点分字符串 -> 网络序uint32 */
    {
        char *addr="127.212.124.78";
        unsigned long conv_addr=inet_addr(addr);
        if(conv_addr==INADDR_NONE)
            printf("Error occured! \n");
        else
            printf("Network ordered integer addr: %#lx \n", conv_addr);
    }

    /* inet_ntoa函数调用示例：网络序整数 -> 点分字符串 */
    {
        struct sockaddr_in addr;
        char *strPtr;
        char strArr[20];
        addr.sin_addr.s_addr=htonl(0x1020304);
        strPtr=inet_ntoa(addr.sin_addr);
        strcpy(strArr, strPtr);
        printf("Dotted-Decimal notation3 %s \n", strArr);
    }

    // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
    WSACleanup();
    return 0;
}

// 统一处理错误，避免每个位置重复写相同的退出代码。
void ErrorHandling(char* message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}