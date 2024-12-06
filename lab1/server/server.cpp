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
//�ͻ���Ϣ
struct clientmessage {
    SOCKET clientsocket;
    string name;
};
//�ͻ����б������
vector<clientmessage> clientlist = {};
atomic<int> clientnumber(0); // ʹ��ԭ�Ӳ���ȷ���̰߳�ȫ

void broadcast(const string& message, SOCKET sendersocket, const string& senderName) {
    // ��ȡ��ǰʱ���ʱ���
    time_t now = chrono::system_clock::to_time_t(chrono::system_clock::now());
    // ʹ�� localtime_s ��ת��ʱ���
    tm localtime;
    localtime_s(&localtime, &now);
    // ��ʱ���ת��Ϊ�ַ���
    char timeBuffer[64];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &localtime);
    for (auto& client : clientlist) {
        if (client.clientsocket != sendersocket) { // ���������Լ�������Ϣ
            // ������������Ϣ
            string fullMessage = senderName + " @ " + timeBuffer + ": " + message;
            send(client.clientsocket, fullMessage.c_str(), fullMessage.length(), 0);
        }
    }
    cout << "�ͻ���[" << senderName << "]��" << timeBuffer << " ������Ϣ: " << message << endl;
}

DWORD WINAPI handle(LPVOID lparam) {
    SOCKET clientsocket = (SOCKET)(LPVOID)lparam;
    
    char message[256]; // ���ջ�����

    //���տͻ�������
    int result = recv(clientsocket, message, sizeof(message) - 1, 0);
    if (result > 0) {
        message[result] = '\0'; // ����ַ���������
        string clientname = message;
        bool found = false;
        for (auto& client : clientlist) {
            if (client.clientsocket == clientsocket) {
                client.name = clientname;
                cout << "�ͻ��� " << client.name << " ���ӳɹ�" << endl;
                found = true;
                break;
            }
        }
        if(!found){
            clientlist.push_back({ clientsocket, message });
            clientnumber++;
            cout << "�¿ͻ��� " << message << " ���ӳɹ���Ŀǰ������������Ϊ��" << clientnumber <<")"<< endl;
        }
    }else {
        cout << "������Ϣʧ�ܣ�������: " << WSAGetLastError() << endl;
        closesocket(clientsocket);
        return 0;
    }

    // ������Ϣ���㲥�������ͻ���
    while (true) {
        memset(message, 0, sizeof(message));
        result = recv(clientsocket, message, sizeof(message) - 1, 0);
        if (result > 0) {    
            message[result] = '\0'; // ����ַ���������
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
                cout << "�޷��ҵ��ͻ�����Ϣ" << endl;
                closesocket(clientsocket);
                return 0;
            }

            if (strcmp(message, "exit") == 0) {
                cout << "�ͻ��� " << sendername << " �ѹر�����" << endl;
                broadcast("���˳�", clientsocket, sendername);
                break;
            }
            else {
                broadcast(message, clientsocket, sendername);
            }
        }
        else {
            cout << "������Ϣʧ�� " << WSAGetLastError() << endl;
            break;
        }
    }
    // �Ƴ��ͻ���
    for (auto i = clientlist.begin(); i != clientlist.end(); ) {
        if (i->clientsocket == clientsocket) {
            cout << "�ͻ��� " << i->name << " �Ѵӿͻ����б����Ƴ�" << endl;
            i = clientlist.erase(i);
            clientnumber--;
            cout << "Ŀǰ�����ҵ�����Ϊ��" << clientnumber << endl;
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
    // ��ʼ��WinSock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cout << "WSAstartupʧ��" << endl;
        return 0;
    }

    // ����socket,ʹ��IPV4��ַ����ʽ�׽��ֺ�TCPЭ��
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        cout << "socket����ʧ��" << endl;
        WSACleanup();
        closesocket(sock);
        return 0;
    }

    // ʹ��bind()���׽��ְ�һ��IP��ַ�Ͷ˿ں�
    sockaddr_in service;
    service.sin_family = AF_INET;
    struct in_addr ip_addr;
    service.sin_addr.s_addr = inet_pton(AF_INET6, "127.0.0.1", &ip_addr);//IP��ַ
    service.sin_port = htons(8888);//�˿ں�
    result = bind(sock, (SOCKADDR*)&service, sizeof(service));
    if (result == SOCKET_ERROR) {
        cout << "socket��IP��˿ں�ʧ��" << endl;
        closesocket(sock);
        WSACleanup();
        return 0;
    }
    //����
    result = listen(sock, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        cout << "socket����ʧ��" << endl;
        closesocket(sock);
        WSACleanup();
        return 0;
    }
    cout << "���ڼ����ͻ��˵�����..." << endl;
    //����
    while (true) {
        //�����µ�socketΪһ���߳�
        sockaddr_in clientaddr;
        int addrlen = sizeof(clientaddr);
        SOCKET clientsocket = accept(sock, (SOCKADDR*)&clientaddr, &addrlen);
        if (clientsocket == INVALID_SOCKET) {
            cout << "��������ʧ��" << endl;
            continue; // ����������һ������
        }

        // �����µ��̴߳���ͻ�������
        DWORD threadid;
        HANDLE hthread = CreateThread(NULL, 0, handle, (LPVOID)clientsocket, 0, &threadid);
        if (hthread == NULL) {
            cout << "�̴߳���ʧ��" << endl;
            closesocket(clientsocket); 
        }
        else {
            CloseHandle(hthread); 
        }
        
    }

    // �ر����пͻ�������
    for (auto& client : clientlist) {
        closesocket(client.clientsocket);
    }
    // ����WinSock
    closesocket(sock);
    WSACleanup();
    return 0;
}