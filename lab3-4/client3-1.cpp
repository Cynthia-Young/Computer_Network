#include <winsock2.h> 
#include<iostream>
#include<fstream>
#include<cstring>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <WS2tcpip.h>
#include<ctime>
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
unsigned char seq = 0; // ��ʼ��8λ���к�

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
	u_long dataLen;
	u_short len;			// ���ݳ��ȣ�16λ �65535λ 8191���ֽ�
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

bool SendPkg(Package p, SOCKET sockClient, SOCKADDR_IN addrSrv)
{
	char Type[10];
	switch (p.hm.type) {
	case 1: strcpy(Type, "SYN"); break;
	case 4: strcpy(Type, "ACK"); break;
	case 8: strcpy(Type, "FIN_ACK"); break;
	case 16:strcpy(Type, "PSH"); break;
	}

	// ������Ϣ
	while (sendto(sockClient, (char*)&p, sizeof(p), 0, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) == -1)
	{
		printf("%s [ ERR  ] Client: send [%s] ERROR:%s Seq=%d\n", timei(), Type, strerror(errno), p.hm.seq);

	}
	printf("%s [ INFO ] Client: [%s] Seq=%d\n", timei(), Type, p.hm.seq);

	if (!strcmp(Type, "ACK"))
		return true;
	// ��ʼ��ʱ
	clock_t start = clock();
	// �ȴ�������Ϣ
	Package p1;
	int addrlen = sizeof(SOCKADDR);
	while (true) {
		if (recvfrom(sockClient, (char*)&p1, sizeof(p1), 0, (SOCKADDR*)&addrSrv, &addrlen) > 0 && clock() - start <= WAIT_TIME) {
			// �յ���Ϣ��Ҫ��֤��Ϣ���͡����кź�У���
			u_short ckSum = checkSumVerify((u_short*)&p1, sizeof(p1));
			if ((p1.hm.type == SYN_ACK && !strcmp(Type, "SYN")) && p1.hm.seq == seq && ckSum == 0)
			{
				printf("%s [ GET  ] Client: receive [SYN, ACK] from Server\n", timei());
				return true;
			}
			else if ((p1.hm.type == ACK && (!strcmp(Type, "FIN_ACK") || !strcmp(Type, "PSH"))) && p1.hm.seq == seq && ckSum == 0)
			{
				printf("%s [ GET  ] Client: receive [ACK] from Server\n", timei());
				return true;
			}
			else {
				SendPkg(p, sockClient, addrSrv);
				return true;
				// ����ش������¼�ʱ
			}
		}
		else {
			SendPkg(p, sockClient, addrSrv);
			return true;
			// ��ʱ�ش������¼�ʱ
		}
	}
}

bool HandShake(SOCKET sockClient, SOCKADDR_IN addrSrv)
{
	char sendBuf[BUFFER_SIZE] = {};
	cin.getline(sendBuf, BUFFER_SIZE);
	if (strcmp(sendBuf, "connect") != 0)
		return false;
	cout << "-----------------------------------CONNECTION-----------------------------------" << endl;
	Package p1;
	p1.hm.type = SYN;
	p1.hm.seq = seq;
	p1.hm.checkSum = 0;
	p1.hm.checkSum = checkSumVerify((u_short*)&p1, sizeof(p1));
	int len = sizeof(SOCKADDR);
	SendPkg(p1, sockClient, addrSrv);
	seq = (seq + 1) % 256;
	p1.hm.type = ACK;
	p1.hm.seq = seq;
	p1.hm.checkSum = 0;
	p1.hm.checkSum = checkSumVerify((u_short*)&p1, sizeof(p1));

	if (sendto(sockClient, (char*)&p1, sizeof(p1), 0, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) != -1)
	{
		printf("%s [ INFO ] Client: [ACK] Seq=%d\n", timei(), seq);
		seq = (seq + 1) % 256;
		return true;
	}
	else
	{
		printf("%s [ ERR  ] Client: send [ACK] ERROR\n", timei());
		return false;
	}
}

bool WaveHand(SOCKET sockClient, SOCKADDR_IN addrSrv)
{
	/*char sendBuf[BUFFER_SIZE] = {};
	cin.getline(sendBuf, BUFFER_SIZE);
	if(strcmp(sendBuf,"disconnect")!=0)
		return false;*/
	cout << "----------------------------------DISCONNECTION---------------------------------" << endl;
	Package p1;
	p1.hm.type = FIN_ACK;
	p1.hm.seq = seq;
	p1.hm.checkSum = 0;
	p1.hm.checkSum = checkSumVerify((u_short*)&p1, sizeof(p1));
	int len = sizeof(SOCKADDR);

	SendPkg(p1, sockClient, addrSrv);
	Package p4;

	while (true)
	{
		if (recvfrom(sockClient, (char*)&p4, sizeof(p4), 0, (SOCKADDR*)&addrSrv, &len) > 0)
		{
			/*memcpy(&s1, recvBuf_s1, sizeof(s1));*/
			if (p4.hm.type == FIN_ACK)
			{
				printf("%s [ GET  ] Client: receive [FIN, ACK] from Server\n", timei());
				p4.hm.type = ACK;
				p4.hm.checkSum = 0;
				p4.hm.checkSum = checkSumVerify((u_short*)&p4, sizeof(p4));
				SendPkg(p4, sockClient, addrSrv);
				break;
			}
			else
			{
				printf("%s [ ERR  ] Server: receive [FIN, ACK] ERROR\n", timei());
				return false;
			}
		}
	}

	return true;
}

bool SendMsg(char* data, SOCKET sockClient, SOCKADDR_IN addrSrv, int dataLen, char fileNum)
{
	int sentLen = 0;
	for (int i = 0; i < dataLen / 8000 + 1; i++)
	{
		// ������Ϣͷ
		Package p;
		p.hm.dataLen = dataLen;
		p.hm.seq = seq; //need to check
		p.hm.type = PSH;
		p.hm.checkSum = 0;
		p.hm.fileNum = fileNum;
		if (fileNum == '1' || fileNum == '2' || fileNum == '3')
			p.hm.fileTyp = JPG;
		else
			p.hm.fileTyp = TXT;

		if (i != dataLen / 8000)
			p.hm.len = 8000;
		else
			p.hm.len = dataLen % 8000;

		// data��ŵ��Ƕ���Ķ��������ݣ�sentLen���ѷ��͵ĳ��ȣ���Ϊ�����η��͵�ƫ����
		memcpy(p.data, data + sentLen, p.hm.len); //�ѱ����������ݴ��ȥ
		sentLen += (int)p.hm.len; //���ͳ��ȼӳ�
		// ����У���
		p.hm.checkSum = checkSumVerify((u_short*)&p, sizeof(p));
		SendPkg(p, sockClient, addrSrv);
		seq = (seq + 1) % 256;
	}
	return true;
}

void main()
{
	WSADATA wsaData; //����socket��ϸ��Ϣ
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	printf("%s [ OK   ] WSAStartup Complete!\n", timei());

	struct timeval timeo = { 20,0 };
	socklen_t lens = sizeof(timeo);

	SOCKET sockClient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	setsockopt(sockClient, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeo, lens);

	//�����
	SOCKADDR_IN addrSrv = { 0 };
	addrSrv.sin_family = AF_INET;//��AF_INET��ʾTCP/IPЭ�顣
	addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//����Ϊ���ػػ���ַ
	addrSrv.sin_port = htons(4567);

	SOCKADDR_IN addrClient;

	int len = sizeof(SOCKADDR);

	if (HandShake(sockClient, addrSrv) == true)
	{
		printf("%s [ INFO ] Client: Connection Success\n", timei());
		cout << "------------------------------CONNECTION SUCCESSFUL------------------------------" << endl;
	}

	cout << "There are the files existing in the path.\n(1) 1.jpg\n(2) 2.jpg\n(3) 3.jpg\n(4)helloworld.txt\n";
	cout << "You can input the num '0' to quit\nPlease input the number of the file which you want to choose to send:\n";

	float seconds;
	float T;
	long long head, tail, freq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);

	while (1)
	{
		// �Զ����Ʒ�ʽ���ļ�
		char Buf[BUFFER_SIZE] = {};
		cin.getline(Buf, BUFFER_SIZE);
		char* data;
		if (strcmp(Buf, "1") == 0 || strcmp(Buf, "2") == 0 || strcmp(Buf, "3") == 0 || strcmp(Buf, "4") == 0)
		{
			char file[100] = "..\\test\\";
			if (Buf[0] == '1' || Buf[0] == '2' || Buf[0] == '3')
				sprintf(file, "%s%c.jpg", file, Buf[0]);
			else
				sprintf(file, "%s%s", file, "helloworld.txt");
			ifstream in(file, ifstream::in | ios::binary);
			int dataLen = 0;
			if (!in)
			{
				printf("%s [ INFO ] Client: can't open the file! Please retry\n", timei());
				continue;
			}
			// �ļ���ȡ��data
			BYTE t = in.get();
			char* data = new char[100000000];
			memset(data, 0, sizeof(data));
			while (in)
			{
				data[dataLen++] = t;
				t = in.get();
			}
			in.close();
			printf("read over\n");
			//cout<<dataLen<<endl;
			QueryPerformanceCounter((LARGE_INTEGER*)&head);

			SendMsg(data, sockClient, addrSrv, dataLen, Buf[0]);
			QueryPerformanceCounter((LARGE_INTEGER*)&tail);
			T = (tail - head) * 1000.0 / freq;
			printf("%s [ INFO ] Client: Send Finish! transmission delay :%fms\n", timei(), T);
		}
		else if (strcmp(Buf, "0") == 0)
			break;
		else
		{
			printf("%s [ ERR  ] Client: Invalidate Input\n", timei());
		}
	}

	if (WaveHand(sockClient, addrSrv) == true)
	{
		printf("%s [ INFO ] Client: Disconnection Success\n", timei());
		cout << "-----------------------------DISCONNECTION SUCCESSFUL----------------------------" << endl;
	}

	closesocket(sockClient);

	WSACleanup();
}