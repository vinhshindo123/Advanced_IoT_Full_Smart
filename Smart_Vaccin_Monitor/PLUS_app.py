from flask import Flask, render_template, request, jsonify
import paho.mqtt.publish as publish
import json
from supabase import create_client

app = Flask(__name__)

# Đảm bảo Key này khớp với mqtt_worker
SUPABASE_URL = "https://vhueauamdzmysquvpjnz.supabase.co"
SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InZodWVhdWFtZHpteXNxdXZwam56Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NjgxOTc1NTAsImV4cCI6MjA4Mzc3MzU1MH0.TYdqA-qvPP8QwLsuw90fZxfmp_hPzslaEapAbKjytL0"
supabase = create_client(SUPABASE_URL, SUPABASE_KEY)

# ĐỊNH NGHĨA IP MÁY TÍNH CHẠY BROKER Ở ĐÂY
MQTT_IP = "localhost" # ĐỔI THÀNH "192.168.0.103" NẾU CHẠY THẬT

@app.route('/')
def index():
    return render_template('PLUS_index.html')

@app.route('/api/data')
def get_data():
    try:
        measurements = supabase.table("measurements").select("*").order("id", desc=True).limit(15).execute()
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
    action = request.json.get('action') 
    # Action có thể là: 'OFF', 'RESET', 'RELAY_ON', 'RELAY_OFF'
    payload = {"device_id": "NODE_01", "action": action, "value": 0}
    publish.single("factory/control/NODE_01/cmd", json.dumps(payload), hostname=MQTT_IP)
    return jsonify({"status": "success", "command_sent": action})

@app.route('/control/relay', methods=['POST'])
def control_relay():
    action = request.json.get('action') # "RELAY_ON" hoặc "RELAY_OFF"
    payload = {"device_id": "NODE_01", "action": action, "value": 1}
    # Gửi lệnh xuống Gateway qua Topic /cmd
    publish.single("factory/control/NODE_01/cmd", json.dumps(payload), hostname=MQTT_IP)
    return jsonify({"status": "success", "relay": action})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)