#include <esp_now.h>
#include <WiFi.h>

// --- CẤU HÌNH PHẦN CỨNG ---
#define TRIG_PIN 5
#define ECHO_PIN 18
#define TIME_TO_SLEEP 2 
#define S_TO_US_FACTOR 1000000ULL

// --- CẤU HÌNH LỌC EMA ---
// Hệ số alpha (0.0 < alpha <= 1.0). Càng nhỏ càng mượt nhưng càng chậm đáp ứng.
// 0.3 là giá trị cân bằng tốt cho đo mực nước.
RTC_DATA_ATTR float lastEMAValue = -1.0; // Lưu giá trị vào RTC Memory để không bị mất khi Deep Sleep
const float alpha = 0.3; 

uint8_t gatewayAddr[] = {0xB0, 0xCB, 0xD8, 0xCF, 0xE5, 0x88};

typedef struct struct_message {
    float level;
    int battery;
} struct_message;
struct_message myData;

// --- HÀM LỌC NHIỄU KẾT HỢP (ẢI 1) ---
float getCleanLevel() {
    int samples = 7; // Tăng lên 7 mẫu để lọc trung vị tốt hơn
    float dists[samples];
    
    // 1. Lấy mẫu và lọc thô
    for(int i=0; i<samples; i++) {
        digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
        digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
        digitalWrite(TRIG_PIN, LOW);
        long duration = pulseIn(ECHO_PIN, HIGH, 30000); // Timeout 30ms
        float d = duration * 0.034 / 2;
        
        // Loại bỏ giá trị vật lý vô lý (HC-SR04 ổn định nhất 2cm - 200cm)
        dists[i] = (d > 200 || d < 2) ? (lastEMAValue > 0 ? lastEMAValue : 200) : d; 
        delay(30);
    }
    
    // 2. Sắp xếp lấy trung vị (Median)
    for(int i=0; i<samples-1; i++) {
        for(int j=i+1; j<samples; j++) {
            if(dists[i] > dists[j]) {
                float temp = dists[i];
                dists[i] = dists[j];
                dists[j] = temp;
            }
        }
    }
    float medianValue = dists[samples/2];

    // 3. Lọc EMA (Exponential Moving Average)
    if (lastEMAValue < 0) { 
        // Lần chạy đầu tiên sau khi khởi động lại hoàn toàn
        lastEMAValue = medianValue;
    } else {
        // Công thức: EMA = alpha * Current + (1 - alpha) * Last
        lastEMAValue = (alpha * medianValue) + ((1.0 - alpha) * lastEMAValue);
    }
    
    return lastEMAValue;
}

void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    Serial.printf("[NODE] Trạng thái gửi: %s\n", status == ESP_NOW_SEND_SUCCESS ? "Thành công" : "Thất bại");
}

void setup() {
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return;
    esp_now_register_send_cb(OnDataSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, gatewayAddr, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    // Thực hiện lọc 2 tầng
    myData.level = getCleanLevel();
    myData.battery = 100; 
    
    Serial.printf("[NODE] Giá trị sau lọc (Median+EMA): %.2f cm\n", myData.level);
    
    esp_now_send(gatewayAddr, (uint8_t *) &myData, sizeof(myData));
    
    delay(500); // Đợi ngắn để hoàn tất RF
    
    Serial.println("[NODE] Deep Sleep...");
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * S_TO_US_FACTOR);
    esp_deep_sleep_start();
}

void loop() {}