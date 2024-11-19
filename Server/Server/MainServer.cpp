#include "framework.h"
#include "Packet.h"

#define SERVERPORT 9000
#define BUFSIZE    512

CRITICAL_SECTION cs;
CRITICAL_SECTION cs2;
std::queue<int> RecvQueue;
std::queue<int> SendQueue;

bool SendInputPacket(SOCKET& sock, const Input_Packet& inputPacket)
{
	int retval;

	retval = send(sock, (char*)&inputPacket, sizeof(inputPacket), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send error");
	}

	return true;

}

bool SendPlayerIDResponse(SOCKET& sock, const PlayerIDResponsePacket& responsePacket)
{
	int retval;
	
	retval = send(sock, (char*)&responsePacket, sizeof(responsePacket), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send error");
	}

	return true;
}

void ProcessTempQueue(std::queue<int>& tempQueue) {
	while (!tempQueue.empty()) {
		int item = tempQueue.front();
		tempQueue.pop();
	}
}

DWORD WINAPI UpdateThreadFunc(LPVOID lpParam)
{
	while (true)
	{
		EnterCriticalSection(&cs);

		if (!RecvQueue.empty()) {
			std::queue<int> TempQueue = RecvQueue;

			RecvQueue = {};
			LeaveCriticalSection(&cs);
			ProcessTempQueue(TempQueue);

			EnterCriticalSection(&cs2);
		}

		if (!SendQueue.empty()) {
			while (!SendQueue.empty()) {
				int dataToSend = SendQueue.front();
				SendQueue.pop();
			}
		}

		LeaveCriticalSection(&cs2);
	}
	
	return 0;
}

DWORD WINAPI CommunicationThread(LPVOID lpParam) {

}

int main(int argc, char* argv[])
{
	int retval;;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// ���� ����
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	HANDLE hThread, hUpdateThread;

	// CS �ʱ�ȭ
	InitializeCriticalSection(&cs);

	// UpdateThread
	hUpdateThread = CreateThread(NULL, 0, UpdateThreadFunc, NULL, 0, NULL);
	if (hUpdateThread == NULL) {
		err_display("CreateThread() for UpdateThreadFunc");
		return 1;
	}

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		
		// ������ ����
		hThread = CreateThread(NULL, 0, CommunicationThread, (LPVOID)client_sock, 0, NULL);
		if (hThread == NULL)
		{
			closesocket(client_sock);
		}
		else
		{
			CloseHandle(hThread);
		}
		

	}

	WaitForSingleObject(hUpdateThread, INFINITE);

	// CS ����
	DeleteCriticalSection(&cs);

	// ���� �ݱ�
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}
