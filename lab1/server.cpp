#include <winsock2.h> 
#include<iostream>
#include<cstring>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

#pragma comment(lib,"ws2_32.lib")

const int BUFFER_SIZE = 1024;
char buf2[64];//����ʱ���
const int WAIT_TIME = 10;//�ͻ��˵ȴ��¼���ʱ�䣬��λms
const int MAX = 5;//��������������
int total = 0;//�Ѿ����ӵĿͷ��˷�����

SOCKET cliSock[MAX];
SOCKADDR_IN cliAddr[MAX];
WSAEVENT cliEvent[MAX];//�¼�

DWORD WINAPI handlerRequest(LPVOID lparam);//�������˴����߳�

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
	printf("%s [ INFO ] Start Server Manager\n",timei());

	WSADATA wsaData; //����socket��ϸ��Ϣ
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	printf("%s [ OK   ] WSAStartup Complete!\n",timei());

	printf("%s [ INFO ] Local Machine IP Address: 127.0.0.1\n",timei());

	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);
	if(sockSrv !=INVALID_SOCKET)
		printf("%s [ OK   ] Socket Created!\n",timei());

	//�����
	SOCKADDR_IN addrSrv = { 0 };
	addrSrv.sin_family = AF_INET;//��AF_INET��ʾTCP/IPЭ�顣
	addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//����Ϊ���ػػ���ַ
	addrSrv.sin_port = htons(819711);

	if(bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR))!= SOCKET_ERROR)
		printf("%s [ OK   ] Bind Success!\n",timei());

	WSAEVENT servEvent = WSACreateEvent();//����һ���¼�����
	WSAEventSelect(sockSrv, servEvent, FD_ALL_EVENTS);//���¼����󣬲��Ҽ��������¼�

	cliSock[0] = sockSrv;
	cliEvent[0] = servEvent;

	CloseHandle(CreateThread(NULL, 0, handlerRequest, (LPVOID)&sockSrv, 0, 0));
	//CreateThread�����µ��߳�  �߳���ֹ���к��̶߳�����Ȼ��ϵͳ�У�����ͨ��CloseHandle�������رո��̶߳���
	//����Ҫ�������ֱ�ӹر� ���ʱ���ں˶�������ü�����Ϊ0���̲߳�ͣ��
	printf("%s [ INFO ] Broadcast thread create success!\n",timei());

	listen(sockSrv, 5);
	printf("%s [ INFO ] Start listening...\n",timei());

	while (1) {
		char contentBuf[BUFFER_SIZE] = { 0 };
		char sendBuf[BUFFER_SIZE] = { 0 };
		cin.getline(contentBuf, sizeof(contentBuf));
		sprintf(sendBuf, "[ ���� ]%s", contentBuf);
		for (int j = 1; j <= total; j++)
		{
			send(cliSock[j], sendBuf, sizeof(sendBuf), 0);
		}
	}

	closesocket(sockSrv);

	WSACleanup();
}

DWORD WINAPI handlerRequest(LPVOID lparam)
{
	SOCKET sockSrv = *(SOCKET*)lparam;//LPVOIDΪ��ָ������
	while (1) //��ִͣ��
	{
		for (int i = 0; i < total + 1; i++)//i�����������ڼ����¼����ն�
		{
			//��ÿһ���նˣ��鿴�Ƿ����¼����ȴ�WAIT_TIME����
			int index = WSAWaitForMultipleEvents(1, &cliEvent[i], false, WAIT_TIME, 0);
			index -= WSA_WAIT_EVENT_0;//��ʱindexΪ�¼����¼������е�λ��

			if (index == WSA_WAIT_TIMEOUT || index == WSA_WAIT_FAILED)
			{
				continue;//���������߳�ʱ�����������ն�
			}
			else if (index == 0)
			{
				WSANETWORKEVENTS networkEvents;
				WSAEnumNetworkEvents(cliSock[i], cliEvent[i], &networkEvents);//�鿴��ʲô�¼�

				//�¼�ѡ��
				if (networkEvents.lNetworkEvents & FD_ACCEPT)
				{
					if (networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
					{
						cout << "����ʱ�������󣬴������" << networkEvents.iErrorCode[FD_ACCEPT_BIT] << endl;
						continue;
					}
					if (total + 1 <= MAX)
					{
						int nextIndex = total + 1;
						int addrLen = sizeof(SOCKADDR);
						SOCKET newSock = accept(sockSrv, (SOCKADDR*)&cliAddr[nextIndex], &addrLen); 
						if (newSock !=INVALID_SOCKET)
						{
							printf("%s [ OK   ] Accept Success!\n",timei());
							cliSock[nextIndex] = newSock;

							WSAEVENT newEvent = WSACreateEvent();
							WSAEventSelect(cliSock[nextIndex], newEvent, FD_CLOSE | FD_READ | FD_WRITE);
							cliEvent[nextIndex] = newEvent;

							total++;//�ͻ�������������

							printf("%s [ JOIN ] user#%d just join, welcome!\n",timei(),nextIndex,inet_ntoa(cliAddr[nextIndex].sin_addr));
							//inet_ntoa() �������ַת���ɡ�.��������ַ�����ʽ

							char buf[BUFFER_SIZE];
							sprintf(buf,"%s [ JOIN ] welcome user#%d enter the chat room",timei(),nextIndex);

							for (int j = i; j <= total; j++)
							{
								send(cliSock[j], buf, sizeof(buf), 0);
							}
						}
					}

				}
				else if (networkEvents.lNetworkEvents & FD_CLOSE)//�ͻ��˱��رգ����Ͽ�����
				{
					//i��ʾ�ѹرյĿͻ����±�
					total--;
					printf("%s [ EXIT ] user#%d just exit the chat room\n",timei(),i,inet_ntoa(cliAddr[i].sin_addr));
					//�ͷ�����ͻ��˵���Դ
					closesocket(cliSock[i]);
					WSACloseEvent(cliEvent[i]);

					for (int j = i; j < total; j++)
					{
						cliSock[j] = cliSock[j + 1];
						cliEvent[j] = cliEvent[j + 1];
						cliAddr[j] = cliAddr[j + 1];
					}

					char buf[BUFFER_SIZE];
					sprintf(buf,"%s [ EXIT ] user#%d just exit the chat room\n",timei(),i);

					for (int j = 1; j <= total; j++)
					{
						send(cliSock[j], buf, sizeof(buf), 0);
					}
				}
				else if (networkEvents.lNetworkEvents & FD_READ)//���յ���Ϣ
				{

					char buffer[BUFFER_SIZE] = { 0 };//�ַ������������ڽ��պͷ�����Ϣ
					char buffer2[BUFFER_SIZE] = { 0 };

					for (int j = 1; j <= total; j++)
					{
						int nrecv = recv(cliSock[j], buffer, sizeof(buffer), 0);//nrecv�ǽ��յ����ֽ���
						if (nrecv > 0)//������յ����ַ�������0
						{
							sprintf(buffer2,"%s [ RECV ] user#%d: %s",timei(),j,buffer);
							cout << buffer2 << endl;
							//�������ͻ�����ʾ���㲥�������ͻ��ˣ�
							for (int k = 1; k <= total; k++)
							{
								send(cliSock[k], buffer2, sizeof(buffer), 0);
							}
						}
					}
				}
			}
		}
	}
	return 0;
}