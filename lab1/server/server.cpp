#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <pthread.h>
#pragma comment(lib,"pthreadVC2.lib")
#include <iostream>
#include <vector>
#include <mutex>
#include <ctime>
#include <stdio.h>
using namespace std;
//客户信息
struct clientmessage {
    SOCKET clientsocket;
    string name;
};
//客户端列表和数量
vector<clientmessage> clientlist = {};
atomic<int> clientnumber(0); // 使用原子操作确保线程安全

void broadcast(const string& message, SOCKET sendersocket, const string& senderName) {
    // 获取当前时间的时间戳
    time_t now = chrono::system_clock::to_time_t(chrono::system_clock::now());
    // 使用 localtime_s 来转换时间戳
    tm localtime;
    localtime_s(&localtime, &now);
    // 将时间戳转换为字符串
    char timeBuffer[64];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &localtime);
    for (auto& client : clientlist) {
        if (client.clientsocket != sendersocket) { // 不向发送者自己发送消息
            // 构建完整的消息
            string fullMessage = senderName + " @ " + timeBuffer + ": " + message;
            send(client.clientsocket, fullMessage.c_str(), fullMessage.length(), 0);
        }
    }
    cout << "客户端[" << senderName << "]在" << timeBuffer << " 发送消息: " << message << endl;
}

DWORD WINAPI handle(LPVOID lparam) {
    SOCKET clientsocket = (SOCKET)(LPVOID)lparam;
    
    char message[256]; // 接收缓冲区

    //接收客户端名字
    int result = recv(clientsocket, message, sizeof(message) - 1, 0);
    if (result > 0) {
        message[result] = '\0'; // 添加字符串结束符
        string clientname = message;
        bool found = false;
        for (auto& client : clientlist) {
            if (client.clientsocket == clientsocket) {
                client.name = clientname;
                cout << "客户端 " << client.name << " 连接成功" << endl;
                found = true;
                break;
            }
        }
        if(!found){
            clientlist.push_back({ clientsocket, message });
            clientnumber++;
            cout << "新客户端 " << message << " 连接成功（目前有聊天室人数为：" << clientnumber <<")"<< endl;
        }
    }else {
        cout << "接收消息失败，错误码: " << WSAGetLastError() << endl;
        closesocket(clientsocket);
        return 0;
    }

    // 接收消息并广播给其他客户端
    while (true) {
        memset(message, 0, sizeof(message));
        result = recv(clientsocket, message, sizeof(message) - 1, 0);
        if (result > 0) {    
            message[result] = '\0'; // 添加字符串结束符
            string sendername;
            bool found = false;
            for (auto& client : clientlist) {
                if (client.clientsocket == clientsocket) {
                    sendername=client.name ;
                    found = true;
                    break;
                }
            }

            if (!found) {
                cout << "无法找到客户端信息" << endl;
                closesocket(clientsocket);
                return 0;
            }

            if (strcmp(message, "exit") == 0) {
                cout << "客户端 " << sendername << " 已关闭连接" << endl;
                broadcast("已退出", clientsocket, sendername);
                break;
            }
            else {
                broadcast(message, clientsocket, sendername);
            }
        }
        else {
            cout << "接收消息失败 " << WSAGetLastError() << endl;
            break;
        }
    }
    // 移除客户端
    for (auto i = clientlist.begin(); i != clientlist.end(); ) {
        if (i->clientsocket == clientsocket) {
            cout << "客户端 " << i->name << " 已从客户端列表中移除" << endl;
            i = clientlist.erase(i);
            clientnumber--;
            cout << "目前聊天室的人数为：" << clientnumber << endl;
            break;
        }
        else {
            i++;
        }
    }
    closesocket(clientsocket);
    return 0;
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
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        cout << "socket创建失败" << endl;
        WSACleanup();
        closesocket(sock);
        return 0;
    }

    // 使用bind()将套接字绑定一个IP地址和端口号
    sockaddr_in service;
    service.sin_family = AF_INET;
    struct in_addr ip_addr;
    service.sin_addr.s_addr = inet_pton(AF_INET6, "127.0.0.1", &ip_addr);//IP地址
    service.sin_port = htons(8888);//端口号
    result = bind(sock, (SOCKADDR*)&service, sizeof(service));
    if (result == SOCKET_ERROR) {
        cout << "socket绑定IP与端口号失败" << endl;
        closesocket(sock);
        WSACleanup();
        return 0;
    }
    //监听
    result = listen(sock, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        cout << "socket监听失败" << endl;
        closesocket(sock);
        WSACleanup();
        return 0;
    }
    cout << "正在监听客户端的连接..." << endl;
    //接受
    while (true) {
        //创建新的socket为一个线程
        sockaddr_in clientaddr;
        int addrlen = sizeof(clientaddr);
        SOCKET clientsocket = accept(sock, (SOCKADDR*)&clientaddr, &addrlen);
        if (clientsocket == INVALID_SOCKET) {
            cout << "接受连接失败" << endl;
            continue; // 继续监听下一个连接
        }

        // 创建新的线程处理客户端请求
        DWORD threadid;
        HANDLE hthread = CreateThread(NULL, 0, handle, (LPVOID)clientsocket, 0, &threadid);
        if (hthread == NULL) {
            cout << "线程创建失败" << endl;
            closesocket(clientsocket); 
        }
        else {
            CloseHandle(hthread); 
        }
        
    }

    // 关闭所有客户端连接
    for (auto& client : clientlist) {
        closesocket(client.clientsocket);
    }
    // 清理WinSock
    closesocket(sock);
    WSACleanup();
    return 0;
}