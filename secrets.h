// Update this file with your credentials

// WiFi
// --------------------------------------------------------------------------------
const char* ssid = "your_wifi_ssid";  // <------- add your Wi Fi details
const char* password = "your_wifi_password"; // <------- add your Wi Fi details

// MQTT
// --------------------------------------------------------------------------------
const char* mqtt_server = "your.mqtt.broker"; // <------- add your MQTT details
String mqtt_user = "your_mqtt_user"; // <------- add your MQTT details
String mqtt_pass = "your_mqtt_password"; // <------- add your MQTT details
const char* mqtt_topic = "outwardMQTTtopic"; // <------- add your MQTT details
String clientId = "id_of_your_choosing"; // <------- add a client ID
String mqtt_in = "inwardMQTTtopic"; // <------ the topic to listen to if needed
unsigned int mqtt_port = 1883; // <------ MQTT Port.  1883/1884/8883/8884
