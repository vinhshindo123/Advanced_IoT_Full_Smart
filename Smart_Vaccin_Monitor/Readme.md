# ❄️ SENTI-Vaccine: Hệ Thống Giám Sát Bảo Quản Vaccine Thông Minh 🚀

**SENTI-Vaccine** là giải pháp IoT chuyên dụng để giám sát nhiệt độ môi trường bảo quản dược phẩm và vaccine. Hệ thống được thiết kế để vận hành bền bỉ ngay cả khi mất kết nối Internet nhờ cơ chế **Store & Forward** (Lưu trữ và Gửi bù) cùng thuật toán lọc nhiễu tín hiệu chính xác. 🛡️

---

## 🛰️ 1. Kiến Trúc Hệ Thống (System Architecture)

Hệ thống được xây dựng trên nền tảng **Edge Computing** với sự phối hợp chặt chẽ của các thành phần cốt lõi:

* **📡 Sensor Node (ESP32):** * Thu thập dữ liệu nhiệt độ và xử lý lọc nhiễu **Median** kết hợp **EMA Filter** để đạt độ chính xác cao nhất. 📈
    * Tự động kích hoạt còi báo động tại chỗ khi nhiệt độ vượt ngưỡng an toàn (>8.0°C). 🚨
    * Giao tiếp hai chiều qua giao thức **ESP-NOW** cực nhanh. ⚡
* **🔌 Smart Gateway (ESP32):** * Vận hành ở chế độ **Hybrid**: Chạy song song ESP-NOW và WiFi. 🌐
    * **Store & Forward:** Tự động lưu dữ liệu vào bộ nhớ đệm (`std::vector`) khi mất mạng và tự động gửi bù ngay khi có kết nối lại. 📦
* **🛠️ MQTT Worker (Python):** * Cầu nối xử lý dữ liệu giữa Broker và Database Supabase. 🌉
    * Phân loại dữ liệu thành **Live** (Trực tiếp) và **Batch** (Gửi bù). 🏷️
* **💻 Cloud & Dashboard:** * **Backend:** Flask API điều khiển thiết bị thời gian thực. ⚙️
    * **Database:** Supabase lưu trữ toàn bộ lịch sử và trạng thái thiết bị. 🗄️



---

## 🔄 2. Luồng Dữ Liệu & Giao Thức (Data Flow)

| Luồng truyền tải | Giao thức | Tính năng đặc biệt |
| :--- | :--- | :--- |
| **Node ➔ Gateway** | **ESP-NOW** | 🚀 Không phụ thuộc WiFi, phản hồi < 100ms |
| **Gateway ➔ Broker** | **MQTT** | 📦 Gửi kèm cờ `batch: true` khi thực hiện gửi bù dữ liệu cũ |
| **Broker ➔ Worker** | **MQTT** | 🔄 Xử lý phản hồi trạng thái (Feedback Loop) để đồng bộ hóa DB |
| **Web ➔ Node** | **HTTP/MQTT** | 🕹️ Điều khiển cưỡng chế (Tắt còi/Bật đèn) từ xa qua Internet |

---

## 🧠 3. Logic Vận Hành Thông Minh (Smart Logic)

* **🛡️ Bảo vệ quá nhiệt:** Khi nhiệt độ vượt ngưỡng an toàn, Node tự động kích hoạt còi báo động ngay lập tức mà không cần chờ lệnh từ Server.
* **🧹 Lọc nhiễu cảm biến:**
    * **Median Filter:** Lấy 10 mẫu liên tục để loại bỏ các giá trị nhiễu đột ngột (spike).
    * **EMA Filter:** Làm mượt dữ liệu giúp đường đồ thị ổn định và chính xác hơn.
* **📦 Cơ chế gửi bù:** Gateway tự động gán nhãn thời gian (Timestamp) cho dữ liệu khi Offline, đảm bảo lịch sử bảo quản vaccine luôn đầy đủ và minh bạch.

---

## 📂 4. Cấu Trúc Thư Mục (Folder Structure)

```text
SENTI-Vaccine/
├── 📂 firmware/
│   ├── 📝 PLUS_esp32_node.ino      # Cảm biến & Điều khiển tại chỗ
│   └── 📝 PLUS_esp32_gateway.ino   # Điều phối & Lưu trữ đệm
├── 📂 server/
│   ├── 🐍 app.py                   # Flask Web Server & API
│   └── 🐍 mqtt_worker.py           # Logic xử lý dữ liệu & Supabase
├── 📂 web/
│   └── 📄 PLUS_index.html          # Giao diện giám sát tập trung
└── 📝 README.md                    # Tài liệu dự án
```

---

### 📸 5. Minh Họa & Case Test (Tiếp tục)

#### 🧪 Các kịch bản thử nghiệm (Test Cases)
* **📡 Mất kết nối WiFi:** Gateway tự động chuyển sang chế độ "Vận hành Offline" và lưu trữ dữ liệu vào bộ đệm RAM. Ngay khi WiFi hoạt động trở lại, hệ thống thực hiện xả bộ đệm để đồng bộ dữ liệu cũ lên Server với nhãn `BATCH_DATA`. 🔄
* **🔥 Cảnh báo quá nhiệt:** Khi cảm biến ghi nhận nhiệt độ > 8.0°C, Node sẽ tự động kích hoạt còi báo động (Buzzer) ngay lập tức để bảo vệ lô Vaccine. 📢
* **🕹️ Điều khiển từ xa:** Người dùng thực hiện lệnh tắt còi (OFF), khôi phục (RESET) hoặc điều khiển đèn (RELAY) trực tiếp từ Dashboard. Lệnh được truyền từ Web ➔ MQTT ➔ Gateway ➔ ESP-NOW ➔ Node với độ trễ cực thấp. ⚡

### 🖼️ Hình ảnh thực tế
| Cụm Gateway (Hybrid Mode) | Cụm Node (Sensor & Alarm) | Dashboard Giám Sát |
| :---: | :---: | :---: |
| ![Gateway](https://via.placeholder.com/300x200?text=Gateway+Photo) | ![Node](https://via.placeholder.com/300x200?text=Node+Photo) | <img width="1435" height="923" alt="image" src="https://github.com/user-attachments/assets/bdb93a95-5a33-485b-9a32-4f05ca2b4f77" /> |



### 🎥 Video Demo Vận Hành
[![SENTI-Vaccine Demo](https://img.shields.io/badge/Google_Drive-Video_Demo-blue?style=for-the-badge&logo=googledrive)](https://drive.google.com/file/d/1PZDTqWoFR0ZFvW6fZ1-QE2DLIUibbFjk/view?usp=sharing)

*Nhấn vào biểu tượng để xem video thực tế về cơ chế gửi bù dữ liệu và phản hồi điều khiển.* 🎬

---

## 👨‍💻 6. Triển Khai Nhanh (Quick Start)

### 1️⃣ Cấu hình Database (Supabase)
* Tạo bảng `measurements`: Lưu trữ giá trị nhiệt độ, ID node và nhãn (`Live`/`Batch`).
* Tạo bảng `devices`: Lưu trữ trạng thái hoạt động của thiết bị (`ALARM_OFF`, `ALARM_READY`, `LED_ON`, v.v.).

### 2️⃣ Cài đặt Python Environment
```bash
pip install flask paho-mqtt supabase python-dotenv
```
### 3️⃣ Nạp Firmware (Arduino IDE) 💻

* **Đối với Node:** Mở file `PLUS_esp32_node.ino`. 
    * Tìm dòng `uint8_t gatewayAddr[]` và thay bằng địa chỉ MAC của ESP32 Gateway của bạn.
    * Đảm bảo các chân `ALARM_PIN` (Còi) và `SENSOR_PIN` (Cảm biến) đã kết nối đúng sơ đồ.
* **Đối với Gateway:** Mở file `PLUS_esp32_gateway.ino`.
    * Tìm dòng `uint8_t nodeAddr[]` và thay bằng địa chỉ MAC của ESP32 Node.
    * Cập nhật SSID và Password WiFi để Gateway có thể kết nối Internet. 📶
    * Cập nhật `mqtt_server` thành địa chỉ IP máy tính đang chạy Broker (ví dụ: `192.168.1.100`).

### 4️⃣ Khởi chạy hệ thống 🚀

Mở 2 cửa sổ Terminal và chạy các lệnh sau:

```bash
# Terminal 1: Chạy Bridge kết nối MQTT và Database Supabase
python PLUS_mqtt_worker.py

# Terminal 2: Khởi chạy Giao diện Web Dashboard
python PLUS_app.py
```

---

---

## 🗺️ 7. Lộ trình phát triển (Roadmap) 🚀

Dự án đang tiếp tục được nâng cấp với các mục tiêu sau:

- [x] **Giai đoạn 1:** Hoàn thiện bộ lọc nhiễu thông minh (Median + EMA). ✅
- [x] **Giai đoạn 2:** Triển khai cơ chế **Store & Forward** xử lý mất mạng. ✅
- [ ] **Giai đoạn 3:** Tích hợp chế độ **Deep Sleep** trên Node để sử dụng pin lên đến 6 tháng. 🔋
- [ ] **Giai đoạn 4:** Xây dựng hệ thống cảnh báo qua **Telegram Bot** và **Zalo** khi có sự cố. 📱
- [ ] **Giai đoạn 5:** Phân tích dữ liệu bằng **AI/ML** để dự đoán sớm hỏng hóc tủ lạnh dựa trên biến động nhiệt độ. 🤖

---

## 🤝 8. Đóng góp & Phát triển (Contributing) 🛠️

Mọi ý tưởng đóng góp hoặc báo lỗi xin vui lòng thực hiện qua các bước:

1. **Fork** dự án này về tài khoản cá nhân.
2. Tạo một **Branch** mới cho tính năng của bạn (`git checkout -b feature/AmazingFeature`).
3. **Commit** các thay đổi (`git commit -m 'Add some AmazingFeature'`).
4. **Push** lên nhánh đã tạo (`git push origin feature/AmazingFeature`).
5. Mở một **Pull Request** để chúng ta cùng thảo luận.

---

## 📄 9. Giấy phép (License) 📜

Dự án này được phân phối dưới giấy phép **MIT**. Bạn có quyền tự do sử dụng, chỉnh sửa và phân phối lại cho các mục đích nghiên cứu và thương mại.

---

## 📞 10. Liên hệ & Hỗ trợ (Support) ✉️

Nếu bạn gặp khó khăn trong quá trình cài đặt hoặc cấu hình địa chỉ MAC/IP:

* **Tác giả:** [Tên của bạn]
* **Email:** [Email của bạn]
* **Cộng đồng:** Tham gia các nhóm thảo luận về **SENTI-IoT** tại Việt Nam.

---
**💡 SENTI-IoT Solutions** - *An tâm bảo quản, vẹn toàn giá trị.* 🌟
