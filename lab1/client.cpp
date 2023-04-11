#include<iostream>
#include<winsock2.h>
#include<cstring>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

#pragma comment(lib,"ws2_32.lib")   //socket��̬���ӿ�

const int BUF_SIZE = 1024;
char buf2[64];

DWORD WINAPI recvMsgThread(LPVOID IpParameter);

char* timei()
{
	time_t t;
    struct tm *tmp;

	time(&t);
    tmp = localtime(&t);
	strftime(buf2, 64, "%Y-%m-%d %H:%M:%S", tmp);
	return buf2;
}

void main()
{
	printf("%s [ GET  ] Input the IP of server\n",timei());
	char ip[20]={0};
	gets(ip);
	printf("%s [ GET  ] Input the Port of server\n",timei());
	int port;
	cin>>port;

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData); //���汾��Ϊ2�����汾��Ϊ2
	printf("%s [ OK   ] WSAStartup Complete!\n",timei());

	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
	if(sockClient !=INVALID_SOCKET)
		printf("%s [ OK   ] Socket Created!\n",timei());
	
	//�ͻ���
	SOCKADDR_IN cliAddr = { 0 };
	cliAddr.sin_family = AF_INET;
	cliAddr.sin_addr.s_addr = inet_addr("127.0.0.1");//IP��ַ
	cliAddr.sin_port = htons(12345);//�˿ں�

	//�����
	SOCKADDR_IN addrSrv = { 0 };
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_addr.S_un.S_addr = inet_addr(ip);
	addrSrv.sin_port = htons(port);

	if (connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		cout << timei()<<" [ INFO ] ���ӳ��ִ��󣬴������" << WSAGetLastError() << endl;
	}
	else
		printf("%s [ INFO ] Server connected succesfully!\n",timei());

	//����������Ϣ�߳�
	CloseHandle(CreateThread(NULL, 0, recvMsgThread, (LPVOID)&sockClient, 0, 0));

	while (1)
	{
		char buf[BUF_SIZE] = { 0 };
		cin.getline(buf, sizeof(buf));
		if (strcmp(buf, "exit") == 0)
		{
			break;
		}
		send(sockClient, buf, sizeof(buf), 0);
	}

	closesocket(sockClient);
	WSACleanup();

}

DWORD WINAPI recvMsgThread(LPVOID Iparam)//������Ϣ���߳�
{
	SOCKET cliSock = *(SOCKET*)Iparam;//��ȡ�ͻ��˵�SOCKET����

	while (1)
	{
		char buffer[BUF_SIZE] = { 0 };
		int nrecv = recv(cliSock, buffer, sizeof(buffer), 0);//nrecv�ǽ��յ����ֽ���
		if (nrecv > 0)//������յ����ַ�������0
		{
			cout << buffer << endl;
		}
		else if (nrecv < 0)//������յ����ַ���С��0��˵���Ͽ�����
		{
			printf("%s [ INFO ] Disconnect from server\n",timei());
			break;
		}
	}
	return 0;
}