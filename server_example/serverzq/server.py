import zmq
import json
import threading
from datetime import datetime
import time

class GPSBroker:
    def __init__(self):
        self.context = zmq.Context()

        # Для телефона (REQ/REP)
        self.rep_socket = self.context.socket(zmq.REP)
        self.rep_socket.bind("tcp://*:5556")

        # Для C++ (PUB)
        self.pub_socket = self.context.socket(zmq.PUB)
        self.pub_socket.bind("tcp://*:5557")

        self.last_location = None
        self.running = True

    def phone_thread(self):
        print("📱 Listening for phone on port 5556")

        while self.running:
            try:
                message = self.rep_socket.recv_string()
                print("📨 Phone sent:", message)

                if message.startswith("{"):
                    data = json.loads(message)
                else:
                    data = self.parse_simple(message)

                data["server_timestamp"] = datetime.now().isoformat()
                self.last_location = data

                json_data = json.dumps(data)

                # Рассылаем всем C++ клиентам
                self.pub_socket.send_string(json_data)
                print("📡 Broadcasted to C++:", json_data)

                # Ответ телефону
                self.rep_socket.send_string("OK - location received")

            except Exception as e:
                print("❌ Error:", e)
                self.rep_socket.send_string(f"Error: {e}")

    def parse_simple(self, msg):
        result = {}
        parts = msg.split(",")
        for p in parts:
            if ":" in p:
                k, v = p.split(":", 1)
                result[k.strip()] = float(v.strip())
        return result

    def start(self):
        t = threading.Thread(target=self.phone_thread)
        t.start()

        print("🚀 GPS Server started")
        print("📱 Phone port: 5556")
        print("💻 C++ port: 5557")

        while True:
            time.sleep(1)

if __name__ == "__main__":
    server = GPSBroker()
    server.start()