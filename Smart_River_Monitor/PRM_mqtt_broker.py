import paho.mqtt.client as mqtt
from supabase import create_client, Client
import json
from datetime import datetime

# --- CẤU HÌNH SUPABASE ---
SUPABASE_URL = "https://vhueauamdzmysquvpjnz.supabase.co"
SUPABASE_SERVICE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InZodWVhdWFtZHpteXNxdXZwam56Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NjgxOTc1NTAsImV4cCI6MjA4Mzc3MzU1MH0.TYdqA-qvPP8QwLsuw90fZxfmp_hPzslaEapAbKjytL0"
supabase: Client = create_client(SUPABASE_URL, SUPABASE_SERVICE_KEY)

# --- CẤU HÌNH MQTT ---
BROKER = "localhost"  # Đổi thành IP máy tính nếu chạy thực tế
PORT = 1883
# Topic lắng nghe: dữ liệu nước và trạng thái phản hồi từ Gateway
TOPICS = [("esp32/flood/data", 1), ("esp32/flood/stat", 1)]

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ MQTT Worker đã kết nối và đang lắng nghe...")
        client.subscribe(TOPICS)
    else:
        print(f"❌ Lỗi kết nối MQTT: {rc}")

# --- SỬA LẠI HÀM ON_MESSAGE TRONG MQTT_WORKER.PY ---

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        data = json.loads(payload)
        
        # CHỈ XỬ LÝ TRẠNG THÁI (Vì Level đã gửi qua HTTP rồi)
        if msg.topic == "esp32/flood/stat":
            gate_status = data.get('gate') # "OPEN" hoặc "CLOSE"
            system_mode = data.get('mode') # "AUTO" hoặc "LOCKED"
            
            # Ánh xạ để hiển thị lên Dashboard
            db_status = ""
            if system_mode == "LOCKED":
                db_status = "USER_LOCKED"
            else:
                db_status = f"GATE_{gate_status}"
            
            supabase.table("devices").update({
                "status": db_status,
                "updated_at": datetime.now().isoformat()
            }).eq("device_id", "NODE_01").execute()
            
            print(f"⚙️ [MQTT STAT] Gateway báo: Mode={system_mode}, Gate={gate_status}")
            
    except Exception as e:
        print(f"⚠️ [MQTT WORKER ERROR] {e}")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER, PORT, 60)
client.loop_forever()