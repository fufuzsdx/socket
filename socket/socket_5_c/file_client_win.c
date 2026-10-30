/*
 * 本文件用途：TCP 文件传输：客户端和服务器之间传送文件内容。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define BUF_SIZE 30
void ErrorHandling(char *message);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    SOCKET hSocket;
    FILE *fp;
    char buf[BUF_SIZE];
    int readCnt;

    SOCKADDR_IN servAdr;

    // 参数：客户端运行需要传入服务器IP + 端口号
    if(argc!=3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // Winsock初始化
    if(WSAStartup(MAKEWORD(2, 2), &wsaData)!=0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // ✅ 以二进制写模式打开 receive.dat，用来保存收到的文件
    fp=fopen("receive.dat", "wb");

    // 创建TCP套接字
    hSocket=socket(PF_INET, SOCK_STREAM, 0);

    // 填充服务器地址
    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family=AF_INET;
    servAdr.sin_addr.s_addr=inet_addr(argv[1]);
    servAdr.sin_port=htons(atoi(argv[2]));

    // ✅ 向服务器发起TCP连接
    connect(hSocket, (SOCKADDR*)&servAdr, sizeof(servAdr));

    // ========= 文件接收循环 =========
    // recv返回0：代表对方调用shutdown关闭发送方向，数据流结束
    while( (readCnt=recv(hSocket, buf, BUF_SIZE, 0)) !=0 )
    {
        fwrite(buf, 1, readCnt, fp); // 将收到的数据写入本地文件receive.dat
    }

    puts("Received file data");
    // 文件接收完毕，客户端发送应答消息给服务器
    send(hSocket, "Thank you", 10, 0);

    // 资源释放
    fclose(fp);
    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hSocket);
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