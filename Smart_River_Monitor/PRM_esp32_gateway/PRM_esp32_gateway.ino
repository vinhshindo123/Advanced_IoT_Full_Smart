#include <esp_now.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// --- CẤU HÌNH PHẦN CỨNG ---
#define SERVO_PIN 32
#define BUZZER_PIN 4
#define LED_CONN 5

// --- CẤU HÌNH MQTT/HTTP ---
const char* mqtt_server = "192.168.0.103";
const int mqtt_port = 1883;
const char* topic_stat = "esp32/flood/stat"; 
const char* topic_ctrl = "esp32/flood/control";
const char* server_url = "http://192.168.0.103:5000/api/sensor";

WiFiClient espClient;
PubSubClient client(espClient);

// Biến trạng thái
enum State { SAFE, DISCHARGING, LOCKED_BY_USER };
State currentState = SAFE;

Servo gate;
LiquidCrystal_I2C lcd(0x27, 16, 2);
float currentLevel = 0;
unsigned long lastRecvTime = 0;
unsigned long stateTimer = 0;
unsigned long lastLogTime = 0;
unsigned long lastDataSend = 0;
int dischargeStep = 0;
bool isNodeOnline = false;

typedef struct struct_message {
    float level;
    int battery;
} struct_message;
struct_message incoming;

void sendSensorDataHTTP() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(server_url);
        http.addHeader("Content-Type", "application/json");

        StaticJsonDocument<128> doc;
        doc["level"] = currentLevel;
        doc["node"] = isNodeOnline ? "ON" : "OFF";
        
        String jsonStr;
        serializeJson(doc, jsonStr);

        Serial.print("[HTTP] Đang gửi data cảm biến: "); Serial.println(jsonStr);
        int httpResponseCode = http.POST(jsonStr);

        if (httpResponseCode > 0) {
            Serial.printf("[HTTP] Thành công, Code: %d\n", httpResponseCode);
        } else {
            Serial.printf("[HTTP] THẤT BẠI, Lỗi: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    } else {
        Serial.println("[HTTP] WiFi Disconnected - Không thể gửi HTTP");
    }
}

// --- HÀM GỬI DỮ LIỆU LÊN MQTT (CHO SUPABASE) ---
void sendStatusMQTT() {
    if (client.connected()) {
        StaticJsonDocument<128> doc;
        doc["gate"] = (currentState == DISCHARGING && dischargeStep == 1) ? "OPEN" : "CLOSE";
        doc["mode"] = (currentState == LOCKED_BY_USER) ? "LOCKED" : "AUTO";
        
        char buffer[128];
        serializeJson(doc, buffer);
        
        Serial.print("[MQTT] Đang gửi trạng thái hệ thống...");
        if (client.publish(topic_stat, buffer)) {
            Serial.println("OK");
        } else {
            Serial.println("FAILED");
        }
    }
}

// --- HÀM GỬI XÁC NHẬN LỆNH (FEEDBACK) ---
void sendStatusFeedback(String action) {
    StaticJsonDocument<100> doc;
    doc["device_id"] = "NODE_01";
    doc["action"] = action;
    
    char buffer[256];
    serializeJson(doc, buffer);
    client.publish(topic_stat, buffer);
}

// --- CALLBACK NHẬN LỆNH TỪ WEB ---
void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) message += (char)payload[i];
    
    Serial.print("[MQTT] Nhận lệnh: "); Serial.println(message);

    // Giải mã JSON lệnh (Khớp với cấu trúc Python: {"device_id": "NODE_01", "action": "LOCK"})
    StaticJsonDocument<200> doc;
    deserializeJson(doc, payload, length);
    String action = doc["action"];

    if (action == "LOCK") {
        currentState = LOCKED_BY_USER;
        sendStatusFeedback("LOCK");
    } else if (action == "UNLOCK") {
        currentState = SAFE;
        sendStatusFeedback("UNLOCK");
    }
}

void reconnectMqtt() {
    while (!client.connected()) {
        Serial.print("Đang kết nối MQTT...");
        if (client.connect("Gateway_Flood_ESP32")) {
            Serial.println("Đã kết nối!");
            client.subscribe(topic_ctrl);
        } else {
            Serial.print("Lỗi, rc="); Serial.print(client.state());
            Serial.println(" Thử lại sau 5 giây");
            delay(5000);
        }
    }
}

void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    memcpy(&incoming, data, sizeof(incoming));
    currentLevel = incoming.level;
    lastRecvTime = millis();
    isNodeOnline = true;
}

void setup() {
    Serial.begin(115200);
    gate.attach(SERVO_PIN);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_CONN, OUTPUT);
    
    lcd.init(); lcd.backlight();
    lcd.setCursor(0,0); lcd.print("System Starting");

    WiFiManager wm;
    wm.setConfigPortalTimeout(20);
    if (!wm.autoConnect("Gateway_Config_AP")) {
        Serial.println("[SYSTEM] WiFi Timeout - Offline Mode");
    } else {
        Serial.println("[SYSTEM] WiFi Connected!");
    }

    // Cấu hình MQTT
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    if (esp_now_init() != ESP_OK) return;
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    if (!client.connected()) reconnectMqtt();
    client.loop();

    unsigned long currentMillis = millis();

    // --- ẢI 2: MẤT KẾT NỐI & NHÁY ĐÈN ---
    if (currentMillis - lastRecvTime > 10000) {
        isNodeOnline = false;
        digitalWrite(LED_CONN, (currentMillis / 200) % 2); 
        lcd.setCursor(0, 1); lcd.print("NODE: OFFLINE   ");
    } else {
        isNodeOnline = true;
        digitalWrite(LED_CONN, LOW);
        lcd.setCursor(0, 1); lcd.print("NODE: ONLINE    ");
    }

    // --- GỬI DỮ LIỆU LÊN SERVER (MỖI 3 GIÂY) ---
    if (currentMillis - lastDataSend > 3000) {
        Serial.println("\n--- BẮT ĐẦU CHU KỲ GỬI DỮ LIỆU ---");
        sendSensorDataHTTP(); // Gửi data cảm biến qua HTTP
        sendStatusMQTT();      // Gửi trạng thái qua MQTT
        lastDataSend = currentMillis;
    }

    // --- HIỂN THỊ LCD ---
    lcd.setCursor(0, 0);
    String waterStatus = "        ";
    if (currentLevel > 6.0)      waterStatus = "CAN DAY ";
    else if (currentLevel > 3.0) waterStatus = "NUA NUOC";
    else if (currentLevel <= 2.5) waterStatus = "SAP TRAN";
    lcd.print(String(currentLevel, 1) + "cm " + waterStatus);

    // --- LOG SERIAL ---
    if (currentMillis - lastLogTime > 2000) {
        Serial.printf("Lvl: %.1f | Node: %s | State: %d\n", currentLevel, isNodeOnline?"ON":"OFF", currentState);
        lastLogTime = currentMillis;
    }

    // --- ẢI 3: FSM XẢ LŨ ---
    // --- THAY ĐỔI TRONG PHẦN SWITCH CASE CỦA LOOP ---

    switch (currentState) {
        case SAFE:
            gate.write(0);
            digitalWrite(BUZZER_PIN, LOW);
            // Nếu mực nước xuống dưới ngưỡng báo động (3.5cm)
            if (currentLevel <= 3.5 && isNodeOnline) {
                Serial.println("[FSM] CẢNH BÁO: Mực nước nguy hiểm! Kích hoạt quy trình xả lũ.");
                currentState = DISCHARGING;
                stateTimer = currentMillis;
                dischargeStep = 0; // Bắt đầu bằng việc hú còi
            }
            break;

        case DISCHARGING:
            if (dischargeStep == 0) { 
                // Bước 1: Hú còi cảnh báo trong 4 giây trước khi vận hành cửa đập
                digitalWrite(BUZZER_PIN, (currentMillis / 250) % 2); 
                if (currentMillis - stateTimer > 4000) { 
                    dischargeStep = 1; // Chuyển sang bước xả
                    Serial.println("[FSM] Hú còi xong. Bắt đầu xả lũ theo cấp độ.");
                }
            } 
            else if (dischargeStep == 1) { 
                // Bước 2: Xả lũ phân cấp dựa trên mực nước
                digitalWrite(BUZZER_PIN, LOW); // Tắt còi sau khi đã báo động xong

                if (currentLevel <= 2.5) {
                    // CỰC NGUY HIỂM: Mở 50% (Góc ~ 45 độ)
                    gate.write(45);
                    Serial.println("[FSM] MỨC 2: Nguy hiểm cao! Mở xả 50% (45 deg)");
                } 
                else if (currentLevel <= 3.5) {
                    // BÁO ĐỘNG: Mở 20% (Góc ~ 18 độ)
                    gate.write(75);
                    Serial.println("[FSM] MỨC 1: Báo động! Mở xả 20% (18 deg)");
                }

                // Điều kiện dừng: Khi nước rút lên trên mức an toàn (6.0cm)
                if (currentLevel >= 6.0) {
                    Serial.println("[FSM] AN TOÀN: Nước đã rút. Đóng đập.");
                    currentState = SAFE;
                }
            }
            break;

        case LOCKED_BY_USER:
            // Chế độ cưỡng chế: Luôn đóng đập bất kể mực nước
            gate.write(90);
            digitalWrite(BUZZER_PIN, LOW);
            break;
    }
}