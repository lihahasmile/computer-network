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
#define SIZE 32768  // 数据包大小
// 定义标志位
#define ACK 1  // ACK 标志位 (00000001)
#define SYN 2  // SYN 标志位 (00000010)
#define FIN 4  // FIN 标志位 (00000100)
#define ACK_SYN 3
#define FIN_ACK 5
#define OVER 8  // FIN 标志位 (00001000)
#define NAME 6
//#define DATA 0x08  //DATA 标志位 (00001000)
// 数据包（伪首部+端口号+长度+校验和+数据）
struct Packet {
    uint32_t seq_num = 0;  // 序列号（第几个包）
    uint32_t ack_num = 0;  // 确认号
    uint8_t sign = 0; //协议，低三位ACK,SYN,FIN
    uint16_t len=0;  // 数据长度
    uint16_t checksum=0;  // 校验和
    char data[SIZE] = { 0 };  // 数据内容
};
// 设置标志位
void setFlag(uint8_t& sign, uint8_t flag) {
    sign |= flag;  // 置1
}
// 清除标志位
void clearFlag(uint8_t& sign, uint8_t flag) {
    sign &= ~flag;  // 清零
}
// 检查标志位
bool checkFlag(uint8_t sign, uint8_t flag) {
    return (sign & flag) != 0;
}
//UDP校验和计算
u_short checksum(u_short* buffer, int count) {
    int num = (count + 1) / 2;//计算16bit字段数
    u_short* message = (u_short*)malloc(count + 1);
    memset(message, 0, count + 1);
    memcpy(message, buffer, count);//讲message读入
    u_long sum = 0;
    while (num--) {
        sum += *message++;
        if (sum & 0xFFFF0000) {// 如果高16位有进位
            sum &= 0xFFFF;// 保留低16位
            sum++;// 将高16位的进位加到低16位
        }
    }
    return ~(sum & 0xFFFF);// 取反
}
//校验和的检查
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
//包的检查
//bool datacheck(Packet packet) {
//    // 检查标志位
//    if (!checkFlag(packet.sign, DATA)) {
//        cout << "DATA出错" << endl;
//        return false;
//    }
//    // 校验和验证
//    if (ifcorrect(packet, sizeof(packet))) {
//        cout << "校验和出错" << endl;
//        return false;
//    }
//    return true;
//}
bool syn(Packet packet) {
    // 检查标志位
    if (!checkFlag(packet.sign, SYN)) {
        cout << "SYN出错" << endl;
        return false;
    }
    // 校验和验证
    if (!ifcorrect(packet, sizeof(packet))) {
        cout << "校验和出错" << endl;
        return false;
    }
    return true;
}
bool ackacksyn(Packet packet, int num) {
    // 检查ACK标志位
    if (!checkFlag(packet.sign, ACK_SYN)) {
        cout << "ACK出错" << endl;
        return false;
    }
    // 检查确认号
    if (packet.ack_num != num + 1) {
        cout << "确认号出错" << endl;
        return false;
    }
    // 校验和验证
    if (!ifcorrect(packet, sizeof(packet))) {
        cout << "校验和出错" << endl;
        return false;
    }
    return true;
}
bool ackseqack(Packet packet, int server, int client) {
    // 检查ACK标志位
    if (!checkFlag(packet.sign, ACK)) {
        cout << "ACK出错" << endl;
        return false;
    }
    // 检查确认号
    if (packet.seq_num != client + 1) {
        cout << "确认号出错" << endl;
        return false;
    }
    if (packet.ack_num != server + 1) {
        cout << "确认号出错" << endl;
        return false;
    }
    // 校验和验证
    if (!ifcorrect(packet, sizeof(packet))) {
        cout << "校验和出错" << endl;
        return false;
    }
    return true;
}
bool fin(Packet packet, int client) {
    // 检查FIN标志位
    if (!checkFlag(packet.sign, FIN)) {
        cout << "FIN出错" << endl;
        return false;
    }
    // 校验和验证
    if (!ifcorrect(packet, sizeof(packet))) {
        cout << "校验和出错" << endl;
        return false;
    }
    return true;
}
bool ackack(Packet packet, int num) {
    // 检查ACK标志位
    if (!checkFlag(packet.sign, ACK)) {
        cout << "ACK出错" << endl;
        return false;
    }
    // 检查确认号
    if (packet.ack_num != num + 1) {
        cout << "确认号出错" << endl;
        return false;
    }
    // 校验和验证
    if (!ifcorrect(packet, sizeof(packet))) {
        cout << "校验和出错" << endl;
        return false;
    }
    return true;
}
bool finackack(Packet packet, int num) {
    // 检查FIN标志位
    if (!checkFlag(packet.sign, FIN)) {
        cout << "FIN出错" << endl;
        return false;
    }
    // 检查确认号
    if (packet.ack_num != num+1) {
        cout << "确认号出错" << endl;
        return false;
    }
    // 校验和验证
    if (!ifcorrect(packet, sizeof(packet))) {
        cout << "校验和出错" << endl;
        return false;
    }
    return true;
}