#include "Header.h"
#include "Socket.h"
#include "Task.h"

bool LinkToClient(SOCKET& serverSocket, SOCKET& clientSocket) {
	// Khởi tạo Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "Error: WSAStartup failed." << endl;
		return false;
	}

	// Tạo socket
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		cout << "Error: Socket creation failed." << endl;
		WSACleanup();
		return false;
	}

	// Thiết lập địa chỉ server
	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY; // Lắng nghe từ tất cả các máy tính muốn kết nối
	serverAddr.sin_port = htons(PORT);       // Cổng của server

	// Gắn socket với địa chỉ và cổng
	if (::bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		cout << "Error: Bind failed. Error code: " << WSAGetLastError() << endl;
		closesocket(serverSocket);
		WSACleanup();
		return false;
	}

	// Nghe kết nối từ client
	if (listen(serverSocket, 1) == SOCKET_ERROR) {
		cout << "Error: Listen failed." << endl;
		closesocket(serverSocket);
		WSACleanup();
		return false;
	}

	cout << "Waiting for client connection...";

	// Chấp nhận kết nối từ client
	struct sockaddr_in clientAddr;
	int clientAddrSize = sizeof(clientAddr);
	clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
	if (clientSocket == INVALID_SOCKET) {
		cout << "Error: Accept failed." << endl;
		closesocket(serverSocket);
		WSACleanup();
		return false;
	}

	cout << endl << "Client connected!" << endl;

	return true;
}

void SendFileToClient(SOCKET clientSocket, string file_path) {
	// Mở file ở chế độ nhị phân
	ifstream file(file_path, ios::binary);

	// Kiểm tra xem file có mở được không
	if (!file.is_open()) {
		cout << "Error: Failed to open file " << file_path << endl;
		send(clientSocket, (char*)("Error: Failed to open file " + file_path).c_str(), sizeof(("Error: Failed to open file " + file_path).c_str()), 0);
		return;
	}

	// Lấy kích thước file
	file.seekg(0, ios::end);
	int file_size = file.tellg(); // Lấy vị trí cuối cùng trong file
	file.seekg(0, ios::beg); // Di chuyển con trỏ file về đầu

	// Gửi kích thước file (4 byte)
	if (send(clientSocket, reinterpret_cast<char*>(&file_size), sizeof(file_size), 0) == SOCKET_ERROR) {
		cout << "Error: Failed to send file size." << endl;
		send(clientSocket, "Error: Failed to send file size.", strlen("Error: Failed to send file size."), 0);
		file.close();
		return;
	}

	if (file_size >= 25000000) {
		cout << "Error: Oversize file" << endl;
		send(clientSocket, "Error: Oversize file", strlen("Error: Oversize file"), 0);
		return;
	}

	// Gửi dữ liệu file theo từng khối ()
	char buffer[BUFFER_SIZE];
	int bytes_sent = 0;

	// Lặp qua các khối dữ liệu trong file và gửi đến client
	while (file) {
		// Đọc dữ liệu vào buffer
		file.read(buffer, sizeof(buffer));
		int bytes_read = file.gcount(); // Lấy số byte đã đọc
		if (bytes_read == 0) break;  // Nếu không còn dữ liệu để đọc thì thoát vòng lặp

		int total_sent = 0;
		// Gửi dữ liệu trong buffer đến client theo từng phần nhỏ nếu cần
		while (total_sent < bytes_read) {
			int result = send(clientSocket, buffer + total_sent, bytes_read - total_sent, 0);
			if (result == SOCKET_ERROR) {
				cout << "Error: Failed to send file data." << endl;
				send(clientSocket, "Error: Failed to send file data.", strlen("Error: Failed to send file data."), 0);
				file.close();
				return;
			}
			total_sent += result; // Cập nhật số byte đã gửi
		}

		bytes_sent += total_sent; // Cập nhật tổng số byte đã gửi
	}

	// In ra thông tin kết quả
	cout << "Sent successful!" << endl;
	cout << "Sent: " << bytes_sent << "/" << file_size << " bytes" << endl;
	file.close(); // Đóng file sau khi gửi xong
}

void CommunicateWithClient(SOCKET& serverSocket, SOCKET& clientSocket) {
	char buffer[BUFFER_SIZE] = { 0 };
	int bytesReceived = -1;

	// Vòng lặp liên tục nhận và gửi tin nhắn
	while (true) {
		cout << "\rWaiting for client request...";

		// Nhận tin nhắn từ client
		memset(buffer, 0, BUFFER_SIZE);
		bytesReceived = -1;
		bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);

		if (bytesReceived > 0) {
			cout << endl << endl << "Client request: " << buffer << endl;

			cout << "RESULT:" << endl;
			// Nếu client gửi "Close Connect", kết thúc phiên làm việc
			if (strcmp(buffer, "Close Connect") == 0) {

				string closeMessage = "Close Server";
				send(clientSocket, closeMessage.c_str(), strlen(closeMessage.c_str()), 0);

				Sleep(3000);
				closesocket(clientSocket);
				closesocket(serverSocket);
				WSACleanup();
				cout << "Client disconnected!" << endl;
				break;
			}

			// Nếu client gửi yêu cầu "List App", liệt kê ứng dụng và gửi file danh sách
			if (strcmp(buffer, "List Apps") == 0) {
				if (ListApps()) {  // Liệt kê các ứng dụng đang chạy
					SendFileToClient(clientSocket, "List_Apps.txt");  // Gửi danh sách ứng dụng cho client
					remove("List_Apps.txt");
					cout << "List Apps successful!" << endl;
				}
				else {
					SendFileToClient(clientSocket, "List_Error.txt");  // Gửi danh thong bao loi cho client
					cout << "Error: List Apps fail." << endl;
				}

				continue;
			}

			// Nếu client yêu cầu "Run App", mở ứng dụng theo đường dẫn cho trước
			if (strcmp(buffer, "Start App") == 0) {
				memset(buffer, 0, BUFFER_SIZE);
				if (recv(clientSocket, buffer, BUFFER_SIZE, 0) > 0) {
					StartApp(clientSocket, buffer);  // Chạy ứng dụng theo tên nhận được
				}
				else {
					cout << "Error: Cannot receive message from CLIENT." << endl;
					send(clientSocket, "Error: Cannot receive message from CLIENT.", strlen("Error: Cannot receive message from CLIENT."), 0);
				}

				continue;
			}

			// Nếu client yêu cầu "Close App", đóng một ứng dụng theo tiêu đề cửa sổ cho trước
			if (strcmp(buffer, "Close App") == 0) {
				memset(buffer, 0, BUFFER_SIZE);
				if (recv(clientSocket, buffer, BUFFER_SIZE, 0) > 0) {
					wstring name_app(buffer, buffer + strlen(buffer));
					CloseApp(clientSocket, name_app);
				}
				else {
					cout << "Error: Cannot receive message from CLIENT." << endl;
					send(clientSocket, "Error: Cannot receive message from CLIENT.", strlen("Error: Cannot receive message from CLIENT."), 0);
				}

				continue;
			}

			// Nếu client yêu cầu "List Services", liệt kê tất cả dịch vụ có trong máy
			if (strcmp(buffer, "List Services") == 0) {
				if (ListServices()) {  // Liệt kê các ứng dụng đang chạy
					SendFileToClient(clientSocket, "List_Services.txt");  // Gửi danh sách ứng dụng cho client
					remove("List_Services.txt");
					cout << "List Services successfull!" << endl;
				}
				else {
					SendFileToClient(clientSocket, "List_Error.txt");  // Gửi thong bao loi cho client
					cout << "Error: List Services fail." << endl;
				}

				continue;
			}

			// Nếu client yêu cầu "Find File", tìm kiếm file theo thư mục và tên file
			if (strcmp(buffer, "Find File") == 0) {
				memset(buffer, 0, BUFFER_SIZE);
				if (recv(clientSocket, buffer, BUFFER_SIZE, 0) > 0) {
					string directory(buffer);  // Đọc thư mục từ client

					memset(buffer, 0, BUFFER_SIZE);
					if (recv(clientSocket, buffer, BUFFER_SIZE, 0) > 0) {
						string search_name(buffer);  // Đọc tên file từ client
						string result_path;

						if (FindFile(directory, search_name, result_path)) {  // Tìm kiếm file
							cout << "File found - Path: " << result_path << "!" << endl;
							send(clientSocket, ("File found - Path: " + result_path + "!").c_str(), strlen(("File found - Path: " + result_path + "!").c_str()), 0);
						}
						else {
							cout << "File not found!" << endl;
							send(clientSocket, "File not found!", strlen("File not found!"), 0);
						}
					}
					else {
						cout << "Error: Cannot receive message from CLIENT." << endl;
						send(clientSocket, "Error: Cannot receive message from CLIENT.", strlen("Error: Cannot receive message from CLIENT."), 0);
					}
				}
				else {
					cout << "Error: Cannot receive message from CLIENT." << endl;
					send(clientSocket, "Error: Cannot receive message from CLIENT.", strlen("Error: Cannot receive message from CLIENT."), 0);
				}

				continue;
			}

			// Nếu client yêu cầu "Copy File", sao chép file từ nguồn đến đích và gửi lại file cho admin
			if (strcmp(buffer, "Copy File") == 0) {
				memset(buffer, 0, BUFFER_SIZE);
				if (recv(clientSocket, buffer, BUFFER_SIZE, 0) > 0) {
					string source(buffer);  // Đọc địa chỉ file nguồn

					memset(buffer, 0, BUFFER_SIZE);
					if (recv(clientSocket, buffer, BUFFER_SIZE, 0) > 0) {
						string destination(buffer);  // Đọc địa chỉ file đích
						string message = "";

						if (Copy_File(source, destination)) {  // Sao chép file
							SendFileToClient(clientSocket, destination);  // Gửi file đích cho client

							Sleep(3000);  // Tạm dừng 3 giây

							message = "File copied from " + source + " to " + destination;
						}
						else {
							SendFileToClient(clientSocket, "ERROR.png");

							Sleep(3000);  // Tạm dừng 3 giây

							message = "Error: Unable to copy file.";
						}

						cout << message << endl;
						send(clientSocket, message.c_str(), strlen(message.c_str()), 0);
					}
					else {
						cout << "Error: Cannot receive message from CLIENT." << endl;
						send(clientSocket, "Error: Cannot receive message from CLIENT.", strlen("Error: Cannot receive message from CLIENT."), 0);
					}
				}
				else {
					cout << "Error: Cannot receive message from CLIENT." << endl;
					send(clientSocket, "Error: Cannot receive message from CLIENT.", strlen("Error: Cannot receive message from CLIENT."), 0);
				}

				continue;
			}

			// Nếu client yêu cầu "Delete File", xóa file theo đường dẫn
			if (strcmp(buffer, "Delete File") == 0) {
				memset(buffer, 0, BUFFER_SIZE);
				if (recv(clientSocket, buffer, BUFFER_SIZE, 0) > 0) {
					string path(buffer);  // Đọc đường dẫn file cần xóa

					if (Delete_File(path)) {  // Xóa file
						cout << "Successfully deleted!" << endl;
						send(clientSocket, "Successfully deleted!", strlen("Successfully deleted!"), 0);
					}
					else {
						cout << "Error: Failed to delete." << endl;
						send(clientSocket, "Error: Failed to delete.", strlen("Error: Failed to delete."), 0);
					}
				}
				else {
					cout << "Error: Cannot receive message from CLIENT." << endl;
					send(clientSocket, "Error: Cannot receive message from CLIENT.", strlen("Error: Cannot receive message from CLIENT."), 0);
				}

				continue;
			}

			// Nếu client yêu cầu "Capture Screen", chụp màn hình và gửi cho client
			if (strcmp(buffer, "Capture Screen") == 0) {
				if (CaptureScreen()) {  // Chụp màn hình
					SendFileToClient(clientSocket, "Capture_Screen.png");  // Gửi ảnh chụp màn hình cho client
					remove("Capture_Screen.png");

					Sleep(3000);
					cout << "Successfully captured screen!" << endl;
					send(clientSocket, "Successfully captured screen!", strlen("Successfully captured screen!"), 0);
				}
				else {
					SendFileToClient(clientSocket, "ERROR.png");

					Sleep(3000);
					cout << "Error: Failed to capture screen." << endl;
					send(clientSocket, "Error: Failed to capture screen.", strlen("Error: Failed to capture screen."), 0);
				}

				continue;
			}

			// Nếu client yêu cầu "Web Camera", thực hiện quay Webcam
			if (strcmp(buffer, "Web Camera") == 0) {
				memset(buffer, 0, BUFFER_SIZE);
				if (recv(clientSocket, buffer, BUFFER_SIZE, 0) > 0) {
					int duration_seconds = stoi(buffer);  // Đọc thời gian từ client
					WebCamera(duration_seconds);
					SendFileToClient(clientSocket, "Web_Camera.avi");  // Gửi file kết quả cho client
					remove("Web_Camera.avi");

					Sleep(3000);  // Tạm dừng 2 giây

					string message = "This is result by webcam ran for" + to_string(duration_seconds) + "s";
					send(clientSocket, message.c_str(), strlen(message.c_str()), 0);

					cout << "Use Web Camera successfull!" << endl;
				}
				else {
					cout << "Error: Cannot receive message from CLIENT." << endl;
					send(clientSocket, "Error: Cannot receive message from CLIENT.", strlen("Error: Cannot receive message from CLIENT."), 0);
				}

				continue;
			}

			// Nếu client yêu cầu "Key Logger", ghi lại phím bấm trong thời gian nhất định
			if (strcmp(buffer, "Key Logger") == 0) {
				memset(buffer, 0, BUFFER_SIZE);
				if (recv(clientSocket, buffer, BUFFER_SIZE, 0) > 0) {
					int duration_seconds = stoi(buffer);  // Đọc thời gian từ client
					KeyLogger(duration_seconds);  // Bắt đầu ghi lại phím
					SendFileToClient(clientSocket, "Key_Logger.txt");  // Gửi file kết quả cho client
					remove("Key_Logger.txt");

					Sleep(3000);  // Tạm dừng 2 giây

					string message = "This is result by key logger ran for " + to_string(duration_seconds) + "s!";
					send(clientSocket, message.c_str(), strlen(message.c_str()), 0);

					cout << "Key Logger successfull!" << endl;
				}
				else {
					cout << "Error: Cannot receive message from CLIENT." << endl;
					send(clientSocket, "Error: Cannot receive message from CLIENT.", strlen("Error: Cannot receive message from CLIENT."), 0);
				}

				continue;
			}

			// Nếu client yêu cầu "Lock Keyboard", khóa bàn phím trong thời gian nhất định
			if (strcmp(buffer, "Lock Keyboard") == 0) {
				if (recv(clientSocket, buffer, BUFFER_SIZE, 0) > 0) {
					int duration_seconds = stoi(buffer);  // Đọc thời gian từ client
					LockKeyboard(clientSocket, duration_seconds);  // Khóa bàn phím

					cout << "Lock Keyboard successfull!" << endl;
				}
				else {
					cout << "Error: Cannot receive message from CLIENT." << endl;
					send(clientSocket, "Error: Cannot receive message from CLIENT.", strlen("Error: Cannot receive message from CLIENT."), 0);
				}

				continue;
			}

			// Nếu client yêu cầu "Restart", thực hiện khởi động lại
			if (strcmp(buffer, "Restart") == 0) {
				cout << "Client disconnected!" << endl;
				closesocket(clientSocket);
				closesocket(serverSocket);
				WSACleanup();

				send(clientSocket, "Restart successfully!", strlen("Restart successfully!"), 0);

				cout << "Starting countdown...\n";
				for (int i = 5; i > 0; --i) {
					cout << "\rShutting down in " << i << " seconds...";
					Sleep(1000);  // Dừng 1 giây
				}
				cout << "Restarting now...\n";

				RestartComputer();  // Khởi động lại máy
			}

			// Nếu client yêu cầu "Shutdown", thực hiện tắt nguồn
			if (strcmp(buffer, "Shutdown") == 0) {
				cout << "Client disconnected!" << endl;
				closesocket(clientSocket);
				closesocket(serverSocket);
				WSACleanup();

				// Thực hiện tắt máy
				send(clientSocket, "Shutdown successfully!", strlen("Shutdown successfully!"), 0);

				cout << "Starting countdown...\n";
				for (int i = 5; i > 0; --i) {
					cout << "\rShutting down in " << i << " seconds...";
					Sleep(1000);  // Dừng 1 giây
				}
				cout << "Shutting down now...\n";

				ShutDownComputer();  // Tắt máy
			}
		}
		else if (bytesReceived == 0) {
			cout << endl << "Error: Client disconnected gracefully." << endl;
			send(clientSocket, "Error: Client disconnected gracefully.", strlen("Error: Client disconnected gracefully."), 0);
		}
		else {
			cout << endl << "Error: Cannot receiving data from client." << endl;
			send(clientSocket, "Error: Cannot receiving data from client.", strlen("Error: Cannot receiving data from client."), 0);
		}
	}

	// Đóng kết nối khi kết thúc
	closesocket(clientSocket);
	WSACleanup();  // Dọn dẹp Winsock
	return;
}
