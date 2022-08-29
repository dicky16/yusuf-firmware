#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <espnow.h>

//init temperature
String temperature[5] = {};

//pasrsing data with #
String getValue(String data, char separator, int index)
  {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
      if (data.charAt(i) == separator || i == maxIndex)
      {
        found++;
        strIndex[0] = strIndex[1] + 1;
        strIndex[1] = (i == maxIndex) ? i + 1 : i;
      }
    }

    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
  }

//init esp now for communication
typedef struct struct_message
{
  int id;
  float t;
  float h;
} struct_message;

// Create a struct_message called myData
struct_message myData;

// Create a structure to hold the readings from each board
struct_message board1;
struct_message board2;

// Create an array with all the structures
struct_message boardsStruct[2] = {board1, board2};

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t *mac_addr, uint8_t *incomingData, uint8_t len)
{
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.printf("Board ID %u: %u bytes\n", myData.id, len);
  // Update the structures with the new incoming data
  boardsStruct[myData.id - 1].t = myData.t;
  boardsStruct[myData.id - 1].h = myData.h;
  Serial.print("Data Temp Received : ");
  Serial.println(String(myData.t));
  Serial.println();
}

//init device
String heater, cooling, inverter;
long now = millis();
long lastMeasure = 0;

//wifi
String ssid = "realme C3";
String password = "12345678";

//mqtt
//init variable
WiFiClient wifiClient;
PubSubClient client(wifiClient);

String deviceID = "suhu";
String responseSuhu1;
String flockID = "flock_1";
//init server
const char *brokerUser = "otochicken";
const char *brokerPass = "oto";
const char *broker = "18.119.46.225";
String mqttID = "mqtt_client_" + flockID;

//init topic
String topicTemperature1 = "flock/in/suhu";
String heaterTopic = "flock/"+flockID+"/heater/cloud";
String coolingTopic = "flock/"+flockID+"/cooling/cloud";
String inverterTopic = "flock/"+flockID+"/inverter/cloud";

String temperatureOutTopic = "flock/temperature";

//callback mqtt
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if(topic=="flock/"+flockID+"/pemanas"){
      Serial.print("Changing Pemanas to ");
      if(messageTemp == "on"){
        // digitalWrite(lamp, HIGH);
        Serial.print("On");
      }
      else if(messageTemp == "off"){
        // digitalWrite(lamp, LOW);
        Serial.print("Off");
      }
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", brokerUser, brokerPass)) {
      Serial.println("connected");  
      client.subscribe("room/lamp");
      client.subscribe(topicTemperature1.c_str());
      client.subscribe(heaterTopic.c_str());
      client.subscribe(coolingTopic.c_str());
      client.subscribe(inverterTopic.c_str());
      Serial.println("subscribe to : " + topicTemperature1);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}

//wifi
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
}

void setup(){
  Serial.begin(115200);
  Serial.println();
  Serial.print("ESP8266 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  //init esp now
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_AP_STA);
  // WiFi.disconnect();
  //wifi setup
  setup_wifi();

  Serial.print("MAC ");
  Serial.println(WiFi.macAddress());

  // Init ESP-NOW
  if (esp_now_init() != 0)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);

  //mqtt setup
  client.setServer(broker, 1883);
  client.setCallback(callback);
}
 
void loop(){

  if (WiFi.status() == WL_CONNECTED)
  {
    if (!client.connected()) {
      reconnect();
    }

    if(!client.loop())
      client.connect("ESP8266Client", brokerUser, brokerPass);

    //publish data
    now = millis();
    // Publishes new temperature and humidity every 5 seconds
    if (now - lastMeasure > 5000) {
      lastMeasure = now;
      StaticJsonDocument<96> doc;
      char outTemperature[512];
      doc["flock_id"] = "flock_1";
      doc["suhu"] = boardsStruct[1].t;
      doc["kelembapan"] = boardsStruct[1].h;
      doc["position"] = "depan";

      serializeJson(doc, outTemperature);

      client.publish(temperatureOutTopic.c_str(), outTemperature);
    }

  }

  float tempA = boardsStruct[1].t;
  Serial.print("Temp Location A : ");
  Serial.println(tempA);
  delay(1000);

}
