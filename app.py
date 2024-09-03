from flask import Flask
from pymongo import MongoClient
from flask_cors import CORS
import struct

app = Flask(__name__)
CORS(app)  # Enable CORS for your Flask app

client = MongoClient("mongodb+srv://iot:root@cluster1.eloaump.mongodb.net/")
db = client["sensor"]
collection = db["data"]

@app.route("/api/sensor-data", methods=["GET"])
def get_sensor_data():
    sensor_data = list(collection.find())
    if len(sensor_data)>10:
        sensor_data = sensor_data[-10:]
    for i in sensor_data:
        i["_id"] = str(i["_id"])
    return {"data" : sensor_data}

@app.route("/api/decryption/<data>", methods=["GET"])
def decrypt(data):
    key = "ThisSecretKeyIsVerySecret"
    length = len(key)
    data_bytes = struct.pack('f', float(data)/1000000000000000000)  # 'f' represents single-precision floating-point format
    plaintext = bytearray()
    for i in range(len(data_bytes)):
        plaintext.append(data_bytes[i] ^ ord(key[i % length]))
    return (struct.unpack('f', bytes(plaintext))[0])*30

if __name__ == "__main__":
    app.run(debug=True)
