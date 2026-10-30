/*
 * 本文件用途：简易网页服务器：接收浏览器请求并返回本地文件。
 * 阅读提示：把套接字想成网络插座，把缓冲区想成临时放数据的盒子。
 * 下面的注释按程序执行顺序说明“为什么这样做”，尽量使用日常说法。
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>

#define BUF_SIZE 2048
#define BUF_SMALL 100

unsigned WINAPI RequestHandler(void* arg);
char* ContentType(char* file);
void SendData(SOCKET sock, char* ct, char* fileName);
void SendErrorMSG(SOCKET sock);
void ErrorHandling(char *message);

// 主函数：按顺序准备资源、完成通信，再清理资源。
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    SOCKET hServSock, hClntSock;
    SOCKADDR_IN servAdr, clntAdr;

    HANDLE hThread;
    DWORD dwThreadID;
    int clntAdrSize;

    if(argc!=2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
    if(WSAStartup(MAKEWORD(2, 2), &wsaData)!=0)
        // 启动 Windows 的网络功能；后面的网络函数都要先完成这一步。
        ErrorHandling("WSAStartup() error!");

    // 创建通信插座；可以把它理解成程序与网络之间的一个接口。
    hServSock=socket(PF_INET, SOCK_STREAM, 0);
    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family=AF_INET;
    servAdr.sin_addr.s_addr=htonl(INADDR_ANY);
    servAdr.sin_port=htons(atoi(argv[1]));

    // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
    if(bind(hServSock, (SOCKADDR*) &servAdr, sizeof(servAdr))==SOCKET_ERROR)
        // 把套接字和本机的端口绑在一起，让外部知道从哪里找到服务器。
        ErrorHandling("bind() error");
    // 开始等待别人发起连接；这里的数字表示允许暂时排队的连接数。
    if(listen(hServSock, 5)==SOCKET_ERROR)
        // 开始等待别人发起连接；这里的数字表示允许暂时排队的连接数。
        ErrorHandling("listen() error");

    /*请求及响应*/
    while(1)
    {
        clntAdrSize=sizeof(clntAdr);
        // 取出一个等待中的客户端连接，得到一条专门与它通信的新通道。
        hClntSock=accept(hServSock, (SOCKADDR*)&clntAdr, &clntAdrSize);
        printf("Connection Request : %s:%d\n",
            inet_ntoa(clntAdr.sin_addr), ntohs(clntAdr.sin_port));
        // 创建一个新的执行任务，让这部分工作可以和主任务同时进行。
        hThread=(HANDLE)_beginthreadex(
            NULL, 0, RequestHandler, (void*)hClntSock, 0, (unsigned *)&dwThreadID);
    }
    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(hServSock);
    // 程序结束前关闭网络功能，释放 Windows 分配的网络资源。
    WSACleanup();
    return 0;
}

// 这个函数负责完成“RequestHandler”这一步工作。
unsigned WINAPI RequestHandler(void *arg)
{
    SOCKET hClntSock=(SOCKET)arg;
    char buf[BUF_SIZE];
    char method[BUF_SMALL];
    char ct[BUF_SMALL];
    char fileName[BUF_SMALL];

    // 从网络读取数据，并把收到的内容放入缓冲区。
    recv(hClntSock, buf, BUF_SIZE, 0);   // 查看是否为HTTP提出的请求
    if(strstr(buf, "HTTP/")==NULL)
    {
        SendErrorMSG(hClntSock);
        // 关闭网络通道；关闭后不能再通过它收发数据。
        closesocket(hClntSock);
        return 1;
    }

    strcpy(method, strtok(buf, " "));      // 查看是否为GET方式的请求
    if(strcmp(method, "GET"))
        SendErrorMSG(hClntSock);

    strcpy(fileName, strtok(NULL, " "));    // 查看请求文件名
    strcpy(ct, ContentType(fileName));      // 查看Content-type
    SendData(hClntSock, ct, fileName);      // 响应
    return 0;
}

// 这个函数负责完成“SendData”这一步工作。
void SendData(SOCKET sock, char* ct, char* fileName)
{
    char protocol[]="HTTP/1.0 200 OK\r\n";
    char servName[]="Server:simple web server\r\n";
    char cntLen[]="Content-length:2048\r\n";
    char cntType[BUF_SMALL];
    char buf[BUF_SIZE];
    FILE* sendFile;

    sprintf(cntType, "Content-type:%s\r\n\r\n", ct);
    if((sendFile=fopen(fileName, "r"))==NULL)
    {
        SendErrorMSG(sock);
        return;
    }

    /*传输头信息*/
    send(sock, protocol, strlen(protocol), 0);
    // 把缓冲区中的数据交给网络发送给对方。
    send(sock, servName, strlen(servName), 0);
    // 把缓冲区中的数据交给网络发送给对方。
    send(sock, cntLen, strlen(cntLen), 0);
    // 把缓冲区中的数据交给网络发送给对方。
    send(sock, cntType, strlen(cntType), 0);

    /*传输请求数据*/
    while(fgets(buf, BUF_SIZE, sendFile)!=NULL)
        // 把缓冲区中的数据交给网络发送给对方。
        send(sock, buf, strlen(buf), 0);

    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(sock);   // 由HTTP协议响应后断开
}

// 统一处理错误，避免每个位置重复写相同的退出代码。
void SendErrorMSG(SOCKET sock)    // 发生错误时传递消息
{
    char protocol[]="HTTP/1.0 400 Bad Request\r\n";
    char servName[]="Server:simple web server\r\n";
    char cntLen[]="Content-length:2048\r\n";
    char cntType[]="Content-type:text/html\r\n\r\n";
    char content[]="<html><head><title>NETWORK</title></head>"
        "<body><font size=+5><br> 发生错误! 查看请求文件名和请求方式! "
        "</font></body></html>";

    // 把缓冲区中的数据交给网络发送给对方。
    send(sock, protocol, strlen(protocol), 0);
    // 把缓冲区中的数据交给网络发送给对方。
    send(sock, servName, strlen(servName), 0);
    // 把缓冲区中的数据交给网络发送给对方。
    send(sock, cntLen, strlen(cntLen), 0);
    // 把缓冲区中的数据交给网络发送给对方。
    send(sock, cntType, strlen(cntType), 0);
    // 把缓冲区中的数据交给网络发送给对方。
    send(sock, content, strlen(content), 0);

    // 关闭网络通道；关闭后不能再通过它收发数据。
    closesocket(sock);
}

// 这个函数负责完成“ContentType”这一步工作。
char* ContentType(char* file)    // 区分Content-type
{
    char extension[BUF_SMALL];
    char fileName[BUF_SMALL];
    strcpy(fileName, file);
    strtok(fileName, ".");
    strcpy(extension, strtok(NULL, "."));

    if(!strcmp(extension, "html")||!strcmp(extension, "htm"))
        return "text/html";
    else
        return "text/plain";
}

// 统一处理错误，避免每个位置重复写相同的退出代码。
void ErrorHandling(char* message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}