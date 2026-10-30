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
    SOCKET hServSock, hClntSock;
    FILE * fp;
    char buf[BUF_SIZE];
    int readCnt;

    SOCKADDR_IN servAdr, clntAdr;
    int clntAdrSz;

    // 判断命令行参数，运行时需要传入端口号
    if(argc!=2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // ✅ Winsock库初始化（Windows socket必须！Linux不需要这一步）
    if(WSAStartup(MAKEWORD(2, 2), &wsaData)!=0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // ✅ 以二进制只读模式打开【服务器自己的源码文件】作为待发送文件
    fp=fopen("file_server_win.c", "rb");

    // ✅ 创建TCP套接字
    hServSock=socket(PF_INET, SOCK_STREAM, 0);

    // ✅ 填充服务器地址结构体
    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family=AF_INET;
    servAdr.sin_addr.s_addr=htonl(INADDR_ANY); // 监听本机所有网卡IP
    servAdr.sin_port=htons(atoi(argv[1]));    // 端口号转网络字节序

    // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
    bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr));
    // 开始等待别人发起连接；这里的数字表示允许暂时排队的连接数。
    listen(hServSock, 5);

    // ✅ 阻塞等待客户端连接
    clntAdrSz=sizeof(clntAdr);
    // 取出一个等待中的客户端连接，得到一条专门与它通信的新通道。
    hClntSock=accept(hServSock, (SOCKADDR*)&clntAdr, &clntAdrSz);

    // ========= 文件发送循环 =========
    while(1)
    {
        // 从本地文件读取最多 BUF_SIZE 字节到缓冲区buf
        readCnt=fread((void*)buf, 1, BUF_SIZE, fp);
        if(readCnt<BUF_SIZE)  // 读到文件末尾了
        {
            // 把缓冲区中的数据交给网络发送给对方。
            send(hClntSock, buf, readCnt, 0);
            break;
        }
        // 把缓冲区中的数据交给网络发送给对方。
        send(hClntSock, buf, BUF_SIZE, 0);
    }

    // ✅ shutdown关闭【发送方向】，发送FIN包，告知客户端：文件数据已经发送完毕
    shutdown(hClntSock, SD_SEND);

    // ✅ 阻塞接收客户端回发的消息 "Thank you"
    recv(hClntSock, buf, BUF_SIZE, 0);
    printf("Message from client: %s \n", buf);

    // ✅ 资源释放
    fclose(fp);
    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hClntSock);
    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hServSock);
    // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
    WSACleanup();
    return 0;
}

// 错误处理函数：打印错误信息并且退出程序
void ErrorHandling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}