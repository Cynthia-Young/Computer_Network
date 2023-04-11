#include <winsock2.h> 
#include<iostream>
#include<fstream>
#include<cstring>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include<ctime>
//#include <netinet/in.h>
#include <errno.h>
#include <WS2tcpip.h>
using namespace std;
#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable:4996)

#define SYN 1
#define SYN_ACK 2
#define ACK 4
#define FIN_ACK 8
#define PSH 16
#define NAK 32

#define JPG 1
#define TXT 2

const int BUFFER_SIZE = 8192;
const int WAIT_TIME = 100;//�ͻ��˵ȴ��¼���ʱ�䣬��λms
char buf[64];//����ʱ���
unsigned char lastAck = -1;

char* timei()
{
	time_t t;
	struct tm* tmp;

	time(&t);
	tmp = localtime(&t);
	strftime(buf, 64, "%Y-%m-%d %H:%M:%S", tmp);
	return buf;
}

struct HeadMsg {
	u_long dataLen;         //�����ܳ���
	u_short len;			// ���ݳ��ȣ�16λ
	u_short checkSum;		// У��ͣ�16λ
	unsigned char type;		// ��Ϣ����
	unsigned char seq;		// ���кţ����Ա�ʾ0-255
	unsigned char fileNum;  //�ļ���
	unsigned char fileTyp;
};

struct Package {
	HeadMsg hm;
	char data[8000];
};

struct RecvData {
	char* data;
	int dataLen;
	char fileNum;
};

// У��ͣ�ÿ16λ��Ӻ�ȡ�������ն�У��ʱ�����Ϊȫ0��Ϊ��ȷ��Ϣ
u_short checkSumVerify(u_short* msg, int length) {
	int count = (length + 1) / 2;
	u_long checkSum = 0;//32bit
	while (count--) {
		checkSum += *msg++;
		if (checkSum & 0xffff0000) {
			checkSum &= 0xffff;
			checkSum++;
		}
	}
	return ~(checkSum & 0xffff);
}

bool SendPkg(Package p, SOCKET sockSrv, SOCKADDR_IN addrClient)
{
	char Type[10];
	switch (p.hm.type) {
	case 2: strcpy(Type, "SYN_ACK"); break;
	case 4: strcpy(Type, "ACK"); break;
	case 8: strcpy(Type, "FIN_ACK"); break;
	}
	// ������Ϣ
	while (sendto(sockSrv, (char*)&p, sizeof(p), 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR)) == -1)
	{
		printf("%s [ ERR  ] Server: send [%s] ERROR:%s Seq=%d\n", timei(), Type, strerror(errno), p.hm.seq);

	}
	printf("%s [ INFO ] Server: [%s] Seq=%d\n", timei(), Type, p.hm.seq);
	if (!strcmp(Type, "ACK"))
		return true;
	// ��ʼ��ʱ
	clock_t start = clock();
	// �ȴ�������Ϣ
	Package p1;
	int addrlen = sizeof(SOCKADDR);
	while (true) {
		if (recvfrom(sockSrv, (char*)&p1, sizeof(p1), 0, (SOCKADDR*)&addrClient, &addrlen) > 0 && clock() - start <= WAIT_TIME) {
			u_short ckSum = checkSumVerify((u_short*)&p1, sizeof(p1));
			// �յ���Ϣ��Ҫ��֤��Ϣ���͡����кź�У���
			if (p1.hm.type == ACK && ckSum == 0)
			{
				printf("%s [ GET  ] Server: receive [ACK] from Client\n", timei());
				return true;
			}
			else {
				SendPkg(p, sockSrv, addrClient);
				return true;
				// ����ش������¼�ʱ
			}
		}
		else {
			SendPkg(p, sockSrv, addrClient);
			return true;
			// ��ʱ�ش������¼�ʱ
		}
	}
}

bool HandShake(SOCKET sockSrv, SOCKADDR_IN addrClient)
{
	Package p2;
	int len = sizeof(SOCKADDR);
	while (true)
	{
		if (recvfrom(sockSrv, (char*)&p2, sizeof(p2), 0, (SOCKADDR*)&addrClient, &len) > 0)
		{
			cout << "-----------------------------------CONNECTION-----------------------------------" << endl;
			int ck = checkSumVerify((u_short*)&p2, sizeof(p2));
			if (p2.hm.type == SYN && ck == 0)
			{
				printf("%s [ GET  ] Server: receive [SYN] from Client\n", timei());
				Package p3;
				p3.hm.type = SYN_ACK;
				p3.hm.seq = (lastAck + 1) % 256;
				lastAck = (lastAck + 2) % 256;
				p3.hm.checkSum = 0;
				p3.hm.checkSum = checkSumVerify((u_short*)&p3, sizeof(p3));
				SendPkg(p3, sockSrv, addrClient);
				break;
			}
			else
			{
				printf("%s [ ERR  ] Server: receive [SYN] ERROR\n", timei());
				return false;
			}
		}
	}
	return true;
}

bool WaveHand(SOCKET sockSrv, SOCKADDR_IN addrClient, Package p2)
{
	int len = sizeof(SOCKADDR);

	u_short ckSum = checkSumVerify((u_short*)&p2, sizeof(p2));
	if (p2.hm.type == FIN_ACK && ckSum == 0)
	{
		printf("%s [ GET  ] Server: receive [FIN, ACK] from Client\n", timei());
		p2.hm.type = ACK;
		p2.hm.seq = (lastAck + 1) % 256;
		lastAck = (lastAck + 2) % 256;
		p2.hm.checkSum = 0;
		p2.hm.checkSum = checkSumVerify((u_short*)&p2, sizeof(p2));
		SendPkg(p2, sockSrv, addrClient);
	}
	else
	{
		printf("%s [ ERR  ] Server: receive [FIN, ACK] ERROR\n", timei());
		return false;
	}


	Package p3;
	p3.hm.type = FIN_ACK;
	p3.hm.checkSum = 0;
	p3.hm.checkSum = checkSumVerify((u_short*)&p2, sizeof(p2));
	SendPkg(p3, sockSrv, addrClient);

	return true;
}

RecvData RecvMsg(SOCKET sockSrv, SOCKADDR_IN addrClient)
{
	Package p1;
	int addrlen = sizeof(SOCKADDR);
	int totalLen = 0;
	RecvData rd;
	rd.data = new char[100000000];
	// �ȴ�������Ϣ
	while (true) {

		// �յ���Ϣ��Ҫ��֤У��ͼ����к�
		if (recvfrom(sockSrv, (char*)&p1, sizeof(p1), 0, (SOCKADDR*)&addrClient, &addrlen) > 0)
		{
			if (p1.hm.type == FIN_ACK)
			{
				cout << "----------------------------------DISCONNECTION---------------------------------" << endl;
				rd.fileNum = '0';
				WaveHand(sockSrv, addrClient, p1);
				break;
			}
			Package p2;
			p2.hm.fileTyp = p2.hm.fileNum = p2.hm.checkSum = p2.hm.dataLen = p2.hm.len = 0;

			int ck = !checkSumVerify((u_short*)&p1, sizeof(p1));
			if (p1.hm.seq == (lastAck + 1) % 256 && ck == 1)  //���к�����

			{
				lastAck = (lastAck + 1) % 256;
				p2.hm.type = ACK;
				p2.hm.seq = lastAck;
				p2.hm.checkSum = checkSumVerify((u_short*)&p2, sizeof(p2));
				while (sendto(sockSrv, (char*)&p2, sizeof(p2), 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR)) == -1)
					printf("%s [ ERR  ] Server: send [ACK] ERROR:%s Seq=%d\n", timei(), strerror(errno), p2.hm.seq);
				printf("%s [ INFO ] Server: [ACK] Seq=%d\n", timei(), p2.hm.seq);
				memcpy(rd.data + totalLen, (char*)&p1 + sizeof(p1.hm), p1.hm.len);
				totalLen += (int)p1.hm.len;
				rd.dataLen = p1.hm.dataLen;
				rd.fileNum = p1.hm.fileNum;
				if (totalLen == p1.hm.dataLen)
				{
					break;
				}
			}
			else {
				// ����ش�
				p2.hm.type = NAK;
				p2.hm.seq = lastAck;
				p2.hm.checkSum = checkSumVerify((u_short*)&p2, sizeof(p2));
				sendto(sockSrv, (char*)&p2, BUFFER_SIZE, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
			}
		}
	}
	return rd;
}

void main()
{
	WSADATA wsaData; //����socket��ϸ��Ϣ
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	printf("%s [ OK   ] WSAStartup Complete!\n", timei());

	struct timeval timeo = { 20,0 };
	socklen_t lens = sizeof(timeo);

	SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	setsockopt(sockSrv, SOL_SOCKET, SO_RCVTIMEO, (char*) & timeo, lens);

	//�����
	SOCKADDR_IN addrSrv = { 0 };
	addrSrv.sin_family = AF_INET;//��AF_INET��ʾTCP/IPЭ�顣
	addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//����Ϊ���ػػ���ַ
	addrSrv.sin_port = htons(4567);
	printf("%s [ INFO ] Local Machine IP Address: 127.0.0.1\n", timei());

	if (bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) != SOCKET_ERROR)
		printf("%s [ OK   ] Bind Success!\n", timei());
	else
		printf("%s [ ERR  ] Bind Failure!\n", timei());
	SOCKADDR_IN addrClient = { 0 };

	int len = sizeof(SOCKADDR);


	if (HandShake(sockSrv, addrClient) == true)
	{
		printf("%s [ INFO ] Server: Connection Success\n", timei());
		cout << "------------------------------CONNECTION SUCCESSFUL------------------------------" << endl;
	}

	while (1)
	{
		RecvData rd;
		rd = RecvMsg(sockSrv, addrClient);
		if (rd.fileNum == '0')
			break;
		char file1[100] = ".\\file\\";
		if (rd.fileNum == '1' || rd.fileNum == '2' || rd.fileNum == '3')
			sprintf(file1, "%s%c.jpg", file1, rd.fileNum);
		else
			sprintf(file1, "%s%s", file1, "helloworld.txt");
		ofstream out(file1, ofstream::binary);
		for (int i = 0; i < rd.dataLen; i++)
		{
			out << rd.data[i];
		}
		out.close();
		printf("�յ��ļ�: %s\n", file1);
	}
	printf("%s [ INFO ] Server: Disconnection Success\n", timei());
	cout << "-----------------------------DISCONNECTION SUCCESSFUL----------------------------" << endl;

	closesocket(sockSrv);

	WSACleanup();
}