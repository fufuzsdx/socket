/*
 * 本文件用途：主机名和地址查询：把机器名称与网络地址互相转换。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

void ErrorHandling(char *message);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    int i;
    struct hostent *host;

    // 判断命令行参数数量
    if(argc!=2) {
        printf("Usage : %s <addr>\n", argv[0]);
        exit(1);
    }

    // Winsock库初始化，Windows网络编程必须
    if(WSAStartup(MAKEWORD(2, 2), &wsaData)!=0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // ✅核心函数 gethostbyname：传入域名字符串，返回hostent结构体
    host=gethostbyname(argv[1]);

    if(!host)
        ErrorHandling("gethost... error");

    // h_name：主机的正式域名 official name
    printf("Official name: %s \n", host->h_name);

    // h_aliases：别名数组，该主机其他域名
    for(i=0; host->h_aliases[i]; i++)
        printf("Aliases %d: %s \n", i+1, host->h_aliases[i]);

    // h_addrtype：地址类型 AF_INET代表IPv4
    printf("Address type: %s \n",
        (host->h_addrtype==AF_INET)?"AF_INET":"AF_INET6");

    // h_addr_list：该域名对应的IP地址列表（一个域名可以绑定多个IP）
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