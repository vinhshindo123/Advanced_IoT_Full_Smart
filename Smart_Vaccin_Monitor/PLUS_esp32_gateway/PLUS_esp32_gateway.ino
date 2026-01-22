#include <esp_now.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <vector>
#include <ArduinoJson.h>
#include <WiFiManager.h> 

// --- CẤU HÌNH ---
const char* mqtt_server = "192.168.0.103";
uint8_t nodeAddr[] = {0xB0, 0xCB, 0xD8, 0xCF, 0xE5, 0x88}; 

struct DataPoint { float temp; uint32_t ts; };
std::vector<DataPoint> buffer;

WiFiClient espClient;
PubSubClient client(espClient);

// --- CALLBACK NHẬN DỮ LIỆU TỪ NODE (ESP-NOW) ---
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    float temp;
    memcpy(&temp, data, sizeof(temp));
    
    Serial.println("------------------------------------------");
    if (WiFi.status() != WL_CONNECTED || !client.connected()) {
        buffer.push_back({temp, (uint32_t)(millis()/1000)});
        Serial.printf("📥 [ESP-NOW] Nhận: %.2f°C | 📦 [STORE] Đã lưu vào bộ đệm (Tổng: %d)\n", temp, buffer.size());
    } else {
        StaticJsonDocument<128> doc;
        doc["temp"] = temp;
        doc["ts"] = millis()/1000;
        char msg[128];
        serializeJson(doc, msg);
        client.publish("factory/sensor/01/temp", msg);
        Serial.printf("📥 [ESP-NOW] Nhận: %.2f°C | 📤 [LIVE] Đã gửi lên Server\n", temp);
    }
}

// --- CALLBACK NHẬN LỆNH TỪ WEB (MQTT) ---
void callback(char* topic, byte* payload, unsigned int length) {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, payload, length);
    
    struct { char action[10]; int value; } cmd;
    strcpy(cmd.action, doc["action"] | "NONE");
    cmd.value = doc["value"] | 0;

    Serial.println("\n==========================================");
    Serial.printf("📩 [MQTT RECV] Lệnh: %s | Giá trị: %d\n", cmd.action, cmd.value);

    esp_err_t result = esp_now_send(nodeAddr, (uint8_t *)&cmd, sizeof(cmd));
    
    if (result == ESP_OK) {
        Serial.println("🚀 [DOWNLINK] Đã chuyển lệnh xuống Node qua ESP-NOW");
        client.publish("factory/control/01/stat", payload, length); 
        Serial.println("🔄 [FEEDBACK] Đã báo cáo trạng thái STAT về Dashboard");
    } else {
        Serial.println("❌ [DOWNLINK] Chuyển lệnh THẤT BẠI");
    }
    Serial.println("==========================================\n");
}

// --- HÀM KẾT NỐI LẠI MQTT ---
void reconnectMQTT() {
    if (!client.connected()) {
        Serial.print("🔌 [MQTT] Đang thử kết nối Server...");
        if (client.connect("Gateway_Vaccine_Main")) {
            Serial.println(" ĐÃ KẾT NỐI!");
            client.subscribe("factory/control/NODE_01/cmd");
        } else {
            Serial.printf(" THẤT BẠI (rc=%d). Thử lại sau 5s\n", client.state());
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n🚀 [SYSTEM] HỆ THỐNG GATEWAY ĐANG KHỞI ĐỘNG...");

    // Bước 1: Dọn dẹp WiFi để tránh treo do bộ nhớ cũ
    WiFi.mode(WIFI_STA); 
    WiFi.disconnect();
    delay(1000);

    // Bước 2: WiFiManager - Kết nối hoặc phát AP
    WiFiManager wm;
    wm.setConnectTimeout(30); // Chờ kết nối cũ 30s, nếu không được mới phát AP
    
    Serial.println("📶 [WIFI] Đang tìm kiếm mạng đã lưu...");
    if(!wm.autoConnect("Gateway_Config_AP")) {
        Serial.println("❌ [WIFI] Kết nối thất bại! Đang reset lại chip...");
        delay(3000);
        ESP.restart();
    }

    Serial.println("✅ [WIFI] Đã kết nối Internet thành công!");
    Serial.print("📍 [WIFI] IP Address: "); Serial.println(WiFi.localIP());
    Serial.print("📡 [WIFI] Channel hiện tại: "); Serial.println(WiFi.channel());

    // Bước 3: Cài đặt MQTT
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    // Bước 4: Khởi tạo ESP-NOW (Chạy song song WiFi)
    if (esp_now_init() != ESP_OK) {
        Serial.println("❌ [ESP-NOW] Khởi tạo thất bại");
        return;
    }
    esp_now_register_recv_cb(OnDataRecv);
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, nodeAddr, 6);
    peerInfo.channel = 0; // Tự động khớp với Channel của Router WiFi
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("❌ [ESP-NOW] Thêm Node thất bại (Kiểm tra MAC)");
    } else {
        Serial.println("✅ [ESP-NOW] Đã sẵn sàng giao tiếp với Node");
    }
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        // Luôn duy trì MQTT
        if (!client.connected()) {
            reconnectMQTT();
        }
        client.loop();
        
        // Xả bộ đệm Store & Forward khi có mạng lại
        if (!buffer.empty() && client.connected()) {
            Serial.printf("\n🔄 [SYNC] Phát hiện mạng! Đang xả %d mẫu dữ liệu cũ...\n", buffer.size());
            for (int i = 0; i < buffer.size(); i++) {
                StaticJsonDocument<128> doc;
                doc["temp"] = buffer[i].temp;
                doc["ts"] = buffer[i].ts;
                doc["batch"] = true;
                char msg[128];
                serializeJson(doc, msg);
                
                if (client.publish("factory/sensor/01/temp", msg)) {
                    Serial.printf("  >> [%d/%d] Đã đẩy mẫu %.2f°C\n", i+1, buffer.size(), buffer[i].temp);
                }
                delay(100); // Tránh tràn hàng đợi MQTT
            }
            buffer.clear();
            Serial.println("✅ [SYNC] Đã đồng bộ hoàn tất dữ liệu.\n");
        }
    } else {
        Serial.println("⚠️ [SYSTEM] Mất kết nối WiFi... Đang vận hành Offline");
        delay(2000);
    }
    delay(500); 
}