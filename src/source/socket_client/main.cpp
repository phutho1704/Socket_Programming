#include "Socket.h"

int main() {
	SOCKET clientSocket;
	Email email;

	if (LinkToServer(clientSocket, email)) {
		CommunicateWithServer(clientSocket, email);
	}
	else {
		closesocket(clientSocket);
		WSACleanup();
	}

	return false;
}