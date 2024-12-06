#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <pthread.h>
#pragma comment(lib,"pthreadVC2.lib")
#include <iostream>
#include <string>
#include <atomic>
using namespace std;
SOCKET sock;
pthread_t receivethread;
bool keep = true;
// 接收消息的线程函数
void* receiveMessages(void* arg) {
    char recvBuf[256]; // 接收缓冲区
    while (true) {
        int recvResult = recv(sock, recvBuf, sizeof(recvBuf) - 1, 0);
        if (recvResult > 0) {
            recvBuf[recvResult] = '\0'; // 添加字符串结束符
            cout << "收到消息: " << recvBuf << endl;
            cout << "请输入要发送的消息（输入exit结束）: " << endl;
        }
        else if (recvResult == 0) {
            cout << "服务器关闭连接" << endl;
            keep = false;
            // 等待接收线程结束
            pthread_join(receivethread, nullptr);
            // 清理WinSock
            closesocket(sock);
            break;
        }
        else {
            cout << "接收消息失败" << endl;
            keep = false;
            // 等待接收线程结束
            pthread_join(receivethread, nullptr);
            // 清理WinSock
            closesocket(sock);
            break;
        }
    }
    pthread_exit(nullptr);
    return nullptr;
}
int main() {
    // 初始化WinSock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cout << "WSAstartup失败" << endl;
        return 0;
    }
    // 创建socket,使用IPV4地址，流式套接字和TCP协议
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        cout << "socket创建失败" << endl;
        WSACleanup();
        closesocket(sock);
        return 0;
    }

    // 使用connect()连接服务器
    sockaddr_in service;
    service.sin_family = AF_INET;
    struct in_addr ip_addr;
    service.sin_port = htons(8888);//服务器端口号
    // 将IP地址转换为网络字节顺序
    inet_pton(AF_INET, "127.0.0.1", &service.sin_addr); // 使用IPv4地址
    result = connect(sock, (struct sockaddr*)&service, sizeof(service));
    if (result == SOCKET_ERROR) {
        cout << "连接服务器失败" << endl;
        closesocket(sock);
        WSACleanup();
        return 0;
    }
    cout << "请输入您的客户端名称: ";
    string clientname;
    getline(cin, clientname); // 获取客户端名称

    // 发送客户端名称到服务器
    send(sock, clientname.c_str(), clientname.length(), 0);
    // 创建接收消息的线程
    pthread_create(&receivethread, nullptr, receiveMessages, nullptr);
    //send()和recv()
    string message;
    while (keep) {
        cout << "请输入要发送的消息（输入exit结束）: " << endl;
        getline(cin, message); // 使用 getline 允许空格
        send(sock, message.c_str(), message.length(), 0);
        // 如果用户输入 "exit"，则退出循环
        if (message == "exit") {
            break;
        }
    }

    // 等待接收线程结束
    pthread_join(receivethread, nullptr);
    // 清理WinSock
    if (sock != INVALID_SOCKET) {
        closesocket(sock); // 确保socket被关闭
    }
    WSACleanup();
    return 0;
}