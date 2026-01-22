from flask import Flask, render_template, request, jsonify
import paho.mqtt.publish as publish
import json
from supabase import create_client

app = Flask(__name__)

# Cấu hình Supabase đồng bộ với Worker
SUPABASE_URL = "https://vhueauamdzmysquvpjnz.supabase.co"
SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InZodWVhdWFtZHpteXNxdXZwam56Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NjgxOTc1NTAsImV4cCI6MjA4Mzc3MzU1MH0.TYdqA-qvPP8QwLsuw90fZxfmp_hPzslaEapAbKjytL0"
supabase = create_client(SUPABASE_URL, SUPABASE_KEY)

MQTT_IP = "localhost" # Đổi thành IP thực tế của máy chạy Broker

@app.route('/')
def index():
    return render_template('PRM_index.html') # Đảm bảo file HTML nằm trong thư mục templates

@app.route('/api/data')
def get_data():
    try:
        # Lấy 15 dữ liệu mực nước mới nhất
        measurements = supabase.table("measurements").select("*").order("id", desc=True).limit(15).execute()
        # Lấy trạng thái đập hiện tại
        device_status = supabase.table("devices").select("*").eq("device_id", "NODE_01").execute()
        
        return jsonify({
            "measurements": measurements.data,
            "device": device_status.data[0] if device_status.data else {"status": "UNKNOWN"}
        })
    except Exception as e:
        print(f"API Error: {e}")
        return jsonify({"error": str(e)}), 500

@app.route('/control', methods=['POST'])
def control():
    # Nhận lệnh từ nút bấm trên Web (LOCK hoặc UNLOCK)
    action = request.json.get('action') 
    payload = {"device_id": "NODE_01", "action": action}
    
    # Gửi lệnh xuống Topic điều khiển mà Gateway đang lắng nghe
    publish.single("esp32/flood/control", json.dumps(payload), hostname=MQTT_IP)
    return jsonify({"status": "success", "command_sent": action})

@app.route('/api/sensor', methods=['POST'])
def receive_sensor_data():
    try:
        data = request.json
        level = data.get('level')
        node_status = data.get('node')
        
        print(f"📥 [HTTP] Nhận từ Gateway: Level={level}, Node={node_status}")
        
        # Lưu vào Supabase ngay lập tức
        supabase.table("measurements").insert({
            "node_id": "NODE_01", 
            "sensor_value": level,
            "note": f"HTTP_UPLINK | Node: {node_status}"
        }).execute()
        
        return jsonify({"status": "success"}), 200
    except Exception as e:
        print(f"❌ [HTTP ERROR] {e}")
        return jsonify({"status": "error", "message": str(e)}), 500

if __name__ == '__main__':
    # Chạy trên mọi IP trong mạng nội bộ để điện thoại có thể truy cập Dashboard
    app.run(host='0.0.0.0', port=5000, debug=True)