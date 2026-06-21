#pragma once
#include <winsock2.h>
#include <string>
#include "Struct.h"

using namespace std;

bool isValidIP(string& ip);

bool LinkToServer(SOCKET& clientSocket, Email& email);

bool ReceiveFileFromServer(SOCKET clientSocket,const string& filename, Email email, string& ErrorCode);

int doTask(SOCKET clientSocket, Email email, string& dataFromAdmin, char* message, char buffer[], string subjectEmail, string& bodyEmail, string& ErrorCode);

void CommunicateWithServer(SOCKET clientSocket, Email& email);