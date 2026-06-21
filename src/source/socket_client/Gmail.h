#pragma once
#include <string>
#include "Struct.h"

using namespace std;

bool AnalyzeMessage(string& dataFromAdmin, char*& message);

bool checkRequest(string& dataFromAdmin, char*& message);

size_t ReadCallback(void* ptr, size_t size, size_t nmemb, void* userData);

void sendTextEmail(Email email, string& subjectEmail,const string& bodyEmail);

void sendFileEmail(Email email, string filename, string subjectEmail, string bodyEmail);

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

string extractField(string& data,const string& field);

string uidLatestEmail(string username, string password);

bool readEmail(Email& email, string& body);