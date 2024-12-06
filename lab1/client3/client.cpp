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
// ������Ϣ���̺߳���
void* receiveMessages(void* arg) {
    char recvBuf[256]; // ���ջ�����
    while (true) {
        int recvResult = recv(sock, recvBuf, sizeof(recvBuf) - 1, 0);
        if (recvResult > 0) {
            recvBuf[recvResult] = '\0'; // ����ַ���������
            cout << "�յ���Ϣ: " << recvBuf << endl;
            cout << "������Ҫ���͵���Ϣ������exit������: " << endl;
        }
        else if (recvResult == 0) {
            cout << "�������ر�����" << endl;
            keep = false;
            // �ȴ������߳̽���
            pthread_join(receivethread, nullptr);
            // ����WinSock
            closesocket(sock);
            break;
        }
        else {
            cout << "������Ϣʧ��" << endl;
            keep = false;
            // �ȴ������߳̽���
            pthread_join(receivethread, nullptr);
            // ����WinSock
            closesocket(sock);
            break;
        }
    }
    pthread_exit(nullptr);
    return nullptr;
}
int main() {
    // ��ʼ��WinSock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cout << "WSAstartupʧ��" << endl;
        return 0;
    }
    // ����socket,ʹ��IPV4��ַ����ʽ�׽��ֺ�TCPЭ��
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        cout << "socket����ʧ��" << endl;
        WSACleanup();
        closesocket(sock);
        return 0;
    }

    // ʹ��connect()���ӷ�����
    sockaddr_in service;
    service.sin_family = AF_INET;
    struct in_addr ip_addr;
    service.sin_port = htons(8888);//�������˿ں�
    // ��IP��ַת��Ϊ�����ֽ�˳��
    inet_pton(AF_INET, "127.0.0.1", &service.sin_addr); // ʹ��IPv4��ַ
    result = connect(sock, (struct sockaddr*)&service, sizeof(service));
    if (result == SOCKET_ERROR) {
        cout << "���ӷ�����ʧ��" << endl;
        closesocket(sock);
        WSACleanup();
        return 0;
    }
    cout << "���������Ŀͻ�������: ";
    string clientname;
    getline(cin, clientname); // ��ȡ�ͻ�������

    // ���Ϳͻ������Ƶ�������
    send(sock, clientname.c_str(), clientname.length(), 0);
    // ����������Ϣ���߳�
    pthread_create(&receivethread, nullptr, receiveMessages, nullptr);
    //send()��recv()
    string message;
    while (keep) {
        cout << "������Ҫ���͵���Ϣ������exit������: " << endl;
        getline(cin, message); // ʹ�� getline ����ո�
        send(sock, message.c_str(), message.length(), 0);
        // ����û����� "exit"�����˳�ѭ��
        if (message == "exit") {
            break;
        }
    }

    // �ȴ������߳̽���
    pthread_join(receivethread, nullptr);
    // ����WinSock
    if (sock != INVALID_SOCKET) {
        closesocket(sock); // ȷ��socket���ر�
    }
    WSACleanup();
    return 0;
}