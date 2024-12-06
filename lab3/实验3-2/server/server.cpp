#include "packet.h"
#include <fstream>
#include <chrono>
#include <windows.h>
#include <string>
#include <thread>
using namespace std;
SOCKET serversock;
SOCKADDR_IN serveraddr;
SOCKADDR_IN clientaddr;
uint32_t clientnum;//客户端序列号
uint32_t servernum = rand();//服务器序列号
u_short serverport = 8888;//服务器端口号
u_short clientport = 9999;//客户端端口号
int slen;
int clen;
#define TIMEOUT 500  // 超时时间
#define MAX_RETRIES 5 // 最大重试次数
char* buffer = new char[sizeof(Packet)];
char* name = new char[20];
char* recvdata = new char[100000000];
//三次握手
bool threeshake() {
    char* buff = new char[sizeof(Packet)];
    int shakeresult;
    int times = MAX_RETRIES;
    Packet one;
    cout << endl;
    cout << "\033[1;36m-----开始握手----- \033[0m" << endl;
    cout << endl;
rerece:
    cout << "\033[1;33mFirst: \033[0m" << endl;
    while (true) {
        memset(buff, 0, sizeof(buff));
        shakeresult = recvfrom(serversock, buff, sizeof(one), 0, (sockaddr*)&clientaddr, &clen);
        if (shakeresult == -1) {
            goto rerece;
        }
        else {
            memcpy(&one, buff, sizeof(one));
            //对接收到的包进行检查
            //第一个包seq=客户端；
            if (syn(one)) {//检查syn和校验和
                clientnum = one.seq_num;
                cout << "\033[1;32mReceive: \033[0m 接收到客户端连接请求......" << endl;
                break;
            }
        }
    }
    //发送第二个包ack+syn
    cout << endl;
    cout << "\033[1;33mSecond: \033[0m" << endl;
resend:
    Packet two;
    setFlag(two.sign, ACK);
    setFlag(two.sign, SYN);
    two.ack_num = clientnum + 1;
    two.seq_num = servernum;
    two.len = 0;
    two.checksum = 0;
    two.checksum = checksum((u_short*)&two, sizeof(two));
    memset(buff, 0, sizeof(buff));
    memcpy(buff, &two, sizeof(two));
    shakeresult = sendto(serversock, buff, sizeof(two), 0, (sockaddr*)&clientaddr, clen);
    if (shakeresult == -1) {
        goto resend;
    }
    else {
        cout << "\033[1;32mSend: \033[0m 服务器发送ACK确认......" << endl;
    }
    //计时接收客户端ack
    cout << endl;
    cout << "\033[1;33mThird: \033[0m" << endl;
    Packet three;
    clock_t start = clock();
    while (true) {
        memset(buff, 0, sizeof(buff));
        shakeresult = recvfrom(serversock, buff, sizeof(three), 0, (sockaddr*)&clientaddr, &clen);
        clock_t now = clock();
        if (now - start > TIMEOUT) {
            cout << "\033[1;31mInfo：\033[0m 握手超时，客户端将重发，服务器重新接收...... " << endl;
            goto rerece;
        }
        if (shakeresult != -1) {
            break;
        }

    }
    //ack，seq=客户端+1；ack=服务器+1；
    memcpy(&three, buff, sizeof(three));
    //检查包
    if (ackseqack(three, servernum, clientnum)) {
        cout << "\033[1;32mReceive：\033[0m 接收到客户端ACK确认......" << endl;
        cout << "\033[1;35mSuccess：\033[0m 服务器与客户端成功连接" << endl;
    }
    return true;
}

int RecvMessage(bool ifname)
{
    int remsult;
    Packet messpack;
    int seq = 0;//序列号
    long int total_len = 0;//总接收长度
    while (true)
    {
        remsult = recvfrom(serversock, buffer, sizeof(messpack), 0, (sockaddr*)&clientaddr, &clen);
        if (remsult == -1)// 如果接收失败，继续尝试
        {
            continue;
        }
        else {
            memcpy(&messpack, buffer, sizeof(messpack));

            // 模拟丢包：根据丢包概率决定是否丢弃接收的数据包
            if (rand() / (double)RAND_MAX < 0.1)
            {
                cout << "\033[1;31mPacket Dropped: \033[0m: 丢弃, 序列号  " << messpack.seq_num << endl;
                continue; // 丢包，跳过当前循环
            }
            int t = int(messpack.seq_num);
            cout << "\033[1;32mReceive：\033[0m 收到了 " << messpack.len << " 序列号  " << int(messpack.seq_num) << endl;

            if (!ifcorrect(messpack, sizeof(messpack))) {
                cout << "\033[1;31mError：\033[0m 丢弃数据包" << endl;
                continue;
            }

            // 模拟接收延迟
            //this_thread::sleep_for(std::chrono::milliseconds(10));

            if (seq != t) {
                //if (seq > t) {//说明写入文件了但是客户端未收到ack
                    Packet ackpack;
                    ackpack.sign = ACK;
                    ackpack.len = 0;
                    ackpack.seq_num = seq-1;//返回最后接收到的包的序列号
                    ackpack.checksum = 0;
                    ackpack.checksum = checksum((u_short*)&ackpack, sizeof(ackpack));
                    memcpy(buffer, &ackpack, sizeof(ackpack));
                    // 返回ACK
                    sendto(serversock, buffer, sizeof(ackpack), 0, (sockaddr*)&clientaddr, clen);
                    cout << "\033[1;31mResend：\033[0m 重新返回ACK, 序列号  " << seq-1 << endl;
                //}
            }
            else if (messpack.len > 0) {
                if (ifname) {
                    memcpy(name + total_len, messpack.data, messpack.len);
                }
                else {
                    memcpy(recvdata + total_len, messpack.data, messpack.len);
                }
                total_len += messpack.len;

                // 返回ACK
                messpack.sign = ACK;
                messpack.len = 0;
                messpack.seq_num = seq;
                messpack.checksum = 0;
                messpack.checksum = checksum((u_short*)&messpack, sizeof(messpack));
                memcpy(buffer, &messpack, sizeof(messpack));
                // 返回ACK
                sendto(serversock, buffer, sizeof(messpack), 0, (sockaddr*)&clientaddr, clen);
                seq++;
                cout << "\033[1;31mSend：\033[0m 回复客户端 SEQ:" << (int)messpack.seq_num << endl;
            }
        }
        if (messpack.sign == OVER && ifcorrect(messpack, sizeof(messpack)))
        {
            cout << "\033[1;33mInfo：\033[0m 文件传输结束" << endl;
            break;
        }
    }
    // 发送OVER信息
    messpack.sign = OVER;
    messpack.checksum = 0;
    messpack.checksum = checksum((u_short*)&messpack, sizeof(messpack));
    memcpy(buffer, &messpack, sizeof(messpack));
    if (sendto(serversock, buffer, sizeof(messpack), 0, (sockaddr*)&clientaddr, clen) == -1)
    {
        return -1;
    }
    return total_len;
}


bool fourbye() {
    char* byebuff = new char[sizeof(Packet)];
    cout << endl;
    cout << "\033[1;36m-----开始挥手----- \033[0m" << endl;
rereceive:
    cout << endl;
    cout << "\033[1;33mFirst: \033[0m" << endl;
    Packet first;
    //接受客户端断开请求
    int result;
    while (true) {
        memset(byebuff, 0, sizeof(byebuff));
        result = recvfrom(serversock, byebuff, sizeof(first), 0, (struct sockaddr*)&clientaddr, &clen);
        if (result > 0) {
            //如果收到消息
            //检查fin,客户端seq
            memcpy(&first, byebuff, sizeof(first));
            bool firstcheck = fin(first, clientnum);
            if (!firstcheck) {
                goto rereceive;
            }
            clientnum = first.seq_num;
            cout << "\033[1;32mReceive\033[0m 接收到客户端断联请求..." << endl;
            break;
        }
    }

    //发送ack
    cout << endl;
    cout << "\033[1;33mSecond: \033[0m" << endl;
    Packet two;
    setFlag(two.sign, ACK);
    two.seq_num = servernum;
    two.ack_num = clientnum + 1;
    two.len = 0;
    two.checksum = 0;
    two.checksum = checksum((u_short*)&two, sizeof(two));
    memset(byebuff, 0, sizeof(byebuff));
    memcpy(byebuff, &two, sizeof(two));
    result = sendto(serversock, byebuff, sizeof(two), 0, (struct sockaddr*)&clientaddr, sizeof(clientaddr));
    if (result == -1) {
        //ACK未发送成功
        goto rereceive;
    }
    else {
        cout << "\033[1;32mSend: \033[0m 发送ACK确认......" << endl;
    }

    cout << endl;
    cout << "\033[1;33mThird: \033[0m" << endl;
    Packet fintwo;
    setFlag(fintwo.sign, ACK);
    setFlag(fintwo.sign, FIN);
    fintwo.seq_num = servernum;
    fintwo.ack_num = clientnum + 1;
    fintwo.len = 0;
    fintwo.checksum = 0;
    fintwo.checksum = checksum((u_short*)&fintwo, sizeof(fintwo));
    memset(byebuff, 0, sizeof(byebuff));
    memcpy(byebuff, &fintwo, sizeof(fintwo));
    result = sendto(serversock, byebuff, sizeof(fintwo), 0, (struct sockaddr*)&clientaddr, clen);
    if (result == -1) {
        //ACK未发送成功
        goto rereceive;
    }
    else {
        cout << "\033[1;32mSend: \033[0m 发送ACK+FIN......" << endl;
    }

    //等待客户端发送ACK
    //开始计时
    clock_t start = clock();
    cout << endl;
    cout << "\033[1;33mFour: \033[0m" << endl;
    Packet three;
    while (true) {
        clock_t now = clock();
        if (now - start > TIMEOUT) {
            goto rereceive;
        }
        memset(byebuff, 0, sizeof(byebuff));
        result = recvfrom(serversock, byebuff, sizeof(three), 0, (struct sockaddr*)&clientaddr, &clen);
        if (result > 0) {
            //如果收到消息
            //检查ack+seq+ack
            memcpy(&three, byebuff, sizeof(three));
            bool threecheck = ackseqack(three, servernum, clientnum);
            if (!threecheck) {
                goto rereceive;
            }
            cout << "\033[1;32mReceive：\033[0m 接收到客户端ACK......" << endl;
            cout << "\033[1;35mSuccess：\033[0m 成功断开连接...... " << endl;
            break;
        }
    }
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
    serversock = socket(AF_INET, SOCK_DGRAM, 0);
    if (serversock == INVALID_SOCKET) {
        cout << "socket创建失败" << endl;
        WSACleanup();
        closesocket(serversock);
        return 0;
    }

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
    result = bind(serversock, (SOCKADDR*)&serveraddr, slen);
    if (result == SOCKET_ERROR) {
        cout << "socket绑定IP与端口号失败" << endl;
        closesocket(serversock);
        WSACleanup();
        return 0;
    }
    cout << "\033[1;36mInfo: \033[0m 正在监听客户端......" << endl;

    //三次握手
    threeshake();

    //接收文件
    bool ifconnect = true;
    char choice;
    while (true)
    {
        cout << "\033[1;34mChoice: \033[0m 是否接收文件(Y/N)" << endl;
        cin >> choice;
        /*if (ifconnect && (choice == ('Y' | 'y'))) {
            continue;
        }*/
        if (choice == ('N' | 'n')) {
            ifconnect = false;
            break;
        }
        int namelen = RecvMessage(true);
        int datalen = RecvMessage(false);
        string a;
        for (int i = 0; i < namelen; i++)
        {
            a = a + name[i];
        }
        cout << endl
            << "[\033[1;36mOut\033[0m] 接收的文件名:" << a << endl;
        ofstream fout(a.c_str(), ofstream::binary);
        cout << "[\033[1;36mOut\033[0m] 接收的文件长度:" << datalen << endl;
        for (int i = 0; i < datalen; i++)
        {
            fout << recvdata[i];
        }
        //fout.write(recvdata, datalen);
        fout.close();
        cout << "[\033[1;36mOut\033[0m] 文件已成功下载到本地 " << endl;
    }
    //四次挥手
    fourbye();
    // 清理WinSock
    if (serversock != INVALID_SOCKET) {
        closesocket(serversock); // 确保socket被关闭
    }
    WSACleanup();
    return 0;
}