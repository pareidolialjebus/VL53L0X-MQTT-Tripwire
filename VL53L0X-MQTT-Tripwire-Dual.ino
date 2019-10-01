// V0.6
// Dual VL53L0x Time of Flight Laser Rangefinders as directional tripwire
// Reports "on1" and "on2" as MQTT payload when beams broken by a person
// or objects travelling as long as they break both beams sequentially.
//
// Although not strictly necessary, have left in callback function in as it could
// be useful for future version of this sketch.
//
// PareidolialJebus (2019) - "Jee Bus"
// pareidolialjebus at protonmail.com
//
// Sketch heavily modified version of example file supplied with the the Adafruit
// Library found here: https://github.com/adafruit/Adafruit_VL53L0X
//
// MQTT implemented using the PubSubClient library and example code by
// Nick O'Leary found here: https://github.com/knolleary/pubsubclient
// --------------------------------------------------------------------------------

// Libraries
// --------------------------------------------------------------------------------
#include "Adafruit_VL53L0X.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>

#include "secrets.h" // <------- dont forget to update the secrets file!

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

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;


// Flash writing stuff
byte promTest;

byte Bfour;
byte Bthree;
byte Btwo;
byte Bone;

long Lfour;
long Lthree;
long Ltwo;
long Lone;

// Direction timers
// --------------------------------------------------------------------------------
int Timer1 = 0;
int Timer2 = 0;
int Timer3 = 0;
int newDirection = 0;
int oldDirection = 0;
int dirReset = 0;
int mqMsg = 0;

// Calibration and range
int calibration_mode = 0;
long v1_max = 2000;
long v1_min = 0;
long v2_max = 2000;
long v2_min = 0;

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

  if (calibration_mode == 1) {
    if (measure1.RangeStatus != 4) {
      v1_max = measure1.RangeMilliMeter;
      Serial.println(v1_max);
    } else {
      Serial.println(v1_max);
    }
  }

  if (calibration_mode == 2) {
    if (measure2.RangeStatus != 4) {
      v2_max = measure2.RangeMilliMeter;
      Serial.println(v2_max);
    } else {
      Serial.println(v2_max);
    }
  }

  if (calibration_mode == 3) {
    if (measure1.RangeStatus != 4) {
      v1_min = measure1.RangeMilliMeter;
      Serial.println(v1_min);
    } else {
      Serial.println(v1_min);
    }
  }

  if (calibration_mode == 4) {
    if (measure2.RangeStatus != 4) {
      v2_min = measure2.RangeMilliMeter;
      Serial.println(v2_min);
    } else {
      Serial.println(v2_min);
    }
  }

  // run mode
  if (calibration_mode == 0)
  {
    // L0x1 triggered?
    if (measure1.RangeStatus != 4) {
      if ((measure1.RangeMilliMeter < v1_max) && (measure1.RangeMilliMeter > v1_min)) {
        Timer1 = 10;
      }
    }
    Timer1 = Timer1 - 1;
    if (Timer1 < 0) {
      Timer1 = 0;
    }

    // L0x2 triggered?
    if (measure2.RangeStatus != 4) {
      if ((measure2.RangeMilliMeter < v2_max) && (measure2.RangeMilliMeter > v2_min)) {
        Timer2 = 10;
      }
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

  Serial.println("WiFi connected, IP address: ");
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
  if ((char)payload[0] == '1') {
    calibration_mode = 1;
    Serial.println("Set Laser 1 Max Range");
  }
  if ((char)payload[0] == '2') {
    calibration_mode = 2;
    Serial.println("Set Laser 2 Max Range");
  }
  if ((char)payload[0] == '3') {
    calibration_mode = 3;
    Serial.println("Set Laser 1 Min Range");
  }
  if ((char)payload[0] == '4') {
    calibration_mode = 4;
    Serial.println("Set Laser 2 Min Range");
  }

  if ((char)payload[0] == '0') {
    calibration_mode = 0;

    Serial.println("Laser ranges set");

    Serial.print("L1 Max: ");
    Serial.print(v1_max);
    Serial.print(" Min: ");
    Serial.print(v1_min);

    Serial.print("  L2 Max: ");
    Serial.print(v2_max);
    Serial.print(" Min: ");
    Serial.println(v2_min);

  }

  // Write values to EEPROM (Flash really, there is no EEPROM in an ESP8266!)
  if ((char)payload[0] == '9') {

    Serial.print("Writing settings to Flash..");

    Bfour = (v1_max & 0xFF);
    Bthree = ((v1_max >> 8) & 0xFF);
    Btwo = ((v1_max >> 16) & 0xFF);
    Bone = ((v1_max >> 24) & 0xFF);
    EEPROM.write(3, Bfour);
    EEPROM.write(2, Bthree);
    EEPROM.write(1, Btwo);
    EEPROM.write(0, Bone);

    Serial.print(".");

    Bfour = (v1_min & 0xFF);
    Bthree = ((v1_min >> 8) & 0xFF);
    Btwo = ((v1_min >> 16) & 0xFF);
    Bone = ((v1_min >> 24) & 0xFF);
    EEPROM.write(7, Bfour);
    EEPROM.write(6, Bthree);
    EEPROM.write(5, Btwo);
    EEPROM.write(4, Bone);

    Serial.print(".");

    Bfour = (v2_max & 0xFF);
    Bthree = ((v2_max >> 8) & 0xFF);
    Btwo = ((v2_max >> 16) & 0xFF);
    Bone = ((v2_max >> 24) & 0xFF);
    EEPROM.write(11, Bfour);
    EEPROM.write(10, Bthree);
    EEPROM.write(9, Btwo);
    EEPROM.write(8, Bone);

    Serial.print(".");

    Bfour = (v2_min & 0xFF);
    Bthree = ((v2_min >> 8) & 0xFF);
    Btwo = ((v2_min >> 16) & 0xFF);
    Bone = ((v2_min >> 24) & 0xFF);
    EEPROM.write(15, Bfour);
    EEPROM.write(14, Bthree);
    EEPROM.write(13, Btwo);
    EEPROM.write(12, Bone);

    Serial.print(".");

    EEPROM.commit();

    Serial.println(".done!");
  }
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


// Main Setup
// --------------------------------------------------------------------------------
void setup() {

  Serial.begin(115200);
  // wait until serial port opens for native USB devices
  while (! Serial) {
    delay(1);
  }

  // Start EEPROM and reserve space
  // Address 0 -3 = l1 max
  // Address 4 -7 = l1 min
  // Address 8 -11 = l2 max
  // Address 12 -15 = l2 min
  // Address 16 = 42 if routine run before
  EEPROM.begin(64);

  // test if values stored before?
  promTest = EEPROM.read(16);

  if (promTest == 42) {

    // yes! read the saved data

    Serial.print("Reading laser ranges from flash.");
    
    Lfour = EEPROM.read(3);
    Lthree = EEPROM.read(2);
    Ltwo = EEPROM.read(1);
    Lone = EEPROM.read(0);
    v1_max = ((Lfour << 0) & 0xFF) + ((Lthree << 8) & 0xFFFF) + ((Ltwo << 16) & 0xFFFFFF) + ((Lone << 24) & 0xFFFFFFFF);

    Serial.print(".");

    Lfour = EEPROM.read(7);
    Lthree = EEPROM.read(6);
    Ltwo = EEPROM.read(5);
    Lone = EEPROM.read(4);
    v1_min = ((Lfour << 0) & 0xFF) + ((Lthree << 8) & 0xFFFF) + ((Ltwo << 16) & 0xFFFFFF) + ((Lone << 24) & 0xFFFFFFFF);

    Serial.print(".");

    Lfour = EEPROM.read(11);
    Lthree = EEPROM.read(10);
    Ltwo = EEPROM.read(9);
    Lone = EEPROM.read(8);
    v2_max = ((Lfour << 0) & 0xFF) + ((Lthree << 8) & 0xFFFF) + ((Ltwo << 16) & 0xFFFFFF) + ((Lone << 24) & 0xFFFFFFFF);

    Serial.print(".");

    Lfour = EEPROM.read(15);
    Lthree = EEPROM.read(14);
    Ltwo = EEPROM.read(13);
    Lone = EEPROM.read(12);
    v2_min = ((Lfour << 0) & 0xFF) + ((Lthree << 8) & 0xFFFF) + ((Ltwo << 16) & 0xFFFFFF) + ((Lone << 24) & 0xFFFFFFFF);
    
    Serial.print(".done!");

    Serial.print("L1 Max: ");
    Serial.print(v1_max);
    Serial.print(" Min: ");
    Serial.print(v1_min);

    Serial.print("  L2 Max: ");
    Serial.print(v2_max);
    Serial.print(" Min: ");
    Serial.println(v2_min);
    
  } else {

    Serial.println("First run, writing preset ranges to flash.");

    // no, write full range data
    v1_max = 2000;
    byte Bfour = (v1_max & 0xFF);
    byte Bthree = ((v1_max >> 8) & 0xFF);
    byte Btwo = ((v1_max >> 16) & 0xFF);
    byte Bone = ((v1_max >> 24) & 0xFF);
    EEPROM.write(3, Bfour);
    EEPROM.write(2, Bthree);
    EEPROM.write(1, Btwo);
    EEPROM.write(0, Bone);

    v1_min = 0;
    Bfour = (v1_min & 0xFF);
    Bthree = ((v1_min >> 8) & 0xFF);
    Btwo = ((v1_min >> 16) & 0xFF);
    Bone = ((v1_min >> 24) & 0xFF);
    EEPROM.write(7, Bfour);
    EEPROM.write(6, Bthree);
    EEPROM.write(5, Btwo);
    EEPROM.write(4, Bone);

    v2_max = 2000;
    Bfour = (v2_max & 0xFF);
    Bthree = ((v2_max >> 8) & 0xFF);
    Btwo = ((v2_max >> 16) & 0xFF);
    Bone = ((v2_max >> 24) & 0xFF);
    EEPROM.write(11, Bfour);
    EEPROM.write(10, Bthree);
    EEPROM.write(9, Btwo);
    EEPROM.write(8, Bone);

    v2_min = 0;
    Bfour = (v2_min & 0xFF);
    Bthree = ((v2_min >> 8) & 0xFF);
    Btwo = ((v2_min >> 16) & 0xFF);
    Bone = ((v2_min >> 24) & 0xFF);
    EEPROM.write(15, Bfour);
    EEPROM.write(14, Bthree);
    EEPROM.write(13, Btwo);
    EEPROM.write(12, Bone);

    // add 42 to address 16, this should only be run once
    EEPROM.write(16, 42);

    EEPROM.commit();

    Serial.println(".done!");

    Serial.print("L1 Max: ");
    Serial.print(v1_max);
    Serial.print(" Min: ");
    Serial.print(v1_min);

    Serial.print("  L2 Max: ");
    Serial.print(v2_max);
    Serial.print(" Min: ");
    Serial.println(v2_min);

  }

  // start wifi
  setup_wifi();

  // start MQTT outgoing
  Serial.println("Starting MQTT out");
  client.setServer(mqtt_server, mqtt_port);
  // start MQTT incoming
  Serial.println("Starting MQTT in");
  client.setCallback(callback);
  Serial.println("MQTT running");

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

// Program Loop
// --------------------------------------------------------------------------------
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  read_dual_sensors();

  if (mqMsg == 1) {
    client.publish(mqtt_topic, "on1");
    Serial.println("MQTT:on1");
    mqMsg = 0;
  }

  if (mqMsg == 2) {
    client.publish(mqtt_topic, "on2");
    Serial.println("MQTT:on2");
    mqMsg = 0;
  }

  if (mqMsg == 3) {
    client.publish(mqtt_topic, "off");
    Serial.println("MQTT:off");
    mqMsg = 0;
  }

}
