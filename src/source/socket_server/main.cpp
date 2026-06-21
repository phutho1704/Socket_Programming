#include "Socket.h"

int main() {

	SOCKET serverSocket, clientSocket;

	if (LinkToClient(serverSocket, clientSocket))
		CommunicateWithClient(serverSocket, clientSocket);
	else
	{
		closesocket(clientSocket);
		closesocket(serverSocket);
		WSACleanup();
	}

	return false;
}
