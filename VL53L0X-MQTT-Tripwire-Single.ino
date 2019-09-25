// VL53L0X-MQTT-Tripwire-Single V0.2
//
// VL53L0x Time of Flight laser rangefinder as MQTT laser tripwire
// Reports "triggered" as MQTT payload when beam broken.
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
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Adafruit_VL53L0X.h"

#include "secrets.h" // <------- dont forget to update the secrets file!

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

int somethingNew = 0;
int endvalSent = 0;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect(client.connect(clientId.c_str(), mqtt_user.c_str(), mqtt_pass.c_str()))) {
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
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println("Setting up VL53L0X");
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X."));
    while(1);
  }

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // take a measurement
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);

  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    snprintf (msg, 50, "%ld", measure.RangeMilliMeter);
    client.publish(mqtt_topic, msg);
    // reset flag to make sure "off" is sent via mqtt when sensor is not tripped
    somethingNew = 1;
    endvalSent = 0;
  } else {
   // Number is out of range
   somethingNew = 0;
  }

  // send a zero to say nothing going on
  if ((somethingNew == 0) && (endvalSent == 0)){
    client.publish(mqtt_topic, "off");
    endvalSent = 1;
  }
   
}
