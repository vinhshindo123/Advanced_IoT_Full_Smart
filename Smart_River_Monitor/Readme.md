# 🌊 SENTI-Flood: Hệ Thống Giám Sát & Điều Khiển Xả Lũ Tự Động 🚀

**SENTI-Flood** là một giải pháp IoT toàn diện, được thiết kế để giám sát mực nước theo thời gian thực và tự động điều tiết cửa xả lũ. Hệ thống sử dụng các thuật toán lọc nhiễu tiên tiến và cơ chế điều khiển phân cấp để đảm bảo an toàn tuyệt đối cho vùng hạ du.

---

## 🛰️ 1. Kiến Trúc Hệ Thống (System Architecture)

Dự án được triển khai theo mô hình **Edge-to-Cloud** với sự kết hợp của 3 thành phần chính:

* **📡 Sensor Node (ESP32):**
    * Đo mực nước bằng cảm biến siêu âm.
    * Sử dụng bộ lọc kép: **Median Filter** (Trung vị) + **EMA Filter** (Làm mượt).
    * Truyền dữ liệu qua **ESP-NOW** và sử dụng **Deep Sleep** để tối ưu năng lượng.
* **🎮 IoT Gateway (ESP32):**
    * Trung tâm điều phối: Nhận tín hiệu từ Node, điều khiển **Servo** và **Buzzer**.
    * Giao tiếp đa phương thức: **HTTP POST** gửi dữ liệu cảm biến, **MQTT** quản lý trạng thái và nhận lệnh điều khiển.
* **💻 Cloud & Dashboard:**
    * **Backend:** Flask API + MQTT Worker.
    * **Database:** Supabase (PostgreSQL).
    * **Frontend:** Dashboard hiển thị biểu đồ thời gian thực.



---

## 🔄 2. Luồng Dữ Liệu & Giao Thức (Data Flow)

| Thành phần | Giao thức | Chức năng |
| :--- | :--- | :--- |
| **Node ➔ Gateway** | **ESP-NOW** | Gửi dữ liệu mực nước (Tiết kiệm pin, tốc độ cao) |
| **Gateway ➔ Server** | **HTTP POST** | Đẩy dữ liệu Sensor lên database (Lưu lịch sử/Vẽ biểu đồ) |
| **Gateway ➔ Broker** | **MQTT (Pub)** | Cập nhật trạng thái Cửa đập & Chế độ vận hành |
| **Web ➔ Gateway** | **MQTT (Sub)** | Nhận lệnh **LOCK/UNLOCK** từ người quản lý |

---

## 🧠 3. Logic Vận Hành (FSM & Cases)

Hệ thống vận hành dựa trên máy trạng thái hữu hạn (**Finite State Machine**) với các ngưỡng an toàn như sau:

* **✅ Mức An Toàn (>= 6.0cm):** Cửa đập đóng hoàn toàn (0°).
* **⚠️ Mức Cảnh Báo (4.5cm):** Hiển thị trạng thái cảnh báo trên LCD/Web.
* **🚨 Mức Xả Cấp 1 (<= 3.5cm):** Hú còi báo động 4s ➔ Mở cửa **20%** (18°).
* **🆘 Mức Xả Cấp 2 (<= 2.5cm):** Mở cửa **50%** (45°).
* **🔒 Khóa Khẩn Cấp (LOCK):** Ưu tiên cao nhất từ Web ➔ Đóng chặt cửa đập bất kể mực nước.



---

## 🛠️ 4. Giải Quyết Vấn Đề (Problem Solving)

* **Nhiễu dữ liệu:** Sử dụng bộ lọc **Median** để loại bỏ nhiễu trắng và **EMA** để làm mượt đường đồ thị, tránh việc Servo bị rung (jitter).
* **Giám sát kết nối:** Gateway liên tục kiểm tra tín hiệu từ Node. Nếu mất kết nối (>10s), đèn LED sẽ nháy cảnh báo và Dashboard hiển thị **OFFLINE**.
* **An toàn xả lũ:** Luôn có chu kỳ hú còi trước khi Servo chuyển động để cảnh báo người dân vùng lân cận.

---

## 📂 5. Cấu Trúc Thư Mục (Folder Structure)

```text
SENTI-Flood/
├── 📂 firmware/
│   ├── 📝 sensor_node.ino      # Code Node cảm biến (ESP-NOW)
│   └── 📝 gateway.ino          # Code Gateway (MQTT, HTTP, Servo)
├── 📂 server/
│   ├── 🐍 app.py               # Flask Web Server
│   └── 🐍 mqtt_worker.py       # MQTT to Database Bridge
├── 📂 web/
│   └── 📄 index.html           # Real-time Dashboard UI
└── 📝 README.md                # Tài liệu dự án
```
---

## 📸 6. Hình Ảnh & Video Minh Họa (Illustrations)

### 🖼️ Hình ảnh thực tế hệ thống
> *Mẹo: Bạn nên chụp ảnh mạch Gateway có kèm màn hình LCD và ảnh Node cảm biến đặt tại vị trí đo.*

| Cụm Node điều khiển | Cụm Gateway cảm biến | Giao diện Dashboard |
| :---: | :---: | :---: |
|(<img width="1069" height="815" alt="image" src="https://github.com/user-attachments/assets/ede2dd4d-071f-4c43-8ee0-4948de937564" />| (<img width="1091" height="821" alt="image" src="https://github.com/user-attachments/assets/855bc08a-8834-4ddc-81d4-b109b6d3455e" />| (<img width="1909" height="915" alt="image" src="https://github.com/user-attachments/assets/3e6febb9-16a3-4a1a-9656-546b7e421294" />|

### 🎥 Video Demo vận hành


* **Case 1:** Nước dâng ➔ Hú còi ➔ Cửa mở 20% (Ngưỡng 3.5cm).
* **Case 2:** Nước dâng cao ➔ Cửa mở 50% (Ngưỡng 2.5cm).
* **Case 3:** Nhấn nút **LOCK** trên Web ➔ Cửa đóng lập tức (Cưỡng chế).
### 🎥 Video Demo Vận Hành Hệ Thống
[![SENTI-Flood Demo Video](https://img.shields.io/badge/YouTube-Video_Demo-red?style=for-the-badge&logo=youtube)](https://drive.google.com/file/d/1PZDTqWoFR0ZFvW6fZ1-QE2DLIUibbFjk/view?usp=sharing)

*Nhấn vào nút đỏ ở trên để xem video demo vận hành thực tế.*
---

## 👨‍💻 7. Hướng dẫn cài đặt (Installation)

### Bước 1: Chuẩn bị Database
* Tạo tài khoản [Supabase](https://supabase.com/).
* Tạo bảng `measurements` (id, node_id, sensor_value, created_at).
* Tạo bảng `devices` (device_id, status, last_value, updated_at).

### Bước 2: Nạp Firmware
1. Sử dụng Arduino IDE để nạp code cho **Node**.
2. Sử dụng Arduino IDE để nạp code cho **Gateway** (Lưu ý sửa biến `server_url` thành IP máy tính của bạn).

### Bước 3: Khởi động Server
```bash
# Cài đặt thư viện cần thiết
pip install flask paho-mqtt supabase

# Chạy Backend và Bridge dữ liệu
python app.py
python mqtt_worker.py
