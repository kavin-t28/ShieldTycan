#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

const char* ssid = "kavin";
const char* password = "nghs3333";
const char* mqtt_server = "your ip addresss";
const int mqtt_port = 1883;
//const char* mqtt_username = "mqtt_username";
//const char* mqtt_password = "mqtt_password";
const char *key = "ThisSecretKeyIsVerySecret";

WiFiClient espClient;
PubSubClient client(espClient);

int espId = 3; // Unique ID for each ESP32
bool isLeader = false;

#define DHTPIN 4          // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11     // DHT 11

DHT dht(DHTPIN, DHTTYPE);

float temperature = 0.0;
float humidity = 0.0;

float aggregatedTemperature = 0.0;
float aggregatedHumidity = 0.0;
int numNodes = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  // Handle incoming messages if needed
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("esp32/leader");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void electLeader() {
  // Implement leader election algorithm here
  // For simplicity, let's assume the ESP32 with the highest ID becomes the leader
  isLeader = (espId == 3); // Change to your desired condition
  if (isLeader) {
    client.publish("esp32/leader", "1");
    Serial.println("I am the leader");
  }
}

void readDHTSensor() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print("% Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");
}

void sendTemperatureToLeader() {
  if (!isLeader) {
    String temperatureStr = String(temperature, 2);
    String humidityStr = String(humidity, 2);
    client.publish("esp32/temperature", (temperatureStr + "," + humidityStr).c_str());
    Serial.print("Sent temperature and humidity to leader: ");
    Serial.print(temperatureStr);
    Serial.print("°C, ");
    Serial.print(humidityStr);
    Serial.println("%");
  }
}

void aggregateTemperatureData(float receivedTemperature, float receivedHumidity) {
  aggregatedTemperature += receivedTemperature;
  aggregatedHumidity += receivedHumidity;
  numNodes++;
}

float modifyData(float data) {
  byte *byteData = (byte *)&data;
  int len = sizeof(data);

  for (int i = 0; i < len; i++) {
    byteData[i] ^= key[i % strlen(key)];
  }

  float floatData;
  memcpy(&floatData, byteData, sizeof(floatData));

  return floatData;
}

void sendAggregatedDataToGateway() {
  if (isLeader && numNodes > 0) {
    // Calculate average temperature and humidity
    aggregatedTemperature /= numNodes;
    aggregatedHumidity /= numNodes;

    float encryptedTemperature = modifyData(aggregatedTemperature);
    encryptedTemperature*=1000000000000000000;
    String aggregatedData = String(encryptedTemperature, 2) + "," + String(aggregatedHumidity, 2);
    client.publish("gateway/temperature_humidity", aggregatedData.c_str());
    Serial.print("Sent aggregated temperature and humidity to gateway: ");
    Serial.print(encryptedTemperature, 2);
    Serial.print("°C, ");
    Serial.print(aggregatedHumidity, 2);
    Serial.println("%");
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  dht.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Check if leader needs to be elected periodically
  static unsigned long lastElectionTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastElectionTime > 30000) { // Check every 30 seconds
    electLeader();
    lastElectionTime = currentTime;
  }

  readDHTSensor();

  if (!isLeader) {
    sendTemperatureToLeader();
  } else {
    // Aggregate temperature and humidity data from other nodes
    // For simplicity, assume temperature and humidity data are received from other nodes
    float receivedTemperature = 25.0; // Example value received from another node
    float receivedHumidity = 50.0; // Example value received from another node
    aggregateTemperatureData(receivedTemperature, receivedHumidity);
  }

  if (isLeader) {
    // Check if all nodes have sent their data and send aggregated data to gateway
    // For simplicity, assume all nodes have sent their data after a certain time period
    if (millis() > 60000) { // Assuming after 1 minute
      sendAggregatedDataToGateway();
    }
  }

  // Perform other tasks as needed
  delay(5000); // Adjust delay according to your requirements
}
