# Tiểu luận Cuối kỳ: Mạng Cảm Biến

## 1. Thông tin sinh viên
- Họ và tên: Phan Nguyễn Công Trí
- Đề tài: Hệ thống phân loại cảm xúc (Emotion Classifier) sử dụng vi điều khiển (ESP32-S3 + OV3660 + Edge Impulse)
- Link model Edge Impulse: https://studio.edgeimpulse.com/studio/1019828
## 2. Cấu trúc thư mục
- `src/emotion_classifier/`: Chứa file mã nguồn chính (`emotion_classifier.ino`) viết cho nền tảng Arduino.

## 3. Yêu cầu hệ thống (Dependencies)
Phần cứng:
- Board mạch: ESP32-S3 Dev Module
- Camera: OV3660
- Màn hình hiển thị: LCD I2C

Phần mềm & Thư viện:
- Trình biên dịch: Arduino IDE (phiên bản 2.3.9)
- Thư viện AI (từ Edge Impulse): `Face_Emotion_Recognition_inferencing.h`
- Thư viện điều khiển Camera: `esp_camera.h`
- Thư viện màn hình: `LiquidCrystal_I2C.h` và `Wire.h`

## 4. Hướng dẫn cài đặt và chạy
1. Tải toàn bộ source code về máy thông qua Terminal/Git Bash:
   `git clone https://github.com/Ptir-03/mangcambien-cuoiky-PhanNguyenCongTri.git`
2. Mở file `emotion_classifier.ino` bằng phần mềm Arduino IDE.
3. Cài đặt các thư viện yêu cầu: Cài thư viện `LiquidCrystal I2C` thông qua Library Manager và thêm file thư viện `.zip` tải về từ Edge Impulse.
4. Vào Tools -> Board, chọn đúng loại board là ESP32S3 Dev Module.
5. Kết nối board với máy tính, chọn đúng cổng COM.
6. Nhấn nút Upload để nạp chương trình xuống vi điều khiển. Nhấn nút (chân GPIO 0) để bắt đầu nhận diện cảm xúc.
