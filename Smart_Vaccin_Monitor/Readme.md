# ❄️ SENTI-Vaccine: Hệ Thống Giám Sát Bảo Quản Vaccine Thông Minh 🚀

**SENTI-Vaccine** là giải pháp IoT chuyên dụng để giám sát nhiệt độ môi trường bảo quản dược phẩm và vaccine. Hệ thống được thiết kế để vận hành bền bỉ ngay cả khi mất kết nối Internet nhờ cơ chế **Store & Forward** (Lưu trữ và Gửi bù) cùng thuật toán lọc nhiễu tín hiệu chính xác.

---

## 🛰️ 1. Kiến Trúc Hệ Thống (System Architecture)

Hệ thống được xây dựng trên nền tảng **Edge Computing** với sự phối hợp chặt chẽ của các thành phần:

* [cite_start]**📡 Sensor Node (ESP32):** * Thu thập dữ liệu nhiệt độ và xử lý lọc nhiễu **Median** kết hợp **EMA Filter**[cite: 38, 39, 41]. 
    * [cite_start]Tự động kích hoạt còi báo động tại chỗ khi nhiệt độ vượt ngưỡng an toàn (>8°C)[cite: 54, 55]. 
    * [cite_start]Giao tiếp hai chiều qua giao thức **ESP-NOW**[cite: 52].
* [cite_start]**🔌 Smart Gateway (ESP32):** * Vận hành ở chế độ Hybrid: Chạy song song ESP-NOW và WiFi[cite: 18, 24]. 
    * [cite_start]**Store & Forward:** Tự động lưu dữ liệu vào bộ nhớ đệm (`std::vector`) khi mất mạng và gửi bù ngay khi có kết nối lại[cite: 4, 30].
* [cite_start]**🛠️ MQTT Worker (Python):** * Cầu nối xử lý dữ liệu giữa Broker và Database Supabase. 
    * [cite_start]Phân loại dữ liệu "Trực tiếp" (Live) và "Gửi bù" (Batch).
* **💻 Cloud & Dashboard:** * **Backend:** Flask API điều khiển thiết bị thời gian thực. 
    * [cite_start]**Database:** Supabase lưu trữ lịch sử và trạng thái thiết bị.



---

## 🔄 2. Luồng Dữ Liệu & Giao Thức (Data Flow)

| Luồng truyền tải | Giao thức | Tính năng đặc biệt |
| :--- | :--- | :--- |
| **Node ➔ Gateway** | **ESP-NOW** | [cite_start]Không phụ thuộc WiFi, phản hồi cực nhanh [cite: 24, 52] |
| **Gateway ➔ Broker** | **MQTT** | [cite_start]Gửi dữ liệu kèm cờ `batch: true` khi thực hiện gửi bù [cite: 32] |
| **Broker ➔ Worker** | **MQTT** | [cite_start]Xử lý phản hồi trạng thái (Feedback Loop) để đồng bộ DB  |
| **Web ➔ Node** | **HTTP/MQTT** | Điều khiển cưỡng chế (Tắt còi/Bật đèn) từ xa |

---

## 🧠 3. Logic Vận Hành Thông Minh (Smart Logic)

* [cite_start]**🛡️ Bảo vệ quá nhiệt:** Khi nhiệt độ > 8.0°C, Node tự động bật còi báo động trừ khi có lệnh tắt cưỡng chế từ Web[cite: 55].
* **🧹 Lọc nhiễu cảm biến:**
    * [cite_start]**Median Filter:** Lấy 10 mẫu liên tục để loại bỏ nhiễu nhảy số đột ngột[cite: 41, 44].
    * [cite_start]**EMA Filter:** Làm mượt dữ liệu giúp đồ thị nhiệt độ ổn định[cite: 39, 54].
* [cite_start]**📦 Cơ chế gửi bù:** Gateway sử dụng bộ đệm lưu trữ dữ liệu kèm mốc thời gian (Timestamp) khi mất WiFi[cite: 4, 30, 32].

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
* [cite_start]**Mất kết nối WiFi:** Gateway tự động chuyển sang chế độ "Vận hành Offline" và lưu trữ dữ liệu vào bộ đệm RAM[cite: 36, 37]. [cite_start]Ngay khi WiFi hoạt động trở lại, hệ thống thực hiện xả bộ đệm để đồng bộ dữ liệu cũ lên Server.
* [cite_start]**Cảnh báo quá nhiệt:** Khi cảm biến ghi nhận nhiệt độ > 8.0°C, Node sẽ tự động kích hoạt còi báo động (Buzzer)[cite: 55, 56].
* [cite_start]**Điều khiển từ xa:** Người dùng có thể thực hiện lệnh tắt còi (OFF), khôi phục (RESET) hoặc điều khiển đèn (RELAY) trực tiếp từ Dashboard Web[cite: 47, 48, 49, 50].

### 🖼️ Hình ảnh thực tế
| Cụm Gateway (Hybrid Mode) | Cụm Node (Sensor & Alarm) | Dashboard Giám Sát |
| :---: | :---: | :---: |
| ![Gateway](https://via.placeholder.com/300x200?text=Gateway+Photo) | ![Node](https://via.placeholder.com/300x200?text=Node+Photo) | <img width="1435" height="923" alt="image" src="https://github.com/user-attachments/assets/bdb93a95-5a33-485b-9a32-4f05ca2b4f77" />
 |

### 🎥 Video Demo Vận Hành

[![SENTI-Vaccine Demo](https://img.shields.io/badge/Google_Drive-Video_Demo-blue?style=for-the-badge&logo=googledrive)](https://drive.google.com/file/d/1PZDTqWoFR0ZFvW6fZ1-QE2DLIUibbFjk/view?usp=sharing)

*Nhấn vào biểu tượng để xem video thực tế về cơ chế gửi bù dữ liệu và phản hồi điều khiển.*

---

## 👨‍💻 6. Triển Khai Nhanh (Quick Start)

### 1. Cấu hình Database (Supabase)
* Tạo bảng `measurements`: Lưu trữ giá trị nhiệt độ, ID node và nhãn (Live/Batch).
* Tạo bảng `devices`: Lưu trữ trạng thái hoạt động của thiết bị (ALARM_OFF, ALARM_READY, v.v.).

### 2. Cài đặt Python Environment
```bash
pip install flask paho-mqtt supabase python-dotenv
