import paho.mqtt.client as mqtt
from supabase import create_client, Client
import json
from datetime import datetime

# --- CẤU HÌNH SUPABASE ---
SUPABASE_URL = "https://vhueauamdzmysquvpjnz.supabase.co"
SUPABASE_SERVICE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InZodWVhdWFtZHpteXNxdXZwam56Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NjgxOTc1NTAsImV4cCI6MjA4Mzc3MzU1MH0.TYdqA-qvPP8QwLsuw90fZxfmp_hPzslaEapAbKjytL0"
supabase: Client = create_client(SUPABASE_URL, SUPABASE_SERVICE_KEY)

# --- CẤU HÌNH MQTT ---
BROKER = "localhost" # Hoặc IP máy tính của bạn
PORT = 1883
TOPICS = [("factory/sensor/+/temp", 1), ("factory/control/+/stat", 1)]

# 1. Xử lý dữ liệu nhiệt độ (Uplink)
def handle_sensor_data(node_id_raw, payload):
    try:
        node_str = f"NODE_{int(node_id_raw):02d}"
        data = {
            "node_id": node_str,
            "gateway_id": "GATEWAY_ESP32",
            "sensor_value": float(payload)
        }
        supabase.table("measurements").insert(data).execute()
        print(f"📊 [SENSOR] {node_str}: {payload}°C -> Đã lưu Supabase")
    except Exception as e:
        print(f"❌ [DB ERROR] {e}")

# 2. Xử lý phản hồi trạng thái (Feedback Loop)
def handle_device_feedback(payload_json):
    try:
        data = json.loads(payload_json)
        device_id = data.get("device_id")
        
        # Lấy giá trị từ 'action' vì Gateway gửi ngược lại 'action' chứ không phải 'status'
        action_received = data.get("action") 
        value = data.get("value")
        
        # Ánh xạ action sang status để lưu vào DB (Ví dụ: RESET -> ON hoặc READY)
        status_to_db = "ALARM_READY" if action_received == "RESET" else "ALARM_OFF"
        
        current_time = datetime.now().isoformat()
        
        supabase.table("devices").update({
            "status": status_to_db, # Lưu giá trị đã ánh xạ
            "last_value": value, 
            "updated_at": current_time
        }).eq("device_id", device_id).execute()

        print(f"⚙️ [FEEDBACK] Node {device_id} xác nhận: {action_received} -> DB: {status_to_db}")
    except Exception as e:
        print(f"❌ [FEEDBACK ERROR] {e}")

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ MQTT Worker đã kết nối")
        client.subscribe(TOPICS)
    else:
        print(f"❌ Lỗi kết NULL: {rc}")

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        data = json.loads(payload)
        
        if "sensor" in msg.topic:
            temp = data.get('temp')
            # Kiểm tra cờ batch từ Gateway gửi lên
            is_batch = data.get('batch', False)
            label = "BATCH_DATA" if is_batch else "LIVE_DATA"
            
            print(f"{'⏳' if is_batch else '⚡'} [DATA] Node 01: {temp}°C | Loại: {label}")
            
            # Lưu vào Supabase với cột 'note' để Dashboard nhận biết
            supabase.table("measurements").insert({
                "node_id": "NODE_01", 
                "sensor_value": temp,
                "note": label 
            }).execute()
            
        elif "stat" in msg.topic:
            action = data.get('action')
            # Ánh xạ thêm trạng thái Đèn
            status_map = {
                "RESET": "ALARM_READY", 
                "OFF": "ALARM_OFF", 
                "RELAY_ON": "LED_ON", 
                "RELAY_OFF": "LED_OFF"
            }
            new_status = status_map.get(action, "UNKNOWN")
            
            print(f"⚙️ [STAT] Node xác nhận lệnh: {action} -> DB: {new_status}")
            supabase.table("devices").update({"status": new_status}).eq("device_id", "NODE_01").execute()
            
    except Exception as e:
        print(f"⚠️ [ERROR] {e}")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER, PORT, 60)
client.loop_forever()