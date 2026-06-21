#include "Header.h"
#include "Socket.h"
#include "Gmail.h"

bool isValidIP(string& ip) {
	// Biểu thức chính quy kiểm tra địa chỉ IP theo chuẩn IPv4
	regex ip_pattern("^([0-9]{1,3}\\.){3}[0-9]{1,3}$");

	// Kiểm tra chuỗi ip có khớp với biểu thức chính quy hay không
	if (!regex_match(ip, ip_pattern)) {
		return false;
	}

	// Tách các phần của IP và kiểm tra giá trị của mỗi phần
	size_t start = 0, end;
	int partCount = 0;

	try {
		while ((end = ip.find('.', start)) != string::npos) {
			string part = ip.substr(start, end - start);
			int num = stoi(part);

			if (num < 0 || num > 255) {
				return false;
			}
			start = end + 1;
			partCount++;
		}

		// Kiểm tra phần cuối cùng của IP sau dấu chấm cuối cùng
		string part = ip.substr(start);
		int num = stoi(part);
		if (num < 0 || num > 255) {
			return false;
		}

	}
	catch (invalid_argument& e) {
		// Nếu không thể chuyển đổi chuỗi sang số, trả về false
		return false;
	}
	catch (out_of_range& e) {
		// Nếu số vượt quá giới hạn int, trả về false
		return false;
	}

	// Kiểm tra xem có đúng 3 dấu chấm trong địa chỉ IP
	return partCount == 3;
}

bool LinkToServer(SOCKET& clientSocket, Email& email) {
	// Thông báo cho admin khi không thể kết nối với server
	string subjectEmail = "[CLIENT] ESTABLISH A CONNECTION BETWEEN ADMIN AND THE SERVER";
	string bodyEmail = "";

	cout << "\rWaitting for server's IP from admin...";
	string current_UID = uidLatestEmail(email.Client, email.ClientPassword);
	email.UID = current_UID;

	while (email.UID == current_UID) {
		cout << "\rWaitting for server's IP from admin...";
		Sleep(2500);
		current_UID = uidLatestEmail(email.Client, email.ClientPassword);
	}

	// Thiết lập socket
	WSADATA wsaData;    // Gửi yêu cầu kết nối WSA để khởi tạo thư viện Winsock
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		cout << endl << "Error: WSAStartup failed: " << result << endl;
		bodyEmail = "Error: WSAStartup failed: " + to_string(result);
		sendTextEmail(email, subjectEmail, bodyEmail);
		return false;
	}

	// Tạo socket
	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	// Khởi tạo socket
	if (clientSocket == INVALID_SOCKET) {
		cout << "Errror: Socket creation failed. Error code: " << WSAGetLastError() << endl;
		bodyEmail = "Error: Socket creation failed. Error code: " + to_string(WSAGetLastError());
		sendTextEmail(email, subjectEmail, bodyEmail);
		WSACleanup();
		return false;
	}

	string buffer = "", IP = "";

	while (true) {
		if (current_UID > email.UID) {
			buffer = "";
			IP = "";
			if (readEmail(email, buffer) == true && buffer.size() >= 53 && buffer.back() == '.') {
				// Lấy địa chỉ IP từ email
				IP = buffer.substr(52, buffer.size() - 53);

				if (isValidIP(IP)) {
					// Cấu hình địa chỉ server
					struct sockaddr_in serverAddr;
					serverAddr.sin_family = AF_INET;						// Xác định giao thức IPv4
					serverAddr.sin_port = htons(PORT);						// Cổng kết nối
					inet_pton(AF_INET, IP.c_str(), &serverAddr.sin_addr);	// Chuyển IP sang định dạng nhị phân

					// Kết nối tới server
					if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
						cout << endl << "Error: Connection to SERVER failed. Error code: " << WSAGetLastError() << endl;
						bodyEmail = "Error: Connection to SERVER failed. Error code: " + to_string(WSAGetLastError());
						sendTextEmail(email, subjectEmail, bodyEmail);
						continue;
					}

					// Kết nối thành công
					cout << endl << "Successfully connected to SERVER!" << endl;
					sendFileEmail(email, "MENU.jpg", "[CLIENT] SUCCESSFULLY CONNECTED TO SERVER", "This is menu of tasks's socket!");	// Gửi menu cho người dùng ( thành công tất cả)
					return true;
				}
				else {
					// Nếu địa chỉ IP không hợp lệ
					cout << "Error: Invalid IP address entered." << endl;
					bodyEmail = "Error: Invalid IP address entered.";
					sendTextEmail(email, subjectEmail, bodyEmail);
				}
			}
			else {
				if (buffer == "Not Admin.") {
					cout << endl << "Their is someone trying to control your server computer through email." << endl;
					bodyEmail = "Their is someone trying to control your server computer through email.";
				}
				else {
					cout << "Error: Invalid request." << endl;
					bodyEmail = "Error: Invalid request.";
				}

				sendTextEmail(email, subjectEmail, bodyEmail);
			}
		}

		cout << "\rWaitting for server's IP from admin...";
		Sleep(1000);
		current_UID = uidLatestEmail(email.Client, email.ClientPassword);
	}

	return false;
}

bool ReceiveFileFromServer(SOCKET clientSocket, const string& filename, Email email, string& ErrorCode) {
	// Nhận kích thước tệp từ server (4 byte)
	int file_size = 0;
	int bytes_received = recv(clientSocket, reinterpret_cast<char*>(&file_size), sizeof(file_size), 0);
	if (bytes_received == SOCKET_ERROR) {
		cout << "Error: Cannot receive file size." << endl;
		ErrorCode = "Error: Failed to receive data from SERVER - Cannot receive file size.";
		return false;
	}

	char buffer[BUFFER_SIZE] = { 0 };

	if (file_size >= 25000000) {
		recv(clientSocket, buffer, BUFFER_SIZE, 0);
		ErrorCode = "Error: Oversize file.";
		return false;
	}

	// Mở file để ghi dữ liệu
	ofstream file(filename, ios::binary);
	if (!file.is_open()) {
		cout << "Error: Cannot open file to write: " << filename << endl;
		ErrorCode = "Error: Failed to receive data from SERVER - Cannot open file to write: " + filename + ".";
		return false;
	}

	int total_bytes_received = 0;

	// Nhận và ghi dữ liệu vào file
	while (total_bytes_received < file_size) {
		memset(buffer, 0, BUFFER_SIZE);
		bytes_received = recv(clientSocket, buffer, BUFFER_SIZE, 0);
		if (bytes_received == SOCKET_ERROR) {
			cout << "Error: Cannot receive data." << endl;
			ErrorCode = "Error: Failed to receive data from SERVER - Cannot receive data.";
			file.close();
			return false;
		}

		if (bytes_received == 0) {
			cout << "Error: Connection closed before receiving all data." << endl;
			ErrorCode = "Error: Failed to receive data from SERVER - Connection closed before receiving all data.";
			file.close();
			return false;
		}

		file.write(buffer, bytes_received);
		total_bytes_received += bytes_received;
	}

	// Nhận thành công file
	cout << "File received successfully!" << endl;
	cout << "Received: " << total_bytes_received << "/" << file_size << " bytes" << endl;
	file.close();
	return true;
}

int doTask(SOCKET clientSocket, Email email, string& dataFromAdmin, char* message, char buffer[], string subjectEmail, string& bodyEmail, string& ErrorCode) {
	// Xử lý yêu cầu liệt kê danh sách ứng dụng trên máy khi nhận được yêu cầu "List Apps"
	if (strcmp(message, "List Apps") == 0) {
		if (ReceiveFileFromServer(clientSocket, "List_Apps.txt", email, ErrorCode)) {  // Nhận file chứa danh sách ứng dụng từ server
			cout << "List Apps successfull!" << endl;

			bodyEmail = "This is file you requested!";
			sendFileEmail(email, "List_Apps.txt", subjectEmail, bodyEmail);

			return 0;
		}
		else return 2;
	}

	// Xử lý yêu cầu mở một ứng dụng trên máy khi nhận được yêu cầu "Start App"
	if (strcmp(message, "Start App") == 0) {
		if (AnalyzeMessage(dataFromAdmin, message)) {
			send(clientSocket, message, strlen(message), 0);

			memset(buffer, 0, BUFFER_SIZE);
			if (recv(clientSocket, buffer, BUFFER_SIZE, 0) != SOCKET_ERROR) {  // Nhận phản hồi từ server
				bodyEmail = (string)buffer;
				cout << bodyEmail << endl;

				sendTextEmail(email, subjectEmail, bodyEmail);

				return 0;
			}
			else return 2;
		}
		else return 1;
	}

	// Xử lý yêu cầu đóng một ứng dụng trên máy khi nhận được yêu cầu "Close App"
	if (strcmp(message, "Close App") == 0) {
		if (AnalyzeMessage(dataFromAdmin, message)) {
			send(clientSocket, message, strlen(message), 0);

			memset(buffer, 0, BUFFER_SIZE);
			if (recv(clientSocket, buffer, BUFFER_SIZE, 0) != SOCKET_ERROR) {  // Nhận phản hồi từ server
				bodyEmail = (string)buffer;
				cout << bodyEmail << endl;

				sendTextEmail(email, subjectEmail, bodyEmail);

				return 0;
			}
			else return 2;
		}
		else return 1;
	}

	// Xử lý yêu cầu liệt kê dịch vụ trên máy khi nhận được yêu cầu "List Services"
	if (strcmp(message, "List Services") == 0) {
		if (ReceiveFileFromServer(clientSocket, "List_Services.txt", email, ErrorCode)) {  // Nhận file chứa danh sách ứng dụng từ server
			cout << "List Services successfull!" << endl;

			bodyEmail = "This is file you requested!";
			sendFileEmail(email, "List_Services.txt", subjectEmail, bodyEmail);

			return 0;
		}
		else return 2;
	}

	// Xử lý yêu cầu tìm kiếm file trên máy khi nhận được yêu cầu "Find File"
	if (strcmp(message, "Find File") == 0) {
		if (AnalyzeMessage(dataFromAdmin, message)) {
			send(clientSocket, message, strlen(message), 0);

			Sleep(1000);  // Tạm dừng trước khi gửi tiếp yêu cầu

			if (AnalyzeMessage(dataFromAdmin, message)) {  // Phân tích dữ liệu nhận được từ server
				send(clientSocket, message, strlen(message), 0);

				memset(buffer, 0, BUFFER_SIZE);
				if (recv(clientSocket, buffer, BUFFER_SIZE, 0) != SOCKET_ERROR) {  // Nhận phản hồi từ server
					bodyEmail = (string)buffer;
					cout << bodyEmail << endl;

					sendTextEmail(email, subjectEmail, bodyEmail);

					return 0;
				}
				else return 2;
			}
			else return 1;
		}
		else return 1;
	}

	// Xử lý yêu cầu sao chép file có trong máy khi nhận được yêu cầu "Copy File"
	if (strcmp(message, "Copy File") == 0) {
		if (AnalyzeMessage(dataFromAdmin, message)) {
			send(clientSocket, message, strlen(message), 0);

			if (AnalyzeMessage(dataFromAdmin, message)) {
				Sleep(3000);  // Tạm dừng trước khi gửi tiếp yêu cầu
				send(clientSocket, message, strlen(message), 0);  // Gửi tiếp yêu cầu sao chép file

				int index = dataFromAdmin.find_last_of('\\');  // Lấy vị trí tên file trong đường dẫn
				string name_file = dataFromAdmin.substr(index + 1, dataFromAdmin.size() - index - 2);  // Trích xuất tên file

				if (ReceiveFileFromServer(clientSocket, name_file, email, ErrorCode)) {  // Nhận file sao chép từ server
					Sleep(1000);  // Tạm dừng trước khi xử lý phản hồi

					memset(buffer, 0, BUFFER_SIZE);
					if (recv(clientSocket, buffer, BUFFER_SIZE, 0) != SOCKET_ERROR) {  // Nhận phản hồi từ server
						bodyEmail = (string)buffer;
						cout << bodyEmail << endl;

						sendFileEmail(email, name_file, subjectEmail, bodyEmail);

						return 0;
					}
					else return 2;
				}
				else return 2;
			}
			else return 1;
		}
		else return 1;
	}

	// Xử lý yêu cầu xóa file trong máy khi nhận được yêu cầu "Delete File"
	if (strcmp(message, "Delete File") == 0) {
		if (AnalyzeMessage(dataFromAdmin, message)) {
			send(clientSocket, message, strlen(message), 0);
			memset(buffer, 0, BUFFER_SIZE);
			if (recv(clientSocket, buffer, BUFFER_SIZE, 0) != SOCKET_ERROR) {  // Nhận phản hồi từ server
				bodyEmail = (string)buffer;
				cout << bodyEmail << endl;

				sendTextEmail(email, subjectEmail, bodyEmail);

				return 0;
			}
			else return 2;
		}
		else return 1;
	}

	// Xử lý yêu cầu chụp màn hình khi nhận được yêu cầu "Capture Screen"
	if (strcmp(message, "Capture Screen") == 0) {
		if (ReceiveFileFromServer(clientSocket, "Capture_Screen.png", email, ErrorCode)) {  // Nhận file ảnh màn hình từ server
			memset(buffer, 0, BUFFER_SIZE);
			if (recv(clientSocket, buffer, BUFFER_SIZE, 0) != SOCKET_ERROR) {  // Nhận phản hồi từ server
				cout << "Capture Screen successful!";

				sendFileEmail(email, "Capture_Screen.png", subjectEmail, buffer);

				return 0;
			}
			else return 2;
		}
		else return 2;
	}

	// Xử lý yêu cầu sử dụng Web camera khi nhận được yêu cầu "Web Camera"
	if (strcmp(message, "Web Camera") == 0) {
		if (AnalyzeMessage(dataFromAdmin, message)) {
			send(clientSocket, message, strlen(message), 0);

			if (ReceiveFileFromServer(clientSocket, "Web_Camera.avi", email, ErrorCode)) {  // Nhận file ghi nhận phím từ server
				Sleep(1000);
				cout << "Use WebCam successful!" << endl;

				memset(buffer, 0, BUFFER_SIZE);
				if (recv(clientSocket, buffer, BUFFER_SIZE, 0) != SOCKET_ERROR) {
					bodyEmail = (string)buffer;
					sendFileEmail(email, "Web_Camera.avi", subjectEmail, bodyEmail);

					return 0;
				}
				else return 2;
			}
			else return 2;
		}
		else return 1;
	}

	// Xử lý yêu cầu ghi nhận các phím đã nhấn khi nhận được yêu cầu "Key Logger"
	if (strcmp(message, "Key Logger") == 0) {
		if (AnalyzeMessage(dataFromAdmin, message)) {
			send(clientSocket, message, strlen(message), 0);

			if (ReceiveFileFromServer(clientSocket, "Key_Logger.txt", email, ErrorCode)) {  // Nhận file ghi nhận phím từ server
				Sleep(1000);
				cout << "Key Logger successful!" << endl;

				memset(buffer, 0, BUFFER_SIZE);
				if (recv(clientSocket, buffer, BUFFER_SIZE, 0) != SOCKET_ERROR) {
					bodyEmail = (string)buffer;
					sendFileEmail(email, "Key_Logger.txt", subjectEmail, bodyEmail);

					return 0;
				}
				else return 2;
			}
			else return 2;
		}
		else return 1;
	}

	// Xử lý yêu cầu khóa bàn phím trong thời gian quy định khi nhận được yêu cầu "Lock Keyboard"
	if (strcmp(message, "Lock Keyboard") == 0) {
		if (AnalyzeMessage(dataFromAdmin, message)) {
			send(clientSocket, message, strlen(message), 0);

			// Nhận phản hồi đầu tiên từ server
			if (recv(clientSocket, buffer, BUFFER_SIZE, 0) != SOCKET_ERROR) {
				bodyEmail = (string)buffer;
				cout << bodyEmail << endl;

				sendTextEmail(email, subjectEmail, bodyEmail);  // Gửi phản hồi đầu tiên qua email cho admin

				memset(buffer, 0, BUFFER_SIZE);  // Xóa bộ đệm để chuẩn bị nhận dữ liệu tiếp theo

				// Nhận phản hồi bổ sung từ server
				if (recv(clientSocket, buffer, BUFFER_SIZE, 0) != SOCKET_ERROR) {
					bodyEmail = (string)buffer;
					cout << bodyEmail << endl;

					sendTextEmail(email, subjectEmail, bodyEmail);

					return 0;
				}
				else return 2;
			}
			else return 2;
		}
		else return 1;
	}

	return 1;
}

void CommunicateWithServer(SOCKET clientSocket, Email& email) {
	string subjectEmail = "[CLIENT] REPORT FROM CLIENT";
	string bodyEmail = "";
	string ErrorCode = "Error: Failed to receive data from SERVER.";

	string dataFromAdmin, info, current_UID;
	char buffer[BUFFER_SIZE] = { 0 };
	char* message = NULL;
	int result = 0;

	while (true) {
		cout << "\rWaiting for ADMIN request...";  // Đợi yêu cầu từ admin
		Sleep(3000);

		current_UID = uidLatestEmail(email.Client, email.ClientPassword);	// Lấy UID của email gần nhất
		if (current_UID > email.UID) {  // Kiểm tra nếu có email mới từ admin
			if (readEmail(email, dataFromAdmin) == true && dataFromAdmin.back() == '.' && checkRequest(dataFromAdmin, message)) {  // Đọc nội dung email từ admin
				if (send(clientSocket, message, strlen(message), 0) > 0) {// Gửi yêu cầu đến server
					cout << endl << endl << "Client: " << info.assign(message) << endl;
					cout << "Waiting for server response..." << endl;		// Đợi phản hồi từ server

					memset(buffer, 0, BUFFER_SIZE);  // Thiết lập lại buffer
					bodyEmail = "";
					result = 0;

					cout << "RESULT:" << endl;

					// Xử lý yêu cầu đóng kết nối khi nhận được yêu cầu "Close Connect"
					if (strcmp(message, "Close Connect") == 0) {
						if (recv(clientSocket, buffer, BUFFER_SIZE, 0) != SOCKET_ERROR) {
							closesocket(clientSocket);
							WSACleanup();
							cout << "Disconnected!" << endl;

							bodyEmail = (string)buffer;
							bodyEmail += " and Client!";

							sendTextEmail(email, subjectEmail, bodyEmail);

							break;
						}
						else {
							cout << ErrorCode << endl;
							sendTextEmail(email, subjectEmail, ErrorCode);
						}
					}

					// Xử lý yêu cầu tắt máy hoặc khởi động lại máy tính
					if (strcmp(message, "Shutdown") == 0 || strcmp(message, "Restart") == 0) {
						if (recv(clientSocket, buffer, BUFFER_SIZE, 0) != SOCKET_ERROR) {
							bodyEmail = (string)buffer;
							cout << bodyEmail << endl;

							sendTextEmail(email, subjectEmail, bodyEmail);

							cout << "Disconnected!" << endl;
							break;
						}
						else {
							cout << ErrorCode << endl;
							sendTextEmail(email, subjectEmail, ErrorCode);
						}
					}

					result = doTask(clientSocket, email, dataFromAdmin, message, buffer, subjectEmail, bodyEmail, ErrorCode);

					if (result == 1) {
						cout << "Error: Invalid request." << endl;
						bodyEmail = "Error: Invalid request.";
						sendTextEmail(email, subjectEmail, bodyEmail);
					}
					else if (result == 2) {
						cout << ErrorCode << endl;
						sendTextEmail(email, subjectEmail, ErrorCode);
					}
				}
				else {
					recv(clientSocket, buffer, BUFFER_SIZE, 0);
					bodyEmail = (string)buffer;
					sendTextEmail(email, subjectEmail, bodyEmail);
				}
			}
			else {
				if (dataFromAdmin == "Not Admin.") {
					cout << endl << "Their is someone trying to control your server computer through email." << endl;
					bodyEmail = "Their is someone trying to control your server computer through email.";
				}
				else {
					cout << "Error: Invalid request." << endl;
					bodyEmail = "Error: Invalid request.";
				}
			}
		}
	}

	closesocket(clientSocket);  // Đóng kết nối socket
	WSACleanup();  // Dọn dẹp các tài nguyên socket
}