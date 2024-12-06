#pragma once

#include <ws2tcpip.h>  
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
using namespace std;
#define SIZE 32768  // ���ݰ���С
// �����־λ
#define ACK 1  // ACK ��־λ (00000001)
#define SYN 2  // SYN ��־λ (00000010)
#define FIN 4  // FIN ��־λ (00000100)
#define ACK_SYN 3
#define FIN_ACK 5
#define OVER 8  // FIN ��־λ (00001000)
#define NAME 6
//#define DATA 0x08  //DATA ��־λ (00001000)
// ���ݰ���α�ײ�+�˿ں�+����+У���+���ݣ�
struct Packet {
    uint32_t seq_num = 0;  // ���кţ��ڼ�������
    uint32_t ack_num = 0;  // ȷ�Ϻ�
    uint8_t sign = 0; //Э�飬����λACK,SYN,FIN
    uint16_t len=0;  // ���ݳ���
    uint16_t checksum=0;  // У���
    char data[SIZE] = { 0 };  // ��������
};
// ���ñ�־λ
void setFlag(uint8_t& sign, uint8_t flag) {
    sign |= flag;  // ��1
}
// �����־λ
void clearFlag(uint8_t& sign, uint8_t flag) {
    sign &= ~flag;  // ����
}
// ����־λ
bool checkFlag(uint8_t sign, uint8_t flag) {
    return (sign & flag) != 0;
}
//UDPУ��ͼ���
u_short checksum(u_short* buffer, int count) {
    int num = (count + 1) / 2;//����16bit�ֶ���
    u_short* message = (u_short*)malloc(count + 1);
    memset(message, 0, count + 1);
    memcpy(message, buffer, count);//��message����
    u_long sum = 0;
    while (num--) {
        sum += *message++;
        if (sum & 0xFFFF0000) {// �����16λ�н�λ
            sum &= 0xFFFF;// ������16λ
            sum++;// ����16λ�Ľ�λ�ӵ���16λ
        }
    }
    return ~(sum & 0xFFFF);// ȡ��
}
//У��͵ļ��
bool ifcorrect(Packet pack, int count) {
    u_short t = pack.checksum;
    pack.checksum = 0;
    u_short current = checksum((u_short*)&pack, count);
    if (current == t) {
        return true;
    }
    else {
        return false;
    }
}
//���ļ��
//bool datacheck(Packet packet) {
//    // ����־λ
//    if (!checkFlag(packet.sign, DATA)) {
//        cout << "DATA����" << endl;
//        return false;
//    }
//    // У�����֤
//    if (ifcorrect(packet, sizeof(packet))) {
//        cout << "У��ͳ���" << endl;
//        return false;
//    }
//    return true;
//}
bool syn(Packet packet) {
    // ����־λ
    if (!checkFlag(packet.sign, SYN)) {
        cout << "SYN����" << endl;
        return false;
    }
    // У�����֤
    if (!ifcorrect(packet, sizeof(packet))) {
        cout << "У��ͳ���" << endl;
        return false;
    }
    return true;
}
bool ackacksyn(Packet packet, int num) {
    // ���ACK��־λ
    if (!checkFlag(packet.sign, ACK_SYN)) {
        cout << "ACK����" << endl;
        return false;
    }
    // ���ȷ�Ϻ�
    if (packet.ack_num != num + 1) {
        cout << "ȷ�Ϻų���" << endl;
        return false;
    }
    // У�����֤
    if (!ifcorrect(packet, sizeof(packet))) {
        cout << "У��ͳ���" << endl;
        return false;
    }
    return true;
}
bool ackseqack(Packet packet, int server, int client) {
    // ���ACK��־λ
    if (!checkFlag(packet.sign, ACK)) {
        cout << "ACK����" << endl;
        return false;
    }
    // ���ȷ�Ϻ�
    if (packet.seq_num != client + 1) {
        cout << "ȷ�Ϻų���" << endl;
        return false;
    }
    if (packet.ack_num != server + 1) {
        cout << "ȷ�Ϻų���" << endl;
        return false;
    }
    // У�����֤
    if (!ifcorrect(packet, sizeof(packet))) {
        cout << "У��ͳ���" << endl;
        return false;
    }
    return true;
}
bool fin(Packet packet, int client) {
    // ���FIN��־λ
    if (!checkFlag(packet.sign, FIN)) {
        cout << "FIN����" << endl;
        return false;
    }
    // У�����֤
    if (!ifcorrect(packet, sizeof(packet))) {
        cout << "У��ͳ���" << endl;
        return false;
    }
    return true;
}
bool ackack(Packet packet, int num) {
    // ���ACK��־λ
    if (!checkFlag(packet.sign, ACK)) {
        cout << "ACK����" << endl;
        return false;
    }
    // ���ȷ�Ϻ�
    if (packet.ack_num != num + 1) {
        cout << "ȷ�Ϻų���" << endl;
        return false;
    }
    // У�����֤
    if (!ifcorrect(packet, sizeof(packet))) {
        cout << "У��ͳ���" << endl;
        return false;
    }
    return true;
}
bool finackack(Packet packet, int num) {
    // ���FIN��־λ
    if (!checkFlag(packet.sign, FIN)) {
        cout << "FIN����" << endl;
        return false;
    }
    // ���ȷ�Ϻ�
    if (packet.ack_num != num+1) {
        cout << "ȷ�Ϻų���" << endl;
        return false;
    }
    // У�����֤
    if (!ifcorrect(packet, sizeof(packet))) {
        cout << "У��ͳ���" << endl;
        return false;
    }
    return true;
}