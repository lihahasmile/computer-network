#include "packet.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <windows.h>
#include <string>
#include <cmath>
using namespace std;
SOCKET clientsock;
SOCKADDR_IN serveraddr;
SOCKADDR_IN clientaddr;
uint32_t clientnum = rand();//客户端序列号
uint32_t servernum;//服务器序列号
u_short serverport = 8888;//服务器端口号
u_short clientport = 9999;//客户端端口号
int slen;
int clen;
#define TIMEOUT 500  // 超时时间(ms)
#define MAX_RETRIES 5 // 最大重试次数
//三次握手
bool threeshake() {
    char* buff = new char[sizeof(Packet)];
    int shakeresult;
    int times = MAX_RETRIES;
    cout << endl;
    cout << "\033[1;36m-----开始握手----- \033[0m" << endl;
    cout << endl;
    //初始化序列号和确认号
resend:
    cout << "\033[1;33mFirst: \033[0m" << endl;
    //syn+seq
    memset(buff, 0, sizeof(buff));
    Packet onepack;
    onepack.seq_num = clientnum;
    onepack.len = 0;
    setFlag(onepack.sign, SYN);
    onepack.checksum = 0;
    onepack.checksum = checksum((u_short*)&onepack, sizeof(onepack));
    //第一次，发送syn
    memcpy(buff, &onepack, sizeof(onepack));
    //buff = serialize(onepack);
    shakeresult = sendto(clientsock, buff, sizeof(onepack), 0, (struct sockaddr*)&serveraddr, slen);
    if (shakeresult==-1) {
        cout << "\033[1;31mInfo：\033[0m 发送失败,重发......" << endl;
        goto resend;
    }
    else {
        cout << "\033[1;32mSend: \033[0m 发送连接请求成功,等待响应中..." << endl;
    }
    //设置为非阻塞状态
    u_long mode = 1;
    ioctlsocket(clientsock, FIONBIO, &mode);

    //第二次，接收服务器ACK
    Packet twopack;
    cout << endl;
    cout << "\033[1;33mSecond: \033[0m" << endl;
    //开始计时
    clock_t start = clock();
rerece:
    while (true) {
        clock_t now = clock();
        if (now - start > TIMEOUT) {
            //超时重传
            goto resend;
        }
        //第二次，接收ACK和SYN,检查ack，设置seq
        memset(buff, 0, sizeof(buff));
        shakeresult = recvfrom(clientsock, buff, sizeof(twopack), 0, (sockaddr*)&serveraddr, &slen);
        if (shakeresult == -1) {
            goto rerece;
        }
        else {//接收到消息,判断SYN与ACK
            memcpy(&twopack, buff, sizeof(twopack));
            if (ackacksyn(twopack, clientnum)) {
                servernum = twopack.seq_num;
                cout << "\033[1;32mReceive：\033[0m 接收到来自服务器的ACK..." << endl;
                break;
            }
        }
    }

    //第三次,发送ACK确认
    cout << endl;
    cout << "\033[1;33mThird: \033[0m" << endl;
    Packet threepack;
    setFlag(threepack.sign, ACK);
    threepack.ack_num = servernum + 1;
    threepack.seq_num = clientnum + 1;
    threepack.len = 0;
    threepack.checksum = 0;
    threepack.checksum = checksum((u_short*)&threepack, sizeof(threepack));
    memset(buff, 0, sizeof(buff));
    memcpy(buff, &threepack, sizeof(threepack));
    shakeresult = sendto(clientsock, buff, sizeof(threepack), 0, (struct sockaddr*)&serveraddr, slen);
    if (shakeresult == -1) {
        //ACK未发送成功
        return false;
    }
    cout << "\033[1;32mSend: \033[0m 客户端发送ACK确认......" << endl;
    mode = 0;
    ioctlsocket(clientsock, FIONBIO, &mode);
    cout << "\033[1;35mSuccess：\033[0m 服务器与客户端成功连接" << endl;
    return true;
}
// 发送文件
bool sendfile(char* message, int len, bool ifname)
{
    int sresult = 0;
    int packnum = 0; // 需要传送的数据包个数
    char* messbuf = new char[sizeof(Packet)];
    if (len % SIZE != 0) {
        packnum = len / SIZE + 1;
    }
    else {
        packnum = len / SIZE;
    }
    int seqnum = 0;
    for (int i = 0; i < packnum; i++)
    {
        int t = 0;//要发送的长度
        if (i == packnum - 1) {
            t = len - SIZE * (packnum - 1);
        }
        else {
            t = SIZE;
        }
        //发送文件
    resend:
        Packet mess;
        mess.len = t;
        mess.seq_num = seqnum; // 序列号
        mess.checksum = 0;
        for (int j = 0; j < t; j++) {
            mess.data[j] = message[j + SIZE * i];
        }
        if (ifname) {
            mess.sign = NAME;
        }
        else {
            mess.sign = 0;
        }
        mess.checksum = checksum((u_short*)&mess, sizeof(mess)); // 计算校验和
        memcpy(messbuf, &mess, sizeof(mess));
        // 发送
        sresult = sendto(clientsock, messbuf, sizeof(mess), 0, (sockaddr*)&serveraddr, slen);
        if (sresult == -1) {
            //cout << "\033[1;33mInfo:\033[0m 重新发送数据" << endl;
            goto resend;
        }
        cout << "\033[1;31mSend: \033[0m 发送了" << t << " 字节  " <<" 序列号  "<<seqnum << endl;
        clock_t start = clock();
        while (true)
        {
            u_long mode = 1;
            ioctlsocket(clientsock, FIONBIO, &mode);
            sresult = recvfrom(clientsock, messbuf, SIZE, 0, (sockaddr*)&serveraddr, &slen);
            clock_t now = clock();
            if (now - start > TIMEOUT)
            {
                cout << "\033[1;33mInfo:\033[0m 超时重新发送, 序列号  "<<seqnum << endl;
                goto resend;
            }
            Packet messack;
            memcpy(&messack, messbuf, sizeof(messack));
            if (ifcorrect(messack, sizeof(messack)) && messack.seq_num == seqnum && messack.sign == ACK)
            {
                cout << "\033[1;32mReceive:\033[0m 收到ACK, 序列号  "<<seqnum << endl;
                break;
            }
        }
        u_long mode = 0;
        ioctlsocket(clientsock, FIONBIO, &mode); // 改回阻塞模式
        seqnum++;
    }
    // 发送结束
sendover:
    Packet overpack;
    setFlag(overpack.sign, OVER);
    overpack.checksum = 0;
    overpack.checksum = checksum((u_short*)&overpack, sizeof(overpack));
    memcpy(messbuf, &overpack, sizeof(overpack));
    sresult = sendto(clientsock, messbuf, sizeof(overpack), 0, (sockaddr*)&serveraddr, slen);
    if (sresult == -1) {
        cout << "\033[1;33mInfo:\033[0m 重新发送数据" << endl;
        goto sendover;
    }
    cout << "\033[1;31mSend:\033[0m 发送OVER信号" << endl;
    clock_t start = clock();
    while (true)
    {
        u_long mode = 1;
        ioctlsocket(clientsock, FIONBIO, &mode);
        sresult = recvfrom(clientsock, messbuf, SIZE, 0, (sockaddr*)&serveraddr, &slen);
        if (clock() - start > TIMEOUT)
        {
            cout << "\033[1;33mInfo:\033[0m 超时" << endl;
            goto sendover;
        }
        memcpy(&overpack, messbuf, sizeof(overpack));
        // 收到的数据包为OVER则已经成功接受文件
        if (overpack.sign == OVER && ifcorrect(overpack, sizeof(overpack)))
        {
            cout << "\033[1;33mInfo:\033[0m 对方已成功接收文件" << endl;
            break;
        }
    }
    u_long mode = 0;
    ioctlsocket(clientsock, FIONBIO, &mode); // 改回阻塞模式
    return true;
}

// 四次挥手
bool fourbye() {
    char* byebuff = new char[sizeof(Packet)];
    int times = MAX_RETRIES;
    cout << endl;
    cout << "\033[1;36m-----开始挥手----- \033[0m" << endl;
resend:
    cout << endl;
    cout << "\033[1;33mFirst: \033[0m" << endl;
    memset(byebuff, 0, sizeof(byebuff));
    Packet fourpack;
    setFlag(fourpack.sign, FIN);
    fourpack.seq_num = clientnum;
    fourpack.checksum = 0;
    fourpack.checksum = checksum((u_short*)&fourpack, sizeof(fourpack));
    // 发送FIN包+seq
    memcpy(byebuff, &fourpack, sizeof(fourpack));
    int byeresult = sendto(clientsock, byebuff, sizeof(fourpack), 0, (struct sockaddr*)&serveraddr, slen);
    if (byeresult == -1) {
        if (times--) {
            cout << "   发送失败，重新尝试..." << endl;
            goto resend;
        }
        else {
            cout << "   重发次数使用完，仍未连接..." << endl;
            return false;
        }
    }
    cout << "\033[1;32mSend: \033[0m 发送断联请求成功,等待响应中..." << endl;
    //设置为非阻塞状态
    u_long mode = 1;
    ioctlsocket(clientsock, FIONBIO, &mode);
    //接收服务器ACK
    Packet tpack;
    times = MAX_RETRIES;
    cout << endl;
    cout << "\033[1;33mSecond: \033[0m" << endl;
    //开始计时
    clock_t start = clock();
    while (true) {
        clock_t now = clock();
        if (now - start > TIMEOUT) {
            if (times--) {
                cout << "   超时未接受到ACK，重新尝试发送请求..." << endl;
                goto resend;
            }
            else {
                cout << "   超时的重发次数用完，仍未连接..." << endl;
                return false;
            }
        }
        //第二次，接收ACK
        memset(byebuff, 0, sizeof(byebuff));
        byeresult = recvfrom(clientsock, byebuff, sizeof(tpack), 0, (struct sockaddr*)&serveraddr, &slen);
        if (byeresult > 0) {//接收到消息,判断SYN与ACK
            // 检查
            memcpy(&tpack, byebuff, sizeof(tpack));
            bool packetck = ackack(tpack, clientnum);
            if (!packetck) {
                if (times--) {
                    cout << "   未接受到ACK包，重新尝试发送请求..." << endl;
                    goto resend;
                }
                else {
                    cout << "   接收到非ACK包重发次数用完，仍未连接..." << endl;
                    return false;
                }
            }
            servernum = tpack.seq_num;
            cout << "\033[1;32mReceive：\033[0m 接收到来自服务器的ACK..." << endl;
            break;
        }
    }
    //接收服务器FIN
    Packet spack;
    times = MAX_RETRIES;
    cout << endl;
    cout << "\033[1;33mThird: \033[0m" << endl;
    //开始计时
    clock_t start2 = clock();
    while (true) {
        clock_t now2 = clock();
        if (now2 - start2 > TIMEOUT) {
            if (times--) {
                cout << "   超时未接受到ACK，重新尝试发送请求..." << endl;
                goto resend;
            }
            else {
                cout << "   超时的重发次数用完，仍未连接..." << endl;
                return false;
            }
        }
        memset(byebuff, 0, sizeof(byebuff));
        byeresult = recvfrom(clientsock, byebuff, sizeof(spack), 0, (struct sockaddr*)&serveraddr, &slen);
        if (byeresult > 0) {//接收到消息,判断SYN与ACK
            // 检查,fin,seq
            memcpy(&spack, byebuff, sizeof(spack));
            bool packetck = finackack(spack, clientnum);
            if (!packetck) {
                if (times--) {
                    cout << "   未接受到ACK包，重新尝试发送请求..." << endl;
                    goto resend;
                }
                else {
                    cout << "   接收到非ACK包重发次数用完，仍未连接..." << endl;
                    return false;
                }
            }
            cout << "\033[1;32mReceive：\033[0m 接收到来自服务器的ACK..." << endl;
            break;
        }
    }

    //时间轮询等待
    // 
    /*cout << "Waiting for data..." << endl;
    Sleep(2 * MAX_LIVETIME);*/
    //发送ack
    cout << endl;
    cout << "\033[1;33mFour: \033[0m" << endl;
    Packet fpack;
    times = MAX_RETRIES;
    //setFlag(fpack.sign, ACK);
    fpack.sign = 1;
    fpack.ack_num = servernum + 1;
    fpack.seq_num = clientnum + 1;
    fpack.len = 0;
    fpack.checksum = 0;
    fpack.checksum = checksum((u_short*)&fpack, sizeof(fpack));
    memset(byebuff, 0, sizeof(byebuff));
    memcpy(byebuff, &fpack, sizeof(fpack));
    byeresult = sendto(clientsock, byebuff, sizeof(fpack), 0, (struct sockaddr*)&serveraddr, slen);
    if (byeresult == -1) {
        //ACK未发送成功
        if (times--) {
            cout << "   重新发送ACK..." << endl;
            goto resend;
        }
        else {
            cout << "   ACK重发次数用完..." << endl;
            return false;
        }
    }
    cout << "\033[1;32mSend: \033[0m 发送ACK......" << endl;
    cout << "\033[1;35mSuccess：\033[0m 成功断开连接...... " << endl;
    mode = 0;
    ioctlsocket(clientsock, FIONBIO, &mode);
    return true;
}
int main() {
    // 初始化WinSock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cout << "WSAstartup失败" << endl;
        return 0;
    }
    // 创建socket
    clientsock = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientsock == INVALID_SOCKET) {
        cout << "socket创建失败" << endl;
        WSACleanup();
        closesocket(clientsock);
        return 0;
    }

    //// 设置非阻塞模式
    //u_long mode = 1; // 1表示非阻塞模式，0表示阻塞模式
    //if (ioctlsocket(clientsock, FIONBIO, &mode) != NO_ERROR) {
    //    cout << "非阻塞模式建立失败" << endl;
    //    WSACleanup();
    //    closesocket(clientsock);
    //    return 0;
    //}

     // 服务器的IP地址和端口号
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(serveraddr.sin_addr));
    //serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有网络接口
    serveraddr.sin_port = htons(serverport);//端口号
    // 客户端的ip和端口号
    memset(&clientaddr, 0, sizeof(clientaddr));
    clientaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(clientaddr.sin_addr));
    serveraddr.sin_port = htons(clientport);//端口号


    slen = sizeof(serveraddr);
    clen = sizeof(clientaddr);
    //bind绑定
    result = bind(clientsock, (SOCKADDR*)&clientaddr, clen);
    if (result == SOCKET_ERROR) {
        cout << "socket绑定IP与端口号失败" << endl;
        closesocket(clientsock);
        WSACleanup();
        return 0;
    }

    //三次握手
    threeshake();
    bool ifconnect = true;
    char choice;
    while (true)
    {
        cout << endl;
        cout << "\033[1;34mChoice: \033[0m 请选择是否输入文件(Y/N)" << endl;
        cin >> choice;
        if (ifconnect && (choice == ('Y' | 'y'))) {
            //传输文件
        }
        if (choice == ('N' | 'n')) {
            ifconnect = false;
            break;
        }
        // 读取文件内容到buffer
        string file; // 希望传输的文件名称
        cout << "\033[1;34mInput: \033[0m 请输入传输的文件" << endl;
        cin >> file;
        ifstream f(file.c_str(), ifstream::binary); // 以二进制方式打开文件
        if (!f.is_open()) {
            cerr << "\033[1;31mError:\033[0m 无法打开文件" << endl;
            return false;
        }
        f.seekg(0, ios::end);
        int len = f.tellg();
        f.seekg(0, ios::beg);
        char* buffer = new char[len];
        f.read(buffer, len);
        f.close();
        // 发送文件名
        sendfile((char*)(file.c_str()), file.length(), true);
        clock_t start1 = clock();
        // 发送文件内容
        sendfile(buffer, len, false);
        clock_t end1 = clock();
        cout << "\033[1;36mOut:\033[0m 传输总时间为:" << (end1 - start1) / CLOCKS_PER_SEC * 1000 << "ms" << endl;
        cout << "\033[1;36mOut:\033[0m 吞吐率为:" << fixed << setprecision(2) << (((double)len) / ((end1 - start1) / CLOCKS_PER_SEC)) << "byte/ms" << endl;
    }
    //四次挥手
    fourbye();
    // 清理WinSock
    if (clientsock != INVALID_SOCKET) {
        closesocket(clientsock); // 确保socket被关闭
    }
    WSACleanup();
    WSACleanup();
    return 0;
}