#include "packet.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <windows.h>
#include <string>
#include <queue>
#include <cmath>
#include <map>
using namespace std;
SOCKET clientsock;
SOCKADDR_IN serveraddr;
SOCKADDR_IN clientaddr;
uint32_t clientnum = rand();//�ͻ������к�
uint32_t servernum;//���������к�
u_short serverport = 8888;//�������˿ں�
u_short clientport = 9999;//�ͻ��˶˿ں�
int slen;
int clen;
#define TIMEOUT 500  // ��ʱʱ��(ms)
#define MAX_RETRIES 5 // ������Դ���
//��������
bool threeshake() {
    char* buff = new char[sizeof(Packet)];
    int shakeresult;
    int times = MAX_RETRIES;
    cout << endl;
    cout << "\033[1;36m-----��ʼ����----- \033[0m" << endl;
    cout << endl;
    //��ʼ�����кź�ȷ�Ϻ�
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
    //��һ�Σ�����syn
    memcpy(buff, &onepack, sizeof(onepack));
    //buff = serialize(onepack);
    shakeresult = sendto(clientsock, buff, sizeof(onepack), 0, (struct sockaddr*)&serveraddr, slen);
    if (shakeresult == -1) {
        cout << "\033[1;31mInfo��\033[0m ����ʧ��,�ط�......" << endl;
        goto resend;
    }
    else {
        cout << "\033[1;32mSend: \033[0m ������������ɹ�,�ȴ���Ӧ��..." << endl;
    }
    //����Ϊ������״̬
    u_long mode = 1;
    ioctlsocket(clientsock, FIONBIO, &mode);

    //�ڶ��Σ����շ�����ACK
    Packet twopack;
    cout << endl;
    cout << "\033[1;33mSecond: \033[0m" << endl;
    //��ʼ��ʱ
    clock_t start = clock();
rerece:
    while (true) {
        clock_t now = clock();
        if (now - start > TIMEOUT) {
            //��ʱ�ش�
            goto resend;
        }
        //�ڶ��Σ�����ACK��SYN,���ack������seq
        memset(buff, 0, sizeof(buff));
        shakeresult = recvfrom(clientsock, buff, sizeof(twopack), 0, (sockaddr*)&serveraddr, &slen);
        if (shakeresult == -1) {
            goto rerece;
        }
        else {//���յ���Ϣ,�ж�SYN��ACK
            memcpy(&twopack, buff, sizeof(twopack));
            if (ackacksyn(twopack, clientnum)) {
                servernum = twopack.seq_num;
                cout << "\033[1;32mReceive��\033[0m ���յ����Է�������ACK..." << endl;
                break;
            }
        }
    }

    //������,����ACKȷ��
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
        //ACKδ���ͳɹ�
        return false;
    }
    cout << "\033[1;32mSend: \033[0m �ͻ��˷���ACKȷ��......" << endl;
    mode = 0;
    ioctlsocket(clientsock, FIONBIO, &mode);
    cout << "\033[1;35mSuccess��\033[0m ��������ͻ��˳ɹ�����" << endl;
    return true;
}
// �����ļ�
bool sendfile(char* message, int len, bool ifname)
{
    int sresult = 0;
    int cwnd = 1; //ӵ�����ڴ�С
    int ssthresh = 64;//��ֵ
    int lastwindow;//����ӵ���׶�ʹ��

    int lastbyteacked = 0; // ���ڵ���߽磨��С��δȷ�ϰ����кţ�
    int lastbytesent = 0; // ������δ���͵����к�

    int packnum = (len % SIZE == 0) ? len / SIZE : len / SIZE + 1; // �ܰ���
    int last_recvack = -1; // ��һ���յ���ack��
    int count = 0;//��������������ack

    int attribute = 1;//״̬ 1����������2��ӵ�����⣻3�����ٻָ�
    bool first = false;//�Ƿ���ÿ��rtt�Ŀ�ʼ
    int yscount = 0;//ӵ���׶εļ���

    char* messbuf = new char[sizeof(Packet)];
    // ������¼ÿ�����ķ���ʱ��
    clock_t* stime = new clock_t[packnum];
    //�ܵİ���0 ~ packnum-1
    cout << "���ڴ�С��" << cwnd << "  ��ֵ��" << ssthresh << endl;
    while (lastbyteacked < packnum) {
        if (lastbytesent < lastbyteacked + cwnd && lastbytesent < packnum) {
            // �����������ݰ����Է��;ͷ���
            Packet datapack;
            //���ð��Ĵ�С������
            if (lastbytesent != packnum - 1) {
                datapack.len = SIZE;
                memcpy(datapack.data, message + lastbytesent * SIZE, SIZE);
            }
            else {
                datapack.len = len - SIZE * (packnum - 1);
                memcpy(datapack.data, message + (packnum - 1) * SIZE, datapack.len);
            }

            datapack.sign = (ifname) ? NAME : 0;//�Ƿ����ļ�����

            datapack.seq_num = lastbytesent;
            datapack.checksum = 0;
            datapack.checksum = checksum((u_short*)&datapack, sizeof(datapack));
            memset(messbuf, 0, sizeof(messbuf));
            memcpy(messbuf, &datapack, sizeof(datapack));
            // ��¼����ʱ��
            stime[lastbytesent] = clock();

            sendto(clientsock, messbuf, sizeof(datapack), 0, (sockaddr*)&serveraddr, slen);
            cout << "\033[1;33mSend: \033[0m ���Ͱ������к�: " << lastbytesent << endl;
            lastbytesent++; // ������һ����
        }

        //while (true) {
            // ���÷�����ģʽ
            u_long mode = 1;
            ioctlsocket(clientsock, FIONBIO, &mode);
            Packet ackp;
            memset(messbuf, 0, sizeof(messbuf));
            sresult = recvfrom(clientsock, messbuf, sizeof(ackp), 0, (sockaddr*)&serveraddr, &slen);
            if (clock() - stime[lastbyteacked] > TIMEOUT) {
                //��ʱ�ش�
                lastbytesent = lastbyteacked;
                ssthresh = cwnd / 2;
                cwnd = 1;
                count = 0;
                yscount = 0;
                attribute = 1;//����������״̬
                cout << "\033[1;31mInfo��\033[0m ��ʱ...... " << endl;
                cout << "���ڴ�С��" << cwnd <<"  ��ֵ��"<<ssthresh << endl;
                cout << "���ڷ�Χ��" << lastbyteacked << "  ~  " << lastbyteacked + cwnd - 1 << endl;
                continue;
                //break;
            }
            if (sresult == -1) {
                continue;
                //break;
            }
            memcpy(&ackp, messbuf, sizeof(ackp));
            if (ifcorrect(ackp, sizeof(ackp)) && ackp.sign == ACK) {
                cout << "\033[1;32mReceive��\033[0m ���յ����Է�������ACK " << ackp.seq_num << endl;
                int acknum = ackp.seq_num;
                if (acknum >= lastbyteacked-1) {//���ڴ�����߽�
                    if (last_recvack != acknum) {//�µ�ack
                        count = 0;
                        if (cwnd >= ssthresh) {
                            if (attribute == 3|| attribute == 1) {
                                //���ν���ӵ�����ƽ׶�
                                yscount = 0;
                                first = true;
                            }
                            attribute = 2;//����ӵ������׶�
                        }
                        if (attribute==1) {
                            yscount = 0;
                            cwnd += (acknum-lastbyteacked+1);
                        }
                        else if(attribute==2){//ӵ������
                            if (first) {
                                cout << "���ν��벻����" << endl;
                            }
                            else {
                                cout << "���ǳ��ν��룬���м���" << endl;
                                yscount += acknum - last_recvack;   
                            }
                            first = false;
                            lastwindow = cwnd;
                            cout << cwnd << "  " << yscount << endl;
                            if (cwnd == yscount) {
                                cwnd += 1;
                                yscount = 0;
                            }
                        }else if(attribute==3){//���ٻָ�
                            cwnd = ssthresh;
                            attribute = 2;//����ӵ������׶�
                            yscount = 0;
                        }
                        lastbyteacked = acknum + 1;  // ������߽���ǰ����
                        //cout << "������߽磺" << lastbyteacked << endl;
                        cout << "��ACK" <<  "   ״̬: " << attribute << endl;
                    }
                    else if (last_recvack == acknum) {
                        cout << "�ظ�  ״̬: " << attribute << endl;
                        if (attribute==3) {
                            cwnd += 1;
                            yscount = 0;
                        }
                        if (attribute == 1||attribute==2) {
                            count++;
                        }
                        cout << count << endl;
                    }
                    last_recvack = acknum;
                    if (count == 2&& (attribute == 1 || attribute==2)) {//�����ظ���ack��ⶪʧ
                        cout << "�����ظ�ack" << endl;
                        ssthresh = cwnd / 2;
                        cwnd = ssthresh + 3;//�������������׶�
                        // �����ش�������������δȷ�ϵİ�����Ҫ�ش�
                        lastbytesent = lastbyteacked;
                        attribute = 3;
                        cout << "״̬: " << attribute << endl;
                    }
                    cout << "���ڴ�С��" << cwnd << "  ��ֵ��" << ssthresh << endl;
                    cout << "���ڷ�Χ��" << lastbyteacked << "  ~  " << lastbyteacked + cwnd - 1 << endl;
                }
                //break;
            }
        //}
        mode = 0;
        ioctlsocket(clientsock, FIONBIO, &mode);  // �Ļ�����ģʽ
    }

    // ���ͽ�����־
sendover:
    Packet overpack;
    setFlag(overpack.sign, OVER);
    overpack.checksum = 0;
    overpack.checksum = checksum((u_short*)&overpack, sizeof(overpack));
    memcpy(messbuf, &overpack, sizeof(overpack));
    sresult = sendto(clientsock, messbuf, sizeof(overpack), 0, (sockaddr*)&serveraddr, slen);
    if (sresult == -1) {
        cout << "\033[1;33mInfo:\033[0m ���·�������" << endl;
        goto sendover;
    }
    cout << "\033[1;31mSend:\033[0m ����OVER�ź�" << endl;
    clock_t start;
    start = clock();
    while (true)
    {
        u_long mode = 1;
        ioctlsocket(clientsock, FIONBIO, &mode);
        sresult = recvfrom(clientsock, messbuf, SIZE, 0, (sockaddr*)&serveraddr, &slen);
        if (clock() - start > TIMEOUT)
        {
            cout << "\033[1;33mInfo:\033[0m ��ʱ" << endl;
            goto sendover;
        }
        memcpy(&overpack, messbuf, sizeof(overpack));
        // �յ������ݰ�ΪOVER���Ѿ��ɹ������ļ�
        if (overpack.sign == OVER && ifcorrect(overpack, sizeof(overpack)))
        {
            cout << "\033[1;33mInfo:\033[0m �Է��ѳɹ������ļ�" << endl;
            break;
        }
    }
    u_long mode = 0;
    ioctlsocket(clientsock, FIONBIO, &mode); // �Ļ�����ģʽ
    return true;
}


// �Ĵλ���
bool fourbye() {
    char* byebuff = new char[sizeof(Packet)];
    int times = MAX_RETRIES;
    cout << endl;
    cout << "\033[1;36m-----��ʼ����----- \033[0m" << endl;
resend:
    cout << endl;
    cout << "\033[1;33mFirst: \033[0m" << endl;
    memset(byebuff, 0, sizeof(byebuff));
    Packet fourpack;
    setFlag(fourpack.sign, FIN);
    fourpack.seq_num = clientnum;
    fourpack.checksum = 0;
    fourpack.checksum = checksum((u_short*)&fourpack, sizeof(fourpack));
    // ����FIN��+seq
    memcpy(byebuff, &fourpack, sizeof(fourpack));
    int byeresult = sendto(clientsock, byebuff, sizeof(fourpack), 0, (struct sockaddr*)&serveraddr, slen);
    if (byeresult == -1) {
        if (times--) {
            cout << "   ����ʧ�ܣ����³���..." << endl;
            goto resend;
        }
        else {
            cout << "   �ط�����ʹ���꣬��δ����..." << endl;
            return false;
        }
    }
    cout << "\033[1;32mSend: \033[0m ���Ͷ�������ɹ�,�ȴ���Ӧ��..." << endl;
    //����Ϊ������״̬
    u_long mode = 1;
    ioctlsocket(clientsock, FIONBIO, &mode);
    //���շ�����ACK
    Packet tpack;
    times = MAX_RETRIES;
    cout << endl;
    cout << "\033[1;33mSecond: \033[0m" << endl;
    //��ʼ��ʱ
    clock_t start = clock();
    while (true) {
        clock_t now = clock();
        if (now - start > TIMEOUT) {
            if (times--) {
                cout << "   ��ʱδ���ܵ�ACK�����³��Է�������..." << endl;
                goto resend;
            }
            else {
                cout << "   ��ʱ���ط��������꣬��δ����..." << endl;
                return false;
            }
        }
        //�ڶ��Σ�����ACK
        memset(byebuff, 0, sizeof(byebuff));
        byeresult = recvfrom(clientsock, byebuff, sizeof(tpack), 0, (struct sockaddr*)&serveraddr, &slen);
        if (byeresult > 0) {//���յ���Ϣ,�ж�SYN��ACK
            // ���
            memcpy(&tpack, byebuff, sizeof(tpack));
            bool packetck = ackack(tpack, clientnum);
            if (!packetck) {
                if (times--) {
                    cout << "   δ���ܵ�ACK�������³��Է�������..." << endl;
                    goto resend;
                }
                else {
                    cout << "   ���յ���ACK���ط��������꣬��δ����..." << endl;
                    return false;
                }
            }
            servernum = tpack.seq_num;
            cout << "\033[1;32mReceive��\033[0m ���յ����Է�������ACK..." << endl;
            break;
        }
    }
    //���շ�����FIN
    Packet spack;
    times = MAX_RETRIES;
    cout << endl;
    cout << "\033[1;33mThird: \033[0m" << endl;
    //��ʼ��ʱ
    clock_t start2 = clock();
    while (true) {
        clock_t now2 = clock();
        if (now2 - start2 > TIMEOUT) {
            if (times--) {
                cout << "   ��ʱδ���ܵ�ACK�����³��Է�������..." << endl;
                goto resend;
            }
            else {
                cout << "   ��ʱ���ط��������꣬��δ����..." << endl;
                return false;
            }
        }
        memset(byebuff, 0, sizeof(byebuff));
        byeresult = recvfrom(clientsock, byebuff, sizeof(spack), 0, (struct sockaddr*)&serveraddr, &slen);
        if (byeresult > 0) {//���յ���Ϣ,�ж�SYN��ACK
            // ���,fin,seq
            memcpy(&spack, byebuff, sizeof(spack));
            bool packetck = finackack(spack, clientnum);
            if (!packetck) {
                if (times--) {
                    cout << "   δ���ܵ�ACK�������³��Է�������..." << endl;
                    goto resend;
                }
                else {
                    cout << "   ���յ���ACK���ط��������꣬��δ����..." << endl;
                    return false;
                }
            }
            cout << "\033[1;32mReceive��\033[0m ���յ����Է�������ACK..." << endl;
            break;
        }
    }

    //ʱ����ѯ�ȴ�
    // 
    /*cout << "Waiting for data..." << endl;
    Sleep(2 * MAX_LIVETIME);*/
    //����ack
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
        //ACKδ���ͳɹ�
        if (times--) {
            cout << "   ���·���ACK..." << endl;
            goto resend;
        }
        else {
            cout << "   ACK�ط���������..." << endl;
            return false;
        }
    }
    cout << "\033[1;32mSend: \033[0m ����ACK......" << endl;
    cout << "\033[1;35mSuccess��\033[0m �ɹ��Ͽ�����...... " << endl;
    mode = 0;
    ioctlsocket(clientsock, FIONBIO, &mode);
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
    clientsock = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientsock == INVALID_SOCKET) {
        cout << "socket����ʧ��" << endl;
        WSACleanup();
        closesocket(clientsock);
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
    result = bind(clientsock, (SOCKADDR*)&clientaddr, clen);
    if (result == SOCKET_ERROR) {
        cout << "socket��IP��˿ں�ʧ��" << endl;
        closesocket(clientsock);
        WSACleanup();
        return 0;
    }

    //��������
    threeshake();
    bool ifconnect = true;
    char choice;
    while (true)
    {
        cout << endl;
        cout << "\033[1;34mChoice: \033[0m ��ѡ���Ƿ������ļ�(Y/N)" << endl;
        cin >> choice;
        if (ifconnect && (choice == ('Y' | 'y'))) {
            //�����ļ�
        }
        if (choice == ('N' | 'n')) {
            ifconnect = false;
            break;
        }
        // ��ȡ�ļ����ݵ�buffer
        string file; // ϣ��������ļ�����
        cout << "\033[1;34mInput: \033[0m �����봫����ļ�" << endl;
        cin >> file;
        ifstream f(file.c_str(), ifstream::binary); // �Զ����Ʒ�ʽ���ļ�
        if (!f.is_open()) {
            cerr << "\033[1;31mError:\033[0m �޷����ļ�" << endl;
            return false;
        }
        f.seekg(0, ios::end);
        int len = f.tellg();
        f.seekg(0, ios::beg);
        char* buffer = new char[len];
        f.read(buffer, len);
        f.close();
        // �����ļ���
        sendfile((char*)(file.c_str()), file.length(), true);
        clock_t start1 = clock();
        // �����ļ�����
        sendfile(buffer, len, false);
        clock_t end1 = clock();
        cout << "\033[1;36mOut:\033[0m ������ʱ��Ϊ:" << (end1 - start1) / CLOCKS_PER_SEC<< "s" << endl;
        cout << "\033[1;36mOut:\033[0m ������Ϊ:" << fixed << setprecision(2) << (((double)len) / ((end1 - start1) )) << "byte/s" << endl;
    }
    //�Ĵλ���
    fourbye();
    // ����WinSock
    if (clientsock != INVALID_SOCKET) {
        closesocket(clientsock); // ȷ��socket���ر�
    }
    WSACleanup();
    WSACleanup();
    return 0;
}