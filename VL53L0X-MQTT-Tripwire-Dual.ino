// Dual VL53L0x Time of Flight Laser Rangefinders as directional tripwire
// Reports "on1" and "on2" as MQTT payload when beams broken by a person
// or objects travelling as long as they break both beams sequentially.
//
// This sketch is created using example code mashed together by a non-expert
// who liked the idea of adding laser tripwires to thier home assistant install.
//
// PareidolialJebus (2019)
// pareidolialjebus at protonmail.com
// (Just call me Jee Bus)
//
// --------------------------------------------------------------------------------
// Credits
// This sketch is a heavily modified version of example file supplied with the 
// Adafruit Library found here: https://github.com/adafruit/Adafruit_VL53L0X
//
// MQTT implemented using the PubSubClient library and example code by 
// Nick O'Leary found here: https://github.com/knolleary/pubsubclient
// --------------------------------------------------------------------------------

// Libraries
// --------------------------------------------------------------------------------
#include "Adafruit_VL53L0X.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// L0x1 L0x2 definitions
// --------------------------------------------------------------------------------
#define LOX1_ADDRESS 0x31
#define LOX2_ADDRESS 0x51
#define SHT_LOX1 D4
#define SHT_LOX2 D3
Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox2 = Adafruit_VL53L0X();
VL53L0X_RangingMeasurementData_t measure1;
VL53L0X_RangingMeasurementData_t measure2;

// WiFi
// --------------------------------------------------------------------------------
const char* ssid = "your_ssid";  // <------- add your Wi Fi details
const char* password = "your_pass"; // <------- add your Wi Fi details

// MQTT
// --------------------------------------------------------------------------------
const char* mqtt_server = "your.mqtt.broker"; // <------- add your MQTT details
String mqtt_user = "yourmqttuser"; // <------- add your MQTT details
String mqtt_pass = "yourmqttpass"; // <------- add your MQTT details
const char* mqtt_topic = "outTopic"; // <------- add your MQTT details
String clientId = "whateveryoulike"; // <------- add a client ID
String mqtt_in = "inTopic"; // <------ the topic to listen to if you need callback

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// Direction timers
// --------------------------------------------------------------------------------
int Timer1 = 0;
int Timer2 = 0;
int Timer3 = 0;
int newDirection = 0;
int oldDirection = 0;
int dirReset = 0;
int mqMsg = 0;

// reset L0x1 L0x2 and assign thier I2C addresses
// then boot each
// --------------------------------------------------------------------------------
void setID() {

  // reset L0x1 L0x2
  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);
  delay(10);
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);

  // activate L0x1
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, LOW);
  if (!lox1.begin(LOX1_ADDRESS)) {
    Serial.println(F("L0x1 failed to boot"));
    while (1);
  }
  delay(10);
  
  // activate L0x2
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);
  if (!lox2.begin(LOX2_ADDRESS)) {
    Serial.println(F("L0x2 failed to boot"));
    while (1);
  }
  
}

// read L0x1 L0x2 and decide if triggered / direction
// --------------------------------------------------------------------------------
void read_dual_sensors() {

  // take a reading from L0x1 L0x2
  lox1.rangingTest(&measure1, false);
  lox2.rangingTest(&measure2, false);

  // L0x1 triggered?
  if (measure1.RangeStatus != 4) {
    Timer1 = 10;
  }
  Timer1 = Timer1 - 1;
  if (Timer1 < 0) {
    Timer1 = 0;
  }

  // L0x2 triggered?
  if (measure2.RangeStatus != 4) {
    Timer2 = 10;
  }
  Timer2 = Timer2 - 1;
  if (Timer2 < 0) {
    Timer2 = 0;
  }

  // If both triggered, which direction?
  if ((Timer1 != 0) && (Timer2 != 0)) {
    if (Timer1 > Timer2) {
      newDirection = 1;
    }
    if (Timer1 < Timer2) {
      newDirection = 2;
    }
  }

  // If this direction is "new", start timer for "off"
  if ((oldDirection != newDirection) && (newDirection != 0)) {
    mqMsg = newDirection;
    Timer3 = 10;
    dirReset = 1;
  }
  if (Timer3 != 0) {
    Timer3 = Timer3 - 1;
    if (Timer3 < 0) {
      Timer3 = 0;
    }
  }
  if ((Timer3 == 0) && (dirReset == 1)) {
    mqMsg = 3;
    dirReset = 0;
    oldDirection = 0;
    newDirection = 0;
  }
  oldDirection = newDirection;
}


// wifi setup
// --------------------------------------------------------------------------------
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// recieve MQTT message from broker 
// --------------------------------------------------------------------------------
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}

// connect / reconnect MQTT broker 
// --------------------------------------------------------------------------------
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.connect(clientId.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {
      Serial.println("connected");
      client.subscribe(mqtt_in.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {

  Serial.begin(115200);
  // wait until serial port opens for native USB devices
  while (! Serial) {
    delay(1);
  }

  // start wifi
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // start lasers
  pinMode(SHT_LOX1, OUTPUT);
  pinMode(SHT_LOX2, OUTPUT);
  Serial.println("Shutdown pins inited...");
  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);
  Serial.println("Both in reset mode...(pins are low)");
  Serial.println("Starting...");
  setID();
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  read_dual_sensors();

  if (mqMsg == 1) {
    client.publish(mqtt_topic, "on1");
    Serial.print("MQTT:on1");
    mqMsg = 0;
  }

  if (mqMsg == 2) {
    client.publish(mqtt_topic, "on2");
    Serial.print("MQTT:on2");
    mqMsg = 0;
  }

  if (mqMsg == 3) {
    client.publish(mqtt_topic, "off");
    Serial.print("MQTT:off");
    mqMsg = 0;
  }

}
