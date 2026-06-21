#pragma once
#include <winsock2.h>
#include <string>

using namespace std;

bool LinkToClient(SOCKET& serverSocket, SOCKET& clientSocket);

void SendFileToClient(SOCKET clientSocket, string file_path);

void CommunicateWithClient(SOCKET& serverSocket, SOCKET& clientSocket);