READ ME ĐỀ TÀI: "ĐIỀU KHIỂN MÁY TÍNH TỪ XA BẰNG EMAIL VÀ LẬP TRÌNH SOCKET"

1_NHỮNG LƯU Ý TRƯỚC KHI CHẠY MÃ NGUỒN
1.1_MÔI TRƯỜNG HỖ TRỢ

	- Chương trình chỉ hỗ trợ chạy trên hệ điều hành Windows.

	- Chương trình chạy được trên Visual Studio 2022.

	- Chương trình được code bằng ngôn ngữ C++.

1.2_CHƯƠNG TRÌNH CLIENT

	- Cần cài đặt thư viện libcurl và cấu hình vào chương trình. [1]

1.3_CHƯƠNG TRÌNH SERVER

	- Cần cài đặt thư viện opencv và cấu hình vào chương trình. [2]

2_CÁC BƯỚC CHẠY MÃ NGUỒN
	
	- Giải nén hai chương trình Client và Server.
	
	- Cấu hình thư viện libcurl vào chương trình Client và thư viện opencv vào chương trình Server.

	- Xây dựng (build) chương trình Client và Server.

	- Chạy mã nguồn.

Lưu ý: cần thực hiện các bước theo trình tự từ trên xuống để tránh gây ra lỗi không mong muốn
/////////////////////////////////////////////////////////////////////////////////////////////
Chú ý: Nhóm cài đặt thư viện libcurl và thư viện opencv bằng vcpkg thông qua Command Prompt (CMD), do đó, cần phải cài đặt các công cụ hỗ trợ trước (git [3] -> cvpkg [4]).

[3] Các bước cài đặt git bằng PowerShell.

	- Mở PowerShell với quyền quản trị (Admintrator).

	- Tải Git bằng cách sử dụng lệnh sau:
	  	Invoke-WebRequest -Uri "https://github.com/git-for-windows/git/releases/download/v2.42.0.windows.1/Git-2.42.0-64-bit.exe" -OutFile "git-installer.exe"

	- Chạy tệp cài đặt:
		Start-Process ".\git-installer.exe"

	- Hoàn tất cài đặt và kiểm tra lại bằng lệnh:
		git --version

[4] Các bước cài đặt cvpkg từ Github bằng PowerShell.

	- Mở PowerShell với quyền quản trị (Admintrator).

	- Chạy lệnh sau để clone repository:
		git clone https://github.com/cvpkg/cvpkg.git

	- Chuyển vào thư mục vcpkg và chạy lệnh sau để cài đặt vcpkg:
		cd vcpkg
		.\bootstrap-vcpkg.bat


[1] Các bước cài đặt thư viện libcurl và cấu hình vào chương trình:

	-  Cài đặt git (nếu chưa cài).

	- Cài đặt cvpkg (nếu chưa cài).

	- Chạy lệnh sau trong thư mục vcpkg để cài đặt thư viện libcurl:
		.\vcpkg install curl

	- Áp dụng cho Visual Studio - cấu hình tự động bằng lệnh sau:
		.\vcpkg integrate install
	

[2] Các bước cài đặt thư viện libcurl và cấu hình vào chương trình:

	-  Cài đặt git (nếu chưa cài).

	- Cài đặt cvpkg (nếu chưa cài).

	- Chạy lệnh sau trong thư mục vcpkg để cài đặt thư viện opencv:
		.\vcpkg install opencv

	- Áp dụng cho Visual Studio - cấu hình tự động bằng lệnh sau:
		.\vcpkg integrate install