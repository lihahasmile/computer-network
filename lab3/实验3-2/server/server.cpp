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
uint32_t clientnum;//�ͻ������к�
uint32_t servernum = rand();//���������к�
u_short serverport = 8888;//�������˿ں�
u_short clientport = 9999;//�ͻ��˶˿ں�
int slen;
int clen;
#define TIMEOUT 500  // ��ʱʱ��
#define MAX_RETRIES 5 // ������Դ���
char* buffer = new char[sizeof(Packet)];
char* name = new char[20];
char* recvdata = new char[100000000];
//��������
bool threeshake() {
    char* buff = new char[sizeof(Packet)];
    int shakeresult;
    int times = MAX_RETRIES;
    Packet one;
    cout << endl;
    cout << "\033[1;36m-----��ʼ����----- \033[0m" << endl;
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
            //�Խ��յ��İ����м��
            //��һ����seq=�ͻ��ˣ�
            if (syn(one)) {//���syn��У���
                clientnum = one.seq_num;
                cout << "\033[1;32mReceive: \033[0m ���յ��ͻ�����������......" << endl;
                break;
            }
        }
    }
    //���͵ڶ�����ack+syn
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
        cout << "\033[1;32mSend: \033[0m ����������ACKȷ��......" << endl;
    }
    //��ʱ���տͻ���ack
    cout << endl;
    cout << "\033[1;33mThird: \033[0m" << endl;
    Packet three;
    clock_t start = clock();
    while (true) {
        memset(buff, 0, sizeof(buff));
        shakeresult = recvfrom(serversock, buff, sizeof(three), 0, (sockaddr*)&clientaddr, &clen);
        clock_t now = clock();
        if (now - start > TIMEOUT) {
            cout << "\033[1;31mInfo��\033[0m ���ֳ�ʱ���ͻ��˽��ط������������½���...... " << endl;
            goto rerece;
        }
        if (shakeresult != -1) {
            break;
        }

    }
    //ack��seq=�ͻ���+1��ack=������+1��
    memcpy(&three, buff, sizeof(three));
    //����
    if (ackseqack(three, servernum, clientnum)) {
        cout << "\033[1;32mReceive��\033[0m ���յ��ͻ���ACKȷ��......" << endl;
        cout << "\033[1;35mSuccess��\033[0m ��������ͻ��˳ɹ�����" << endl;
    }
    return true;
}

int RecvMessage(bool ifname)
{
    int remsult;
    Packet messpack;
    int seq = 0;//���к�
    long int total_len = 0;//�ܽ��ճ���
    while (true)
    {
        remsult = recvfrom(serversock, buffer, sizeof(messpack), 0, (sockaddr*)&clientaddr, &clen);
        if (remsult == -1)// �������ʧ�ܣ���������
        {
            continue;
        }
        else {
            memcpy(&messpack, buffer, sizeof(messpack));

            // ģ�ⶪ�������ݶ������ʾ����Ƿ������յ����ݰ�
            if (rand() / (double)RAND_MAX < 0.1)
            {
                cout << "\033[1;31mPacket Dropped: \033[0m: ����, ���к�  " << messpack.seq_num << endl;
                continue; // ������������ǰѭ��
            }
            int t = int(messpack.seq_num);
            cout << "\033[1;32mReceive��\033[0m �յ��� " << messpack.len << " ���к�  " << int(messpack.seq_num) << endl;

            if (!ifcorrect(messpack, sizeof(messpack))) {
                cout << "\033[1;31mError��\033[0m �������ݰ�" << endl;
                continue;
            }

            // ģ������ӳ�
            //this_thread::sleep_for(std::chrono::milliseconds(10));

            if (seq != t) {
                //if (seq > t) {//˵��д���ļ��˵��ǿͻ���δ�յ�ack
                    Packet ackpack;
                    ackpack.sign = ACK;
                    ackpack.len = 0;
                    ackpack.seq_num = seq-1;//���������յ��İ������к�
                    ackpack.checksum = 0;
                    ackpack.checksum = checksum((u_short*)&ackpack, sizeof(ackpack));
                    memcpy(buffer, &ackpack, sizeof(ackpack));
                    // ����ACK
                    sendto(serversock, buffer, sizeof(ackpack), 0, (sockaddr*)&clientaddr, clen);
                    cout << "\033[1;31mResend��\033[0m ���·���ACK, ���к�  " << seq-1 << endl;
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

                // ����ACK
                messpack.sign = ACK;
                messpack.len = 0;
                messpack.seq_num = seq;
                messpack.checksum = 0;
                messpack.checksum = checksum((u_short*)&messpack, sizeof(messpack));
                memcpy(buffer, &messpack, sizeof(messpack));
                // ����ACK
                sendto(serversock, buffer, sizeof(messpack), 0, (sockaddr*)&clientaddr, clen);
                seq++;
                cout << "\033[1;31mSend��\033[0m �ظ��ͻ��� SEQ:" << (int)messpack.seq_num << endl;
            }
        }
        if (messpack.sign == OVER && ifcorrect(messpack, sizeof(messpack)))
        {
            cout << "\033[1;33mInfo��\033[0m �ļ��������" << endl;
            break;
        }
    }
    // ����OVER��Ϣ
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
    cout << "\033[1;36m-----��ʼ����----- \033[0m" << endl;
rereceive:
    cout << endl;
    cout << "\033[1;33mFirst: \033[0m" << endl;
    Packet first;
    //���ܿͻ��˶Ͽ�����
    int result;
    while (true) {
        memset(byebuff, 0, sizeof(byebuff));
        result = recvfrom(serversock, byebuff, sizeof(first), 0, (struct sockaddr*)&clientaddr, &clen);
        if (result > 0) {
            //����յ���Ϣ
            //���fin,�ͻ���seq
            memcpy(&first, byebuff, sizeof(first));
            bool firstcheck = fin(first, clientnum);
            if (!firstcheck) {
                goto rereceive;
            }
            clientnum = first.seq_num;
            cout << "\033[1;32mReceive\033[0m ���յ��ͻ��˶�������..." << endl;
            break;
        }
    }

    //����ack
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
        //ACKδ���ͳɹ�
        goto rereceive;
    }
    else {
        cout << "\033[1;32mSend: \033[0m ����ACKȷ��......" << endl;
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
        //ACKδ���ͳɹ�
        goto rereceive;
    }
    else {
        cout << "\033[1;32mSend: \033[0m ����ACK+FIN......" << endl;
    }

    //�ȴ��ͻ��˷���ACK
    //��ʼ��ʱ
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
            //����յ���Ϣ
            //���ack+seq+ack
            memcpy(&three, byebuff, sizeof(three));
            bool threecheck = ackseqack(three, servernum, clientnum);
            if (!threecheck) {
                goto rereceive;
            }
            cout << "\033[1;32mReceive��\033[0m ���յ��ͻ���ACK......" << endl;
            cout << "\033[1;35mSuccess��\033[0m �ɹ��Ͽ�����...... " << endl;
            break;
        }
    }
    return true;
}


int main() {
    // ��ʼ��WinSock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cout << "WSAstartupʧ��" << endl;
        return 0;
    }
    // ����socket
    serversock = socket(AF_INET, SOCK_DGRAM, 0);
    if (serversock == INVALID_SOCKET) {
        cout << "socket����ʧ��" << endl;
        WSACleanup();
        closesocket(serversock);
        return 0;
    }

     // ��������IP��ַ�Ͷ˿ں�
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(serveraddr.sin_addr));
    //serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); // ������������ӿ�
    serveraddr.sin_port = htons(serverport);//�˿ں�
    // �ͻ��˵�ip�Ͷ˿ں�
    memset(&clientaddr, 0, sizeof(clientaddr));
    clientaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(clientaddr.sin_addr));
    serveraddr.sin_port = htons(clientport);//�˿ں�

    slen = sizeof(serveraddr);
    clen = sizeof(clientaddr);
    //bind��
    result = bind(serversock, (SOCKADDR*)&serveraddr, slen);
    if (result == SOCKET_ERROR) {
        cout << "socket��IP��˿ں�ʧ��" << endl;
        closesocket(serversock);
        WSACleanup();
        return 0;
    }
    cout << "\033[1;36mInfo: \033[0m ���ڼ����ͻ���......" << endl;

    //��������
    threeshake();

    //�����ļ�
    bool ifconnect = true;
    char choice;
    while (true)
    {
        cout << "\033[1;34mChoice: \033[0m �Ƿ�����ļ�(Y/N)" << endl;
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
            << "[\033[1;36mOut\033[0m] ���յ��ļ���:" << a << endl;
        ofstream fout(a.c_str(), ofstream::binary);
        cout << "[\033[1;36mOut\033[0m] ���յ��ļ�����:" << datalen << endl;
        for (int i = 0; i < datalen; i++)
        {
            fout << recvdata[i];
        }
        //fout.write(recvdata, datalen);
        fout.close();
        cout << "[\033[1;36mOut\033[0m] �ļ��ѳɹ����ص����� " << endl;
    }
    //�Ĵλ���
    fourbye();
    // ����WinSock
    if (serversock != INVALID_SOCKET) {
        closesocket(serversock); // ȷ��socket���ر�
    }
    WSACleanup();
    return 0;
}