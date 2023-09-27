#pragma GCC optimize("-Ofast")
//#define USE_FIREBASE  // FBWrite(path, data); String txt = FBRead(path);
#define USE_MQTT      // MQTTsend(topic, data); String txt = MQTTget(topic);
#define USE_TELNET    // TELNETsend(data); String txt=TELNETget();

#include <Arduino.h>
#include <WiFiClient.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <NTPClient.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200, 60000);

#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "WIFI_PASSWORD"

#ifdef USE_FIREBASE
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <ArduinoJson.h>
#define API_KEY "API_KEY"
#define DATABASE_URL "DATABASE_URL"
#define USER_EMAIL "USER_EMAIL"
#define USER_PASSWORD "USER_PASSWORD"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String FBRead(String path) {
  String r;
  byte x = 0;
  do {
    Firebase.RTDB.getString(&fbdo, path);
    r = fbdo.to<const char*>();
    x++;
    if (x > 10) break;
  } while (r == "");
  return r;
}

void FBWrite(String path, String data) {
  Firebase.RTDB.setString(&fbdo, path, data);
}
#endif

#ifdef USE_MQTT
#include <PubSubClient.h>
#define mqtt_server "broker.emqx.io"
#define mqtt_port 1883
//mqtt_use_credentials
#ifdef mqtt_use_credentials
#define mqtt_user "mqtt_user"
#define mqtt_password "mqtt_password"
#endif
#define sub_topic1 "/zz/testinput1"
#define sub_topic2 "/zz/testinput2"
#define sub_topic3 "/zz/testinput3"
#define sub_topic4 "/zz/testinput4"
String pyld[4 + 1] = {}, xtopic;
byte tp;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void mqttReconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESPClient-";
    clientId += String(random(0xffff), HEX);
#ifdef mqtt_use_credentials
    if (mqttClient.connect("ESPClient", mqtt_user, mqtt_password))
#else
    if (mqttClient.connect(clientId.c_str()))
#endif
    {
      Serial.println("connected");
      mqttClient.subscribe(sub_topic1);
      mqttClient.subscribe(sub_topic2);
      mqttClient.subscribe(sub_topic3);
      mqttClient.subscribe(sub_topic4);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
void MQTT_LOOP() {
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  xtopic = topic;
  if (String(topic) == String(sub_topic1)) tp = 1;
  if (String(topic) == String(sub_topic2)) tp = 2;
  if (String(topic) == String(sub_topic3)) tp = 3;
  if (String(topic) == String(sub_topic4)) tp = 4;
  for (unsigned int i = 0; i < length; i++) {
    pyld[tp] += String(char(payload[i]));
  }
}
void MQTTsend(const char* tpc, String msg) {
  mqttClient.publish(tpc, msg.c_str());
}
#endif

#ifdef USE_TELNET
uint8_t i;
bool ConnectionEstablished; 
#define MAX_TELNET_CLIENTS 2
WiFiServer TelnetServer(23);
WiFiClient TelnetClient[MAX_TELNET_CLIENTS];

void TELNETsend(String text) {
  for (i = 0; i < MAX_TELNET_CLIENTS; i++) {
    if (TelnetClient[i] || TelnetClient[i].connected()) {
      TelnetClient[i].println(text);
    }
  }
  delay(10);
}

String TELNETget() {
  String readTelnet = "";
  for (i = 0; i < MAX_TELNET_CLIENTS; i++) {
    if (TelnetClient[i] && !TelnetClient[i].connected()) {
      Serial.print("Client disconnected ... terminate session ");
      Serial.println(i + 1);
      TelnetClient[i].stop();
    }
  }
  if (TelnetServer.hasClient()) {
    ConnectionEstablished = false;  // Set to false
    for (i = 0; i < MAX_TELNET_CLIENTS; i++) {
      if (!TelnetClient[i]) {
        TelnetClient[i] = TelnetServer.available();

        Serial.print("New Telnet client connected to session ");
        Serial.println(i + 1);

        TelnetClient[i].flush();  // clear input buffer, else you get strange characters
        TelnetClient[i].println("Welcome!");
        TELNETsend("Connected with ip:" + WiFi.localIP().toString());
        ConnectionEstablished = true;
        break;
      } else {
      }
    }
    if (ConnectionEstablished == false) {
      Serial.println("No free sessions ... drop connection");
      TelnetServer.available().stop();
    }
  }
  for (i = 0; i < MAX_TELNET_CLIENTS; i++) {
    if (TelnetClient[i] && TelnetClient[i].connected()) {
      if (TelnetClient[i].available()) {
        while (TelnetClient[i].available()) {
          readTelnet = TelnetClient[i].readString();
        }
      }
    }
  }
  return readTelnet;
}
#endif

String getserial() {
  String txt = "";
  while (Serial.available()) {
    txt = Serial.readString();
    txt.trim();
  }
  return txt;
}

void Test() {
  timeClient.update();
  String rnd = timeClient.getFormattedTime();
  String senddata = getserial();

#ifdef USE_MQTT
  if (senddata != "") MQTTsend("/zz/testoutput", rnd + " : " + senddata);
  if (pyld[tp] != "") {
    Serial.println("Topic: " + String(xtopic) + " payload: " + pyld[tp]);
    pyld[tp] = "";
  }
#endif

#ifdef USE_FIREBASE
  if (senddata != "") {
    FBWrite("ESP_Basic/test", rnd + " : " + senddata);
    String txt = FBRead("ESP_Basic/test");
    Serial.println("Firebase Data: " + txt);
  }
#endif

#ifdef USE_TELNET
  if (senddata != "") TELNETsend(rnd + " : " + senddata);
  String telnetmsg = TELNETget();
  if (telnetmsg != "") Serial.println("Telnet message: " + telnetmsg);
#endif
}

void setup() {
  delay(3000);
  Serial.begin(115200);
  Serial.println("Connecting to: " + String(WIFI_SSID));
  WiFi.mode(WIFI_STA);  // Setup ESP in client mode
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

#ifdef USE_FIREBASE
  Serial.printf("\n\rFirebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);
  fbdo.setResponseSize(2048);
  Firebase.begin(&config, &auth);
  Firebase.setDoubleDigits(5);
  config.timeout.serverResponse = 10 * 1000;
#endif

#ifdef USE_MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callback);
#endif

#ifdef USE_TELNET
  TelnetServer.begin();
  TelnetServer.setNoDelay(true);
#endif
  timeClient.begin();
}

void loop() {
  MQTT_LOOP();
  Test();
  yield();
}
