from pymongo import MongoClient
import struct

# Define the decrypt function
def decrypt(data):
    key = "ThisSecretKeyIsVerySecret"
    length = len(key)
    data_bytes = struct.pack('f', float(data)/1000000000000000000)  # 'f' represents single-precision floating-point format
    plaintext = bytearray()
    for i in range(len(data_bytes)):
        plaintext.append(data_bytes[i] ^ ord(key[i % length]))
    return (struct.unpack('f', bytes(plaintext))[0])

# MongoDB Configuration
mongo_uri = "mongodb+srv://iot:root@cluster1.eloaump.mongodb.net/"  # Update with your MongoDB Atlas connection string
mongo_db_name = "sensor"  # Update with your MongoDB database name
mongo_collection_name = "data"  # Update with your desired collection name

# Connect to MongoDB
client = MongoClient(mongo_uri)
db = client[mongo_db_name]
collection = db[mongo_collection_name]

# Open a change stream on the collection
with collection.watch() as stream:
    print("Listening for changes...")
    for change in stream:
        if change['operationType'] == 'insert':  # Check if the change is an insert operation
            field_value = change['fullDocument'].get('gateway/temperature_humidity', {}).get('data', None)
            if field_value is not None:
                decrypted_value = decrypt(field_value)
                print("Decrypted value:", decrypted_value)
