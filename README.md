# ESP_Connect

A basic code to connect via WiFi to establish connections with Google Firebase database and MQTT server. In addition, a connection to the code can be established externally via a telnet client.
It is possible to send text back and forth in both directions.

Google Firebase:

FBWrite(path, data);

String txt = FBRead(path);

MQTT:

MQTTsend(topic, data);

String txt = MQTTget(topic);

Telnet:

TELNETsend(data);

String txt=TELNETget();

4 predefined topics are subscribed to for MQTT and can be expanded as desired using the example code.
With the compiler instructions #ifdef and #endif, only relevant parts are used in the code that were also uncommented in the header with #define.
So if you only want to use the Telnet function, #define USE_MQTT and #define USE_FIREBASE must be commented out.
In addition to the WiFi access data, the relevant data must be entered in the code for Firebase and MQTT.
