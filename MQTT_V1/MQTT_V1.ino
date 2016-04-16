// This example uses an Arduino Yun and the
// YunMQTTClient to connect to shiftr.io.
//
// The YunMQTTClient uses a Linux side python
// script to manage the connection which results
// in less program space and memory used on the Arduino.
//
// You can check on your device after a successful
// connection here: https://shiftr.io/try.
//
// by Joël Gähwiler
// https://github.com/256dpi/arduino-mqtt

const int ledPin2 = 18;
const int ledPin3 = 19;
int actchoose = 0;

#include <Bridge.h>
#include <YunMQTTClient.h>
YunMQTTClient client;
unsigned long lastMillis = 0;

#include <DHT.h>
#define DHTPIN 8
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#include <IRremote.h>
int khz = 38; // 38kHz carrier frequency
unsigned int  rawData[211] = {3400, 3400, 450, 1250, 500, 350, 450, 1300, 500, 1200, 500, 350, 450, 450, 400, 1250, 450, 450, 450, 1250,
                              450, 450, 400, 1250, 450, 450, 450, 1250, 450, 1250, 450, 1300, 450, 400, 450, 400, 450, 1250, 450, 450, 400, 450, 450, 1250, 450, 1250, 450, 450,
                              400, 1300, 450, 400, 450, 1250, 450, 450, 400, 1300, 450, 400, 450, 450, 400, 450, 400, 1300, 450, 1250, 450, 1250, 450, 1300, 450, 1250, 450, 450,
                              400, 450, 400, 450, 400, 450, 400, 450, 450, 1250, 450, 1250, 450, 450, 450, 400, 450, 400, 450, 1250, 450, 450, 400, 1250, 500, 400, 450, 1250,
                              450, 450, 400, 450, 450, 1250, 450, 450, 400, 450, 400, 450, 400, 450, 400, 450, 450, 400, 450, 400, 450, 400, 450, 450, 400, 450, 450, 400, 450,
                              400, 450, 450, 400, 400, 450, 450, 400, 450, 400, 400, 500, 400, 450, 450, 400, 400, 450, 450, 400, 450, 400, 450, 450, 400, 450, 450, 400, 400, 450,
                              450, 400, 450, 400, 450, 450, 400, 400, 500, 400, 400, 450, 450, 400, 450, 400, 450, 450, 400, 450, 450, 400, 1250, 450, 450, 400, 450, 450, 400,
                              400, 450, 450, 1250, 450, 450, 400, 450, 450, 1250, 450, 1250, 450, 450, 400, 450, 450, 400, 450
                             };
int rawsize = sizeof(rawData) / sizeof(rawData[0]);
//IRsend irsend;

void setup() {
  Bridge.begin();
  Serial.begin(9600);
  client.begin("broker.shiftr.io");
  pinMode(ledPin3, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  //irsend.sendRaw(rawData, rawsize, khz);
  connect();
}

void connect() {
  Serial.print("connecting...");
  while (!client.connect("Arduino1", "31fc5c29", "856b8426807a09c4")) {
    Serial.print(".");
  }

  Serial.println("\nconnected!");

  client.subscribe("/room1/temp");
  client.subscribe("/room1/humid");
  client.subscribe("/control/room1/LED");
  client.subscribe("/control/room1/AIR");
  client.subscribe("/webflash");
}

void loop() {
  client.loop();

  if (!client.connected()) {
    connect();
  }

  static char temperature[3] ;
  static char moisture[3] ;
  int temp = dht.readTemperature();
  int moi = dht.readHumidity();
  sprintf(temperature, "%d", temp);
  sprintf(moisture, "%d", moi);

  // publish a message roughly every second.
  if (millis() - lastMillis > 2000) {
    lastMillis = millis();
    client.publish("/room1/temp", temperature);
    client.publish("/room1/humid", moisture);
  }

  if (actchoose == 1) {
    int pinstatus =  digitalRead(ledPin3);
    char ledstatus[1] ;
    sprintf(ledstatus, "%d", pinstatus);
    client.publish("/control/room1/LED/status", ledstatus);
    actchoose = 0;
  } else if (actchoose == 2) {
    IRsend irsend;
    irsend.sendRaw(rawData, rawsize, khz);
    irsend.sendRaw(rawData, rawsize, khz);
    int pinstatus =  digitalRead(ledPin2);
    char ledstatus[1] ;
    sprintf(ledstatus, "%d", pinstatus);
    client.publish("/control/room1/AIR/status", ledstatus);
    actchoose = 0;
  }
}

void messageReceived(String topic, String payload, char * bytes, unsigned int length) {
  Serial.print("incoming: ");
  Serial.print(topic);
  Serial.print(" - ");
  Serial.print(payload);
  Serial.println();

  if (topic == "/control/room1/LED" && payload == "switch") {
    PORTF = (~PORTF & B01000000) | (PORTF & B10111111);
    actchoose = 1;
    //crash below
    /*int pinstatus =  digitalRead(ledPin3);
      char ledstatus[1] ;
      sprintf(ledstatus,"%d", pinstatus);
      client.publish("/control/room1/LED/status", ledstatus);*/
  } else if (topic == "/control/room1/AIR" && payload == "switch") {
    PORTF = (~PORTF & B10000000) | (PORTF & B01111111);
    actchoose = 2;
  } else if (topic == "/webflash" && payload == "flash") {
    actchoose = 1;
  }
}
