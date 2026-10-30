/*
 * 本文件用途：网络基础：地址转换、字节顺序，以及最简单的 TCP 客户端和服务器。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#undef UNICODE
#undef _UNICODE
#include <stdio.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    char *strAddr="203.211.218.102:9190";
    char strAddrBuf[50];
    SOCKADDR_IN servAddr;
    int size_int;
    DWORD size_dword;

    WSADATA wsaData;
    // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    size_int = sizeof(servAddr);
    WSAStringToAddress(
        strAddr, AF_INET, NULL, (SOCKADDR*)&servAddr, &size_int);

    size_dword = sizeof(strAddrBuf);
    WSAAddressToString(
        (SOCKADDR*)&servAddr, sizeof(servAddr), NULL, strAddrBuf, &size_dword);

    printf("Second conv result: %s \n", strAddrBuf);
    // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
    WSACleanup();
    return 0;
}