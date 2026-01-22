#include <esp_now.h>
#include <WiFi.h>

#define ALARM_PIN 4    // Còi (Buzzer)
#define RELAY_PIN 5    // LED giả lập Relay (Đèn)
#define SENSOR_PIN 34  

float ema_value = 5.0; 
float alpha = 0.08;    // Bộ lọc mượt
bool forceAlarmOff = false;
bool gatewayOnline = true; 
uint8_t gatewayAddr[] = {0x3C, 0x8A, 0x1F, 0x50, 0x0A, 0xB8}; 

struct DataPacket { float temp; };
struct ControlCmd { char action[10]; int value; };

// Hàm lọc trung vị loại bỏ nhiễu nhảy số (1,2,3,4)
float readMedian() {
    int samples[10];
    for (int i = 0; i < 10; i++) {
        samples[i] = analogRead(SENSOR_PIN);
        delay(5);
    }
    for (int i = 0; i < 9; i++) {
        for (int j = i + 1; j < 10; j++) {
            if (samples[i] > samples[j]) {
                int temp = samples[i];
                samples[i] = samples[j];
                samples[j] = temp;
            }
        }
    }
    return (samples[4] + samples[5]) / 2.0;
}

void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
    gatewayOnline = (status == ESP_NOW_SEND_SUCCESS);
}

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    ControlCmd cmd;
    memcpy(&cmd, data, sizeof(cmd));
    
    // LOGIC ĐIỀU KHIỂN MỚI
    if (strcmp(cmd.action, "OFF") == 0) {
        forceAlarmOff = true;
        digitalWrite(ALARM_PIN, LOW); // Tắt còi ngay lập tức
    } 
    else if (strcmp(cmd.action, "RESET") == 0) {
        forceAlarmOff = false;
    }
    else if (strcmp(cmd.action, "RELAY_ON") == 0) {
        digitalWrite(RELAY_PIN, HIGH); // Bật đèn
    }
    else if (strcmp(cmd.action, "RELAY_OFF") == 0) {
        digitalWrite(RELAY_PIN, LOW);  // Tắt đèn
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(ALARM_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    
    digitalWrite(ALARM_PIN, LOW);
    digitalWrite(RELAY_PIN, LOW); // Mặc định ĐÈN SÁNG (An toàn)

    WiFi.mode(WIFI_STA);
    esp_now_init();
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, gatewayAddr, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
}

void loop() {
    float medianADC = readMedian();
    float rawTemp = (medianADC / 4095.0) * 50; 
    ema_value = (alpha * rawTemp) + ((1 - alpha) * ema_value);

    // LOGIC ĐẢO NGƯỢC THEO YÊU CẦU
    if (ema_value > 8.0) {
        if (!forceAlarmOff) {
            digitalWrite(ALARM_PIN, HIGH); 
        }
    } else {
        digitalWrite(ALARM_PIN, LOW);
    }

    DataPacket pkg = { ema_value };
    esp_now_send(gatewayAddr, (uint8_t *)&pkg, sizeof(pkg));
    Serial.printf("Temp: %.2f°C | LED: %s | Buzzer: %s\n", 
                  ema_value, digitalRead(RELAY_PIN)?"ON":"OFF", digitalRead(ALARM_PIN)?"ALARM":"SILENT");
    delay(1000);
}