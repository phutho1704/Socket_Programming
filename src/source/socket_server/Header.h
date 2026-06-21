#pragma once
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <sstream>
#include <fstream>
#include <conio.h>
#include <regex>
#include <chrono>
#include <opencv2/opencv.hpp>

#pragma comment(lib, "ws2_32.lib") // Liên kết thư viện Winsock

#define PORT 8080
#define BUFFER_SIZE 1024

using namespace std;

