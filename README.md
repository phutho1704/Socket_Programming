# Điều Khiển Máy Tính Windows Qua Email Và Socket

## Tổng Quan

Repository này chứa một project client/server viết bằng C++ cho Windows, kết hợp:

- Trao đổi lệnh qua email
- Giao tiếp client/server qua socket
- Các Windows API để thực hiện thao tác trên máy cục bộ

Nhìn theo cấu trúc mã nguồn, đây là một project học thuật hoặc đồ án xoay quanh mô hình điều khiển máy tính từ xa trên Windows.

## Cấu Trúc Thư Mục

```text
Project
├─ source/
│  ├─ socket_client/
│  │  ├─ main.cpp
│  │  ├─ Socket.cpp
│  │  ├─ Gmail.cpp
│  │  └─ *.vcxproj
│  ├─ socket_server/
│  │  ├─ main.cpp
│  │  ├─ Socket.cpp
│  │  ├─ Task.cpp
│  │  └─ *.vcxproj
│  └─ readme.txt
```

## Thành Phần Chính

### `socket_client`

Chịu trách nhiệm:

- đọc lệnh từ email
- kết nối tới server qua TCP
- chuyển tiếp yêu cầu sang server
- nhận kết quả hoặc file trả về từ server
- gửi báo cáo và file đính kèm lại qua email

### `socket_server`

Chịu trách nhiệm:

- chấp nhận kết nối từ client
- thực thi các tác vụ phía Windows
- trả về kết quả dạng text hoặc file

## Công Nghệ Sử Dụng

Nhóm công nghệ cốt lõi:

- `C++`
- `WinSock`
- `TCP Socket`
- `IMAP`
- `SMTP`

Nhóm công nghệ và thư viện bổ sung:

- `SSL/TLS`
- `libcurl`
- `OpenCV`
- `Win32 API`
- `Visual Studio 2022`
- `vcpkg`

## Các Thành Phần Quan Trọng Theo Hướng Security

Code sử dụng nhiều khả năng nhạy cảm về mặt hệ thống và bảo mật:

- `SSL/TLS` để bảo vệ kết nối email
- `libcurl` để thao tác gửi/đọc mail qua Gmail
- `Win32 Security/Privilege APIs` cho các thao tác mức hệ thống
- low-level keyboard hooks
- screen capture and webcam access
- file and process control on Windows

## Môi Trường Build

Các file project hiện tại cho thấy chương trình được build với:

- Chỉ hỗ trợ Windows
- Visual Studio 2022
- Windows SDK 10
- MSVC toolset `v143`
- `libcurl` được link trong client
- `OpenCV` được link trong server

## Tóm Tắt

Ở mức tổng quan, đây là một project điều khiển máy tính Windows bằng C++ sử dụng email làm kênh nhận lệnh, socket làm kênh truyền dữ liệu, `libcurl` để xử lý mail, `SSL/TLS` để bảo vệ kết nối, `OpenCV` để làm việc với webcam, và `Win32 API` để thực hiện các tác vụ mức hệ điều hành.
