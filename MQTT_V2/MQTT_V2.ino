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

#include <Bridge.h>
#include <YunMQTTClient.h>

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

const int ledPin1 = 18;
const int ledPin2 = 19;
const int ledPin3 = 13;
const int ledPin4 = 3;
const int buzzer = 10;

YunMQTTClient client;
Process p;   // Run async

unsigned long lastMillis = 0;
unsigned long lastMillis2 = 0;
int lampLui = 0;
bool VisionStatus = B0;
int robotx = 500;
int roboty = 500;
#include <Pixy.h>
Pixy pixy;
#define X_CENTER        ((PIXY_MAX_X-PIXY_MIN_X)/2)
#define Y_CENTER        ((PIXY_MAX_Y-PIXY_MIN_Y)/2)
uint16_t blocks;
int32_t panError, tiltError;
int loopcnt = 0;
//bool updateonce = B0;

class ServoLoop
{
  public:
    ServoLoop(int32_t pgain, int32_t dgain);

    void updatepos(int32_t error);
    void setgain(int32_t pgain, int32_t dgain);

    int32_t m_pos;
    int32_t m_prevError;
    int32_t m_pgain;
    int32_t m_dgain;
};

ServoLoop::ServoLoop(int32_t pgain, int32_t dgain)
{
  m_pos = PIXY_RCS_CENTER_POS;
  m_pgain = pgain;
  m_dgain = dgain;
  m_prevError = 0x80000000L;
}

ServoLoop panLoop(350, 700);
ServoLoop tiltLoop(550, 700);

void ServoLoop::setgain(int32_t pgain, int32_t dgain)
{
  m_pgain = pgain;
  m_dgain = dgain;
}

void ServoLoop::updatepos(int32_t error)
{
  long int vel;
  if (m_prevError != 0x80000000)
  {
    vel = (error * m_pgain + (error - m_prevError) * m_dgain) >> 10;

    m_pos += vel;
    if (m_pos > PIXY_RCS_MAX_POS)
      m_pos = PIXY_RCS_MAX_POS;
    else if (m_pos < PIXY_RCS_MIN_POS)
      m_pos = PIXY_RCS_MIN_POS;
  }
  m_prevError = error;
}

void pantilt() {
  blocks = pixy.getBlocks();
  if (blocks) {
    panError = X_CENTER - pixy.blocks[0].x;
    tiltError = pixy.blocks[0].y - Y_CENTER;

    //double area = pixy.blocks[0].width * pixy.blocks[0].height;
    double width = pixy.blocks[0].width;
    double w =  width * 1.0 / 319 * 255;
    double r = width * 1.2 / 100;
    double d = 1 / r;

    if (d < 1.2) {
      panLoop.setgain(350 * d, 600 * d);
      tiltLoop.setgain(500 * d, 700 * d);
    } else {
      panLoop.setgain(350, 600);
      tiltLoop.setgain(550, 700);
    }

    pixy.setLED(w, w, w);

    panLoop.updatepos(panError);
    tiltLoop.updatepos(tiltError);

    pixy.setServos(panLoop.m_pos, tiltLoop.m_pos);
    //pixy.setServos(panLoop.m_pos, tiltLoop.m_pos);
  }//if block end
}//pantilt end

void BuzzerTone(double freq, double timelast, double volumn) {
  int Interval = 1000000 /  freq;
  int HighTime = Interval * volumn;
  int LowTime = Interval * (1 - volumn);
  int n = timelast * 1000000 / Interval;
  for (int i = 0; i < n; i++) {
    PORTB |=  B01000000;  //10
    delayMicroseconds(HighTime);
    PORTB &= ~B01000000;  //10
    delayMicroseconds(LowTime);
  }
  delay(100);
}

void sendmail(String ONnote, String OffNote, bool Notestatus) {
  p.begin(F("/mnt/sda1/mail.py"));   // Process that launch the  command
  if (Notestatus) {
    p.addParameter(ONnote); // pass  parameter
  } else {
    p.addParameter(OffNote); // pass  parameter
  }
  p.runAsynchronously();      // Run the process and wait for its termination
}

void setup() {
  Bridge.begin();
  Serial.begin(9600);
  client.begin("broker.shiftr.io");
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);
  pinMode(ledPin4, OUTPUT);
  pinMode(buzzer, OUTPUT);

  connect();
  pixy.init(); //PIXY
}

void connect() {
  Serial.print(F("connecting..."));
  while (!client.connect("arduino", "36f92922", "6587a1b1f2244682")) {
    Serial.print(F("."));
  }
  Serial.println(F("\nconnected!"));

  client.subscribe(F("/room1/ctrl_LED"));
  client.subscribe(F("/room1/ctrl_SAFEBELL"));
  client.subscribe(F("/room1/ctrl_AIR"));
  client.subscribe(F("/room1/ctrl_SMARTVISION"));
  client.subscribe(F("/room1/ctrl_LAMP"));
  client.subscribe(F("/room1/ctrl_ROBOTX"));
  client.subscribe(F("/room1/ctrl_ROBOTY"));
  client.subscribe(F("/room1/ctrl_WEBFLASH"));
  // client.unsubscribe("/example");
}

void intPublish(String location, int input) {
  char output[10];
  sprintf(output, "%d", input);
  client.publish(location, output);
}

void intPublish(String location, int input, bool enable) {
  if (enable) {
    char output[10];
    sprintf(output, "%d", input);
    client.publish(location, output);
  }
}

String wifiquality () {
  Process wifiCheck;  // initialize a new process
  wifiCheck.runShellCommand(F("/usr/bin/wifi-info.lua"));  // command you want to run
  String parse = "";
  while (wifiCheck.available() > 0) {
    parse = parse + String(char(wifiCheck.read()));
  }
  return parse;
}

void loop() {
  loopcnt++;
  if (!VisionStatus) {
    client.loop();
  } else if (loopcnt % 50 == 0) {
    client.loop();
  }
  if (!client.connected()) {
    connect();
  }
  // publish a message roughly every second.
  if (!VisionStatus && millis() - lastMillis > 2500) {
    lastMillis = millis();
    intPublish(F("/room1/temp"), dht.readTemperature());
    intPublish(F("/room1/humid"), dht.readHumidity());
    intPublish(F("/room1/WIFI"), wifiquality().toInt());
  }
  if (VisionStatus && millis() - lastMillis2 > 1) {
    lastMillis2 = millis();
    pantilt();
  }
}

void messageReceived(String topic, String payload, char * bytes, unsigned int length) {
  //Serial.print("incoming: ");
  //Serial.print(topic);
  //Serial.print(" - ");
  //Serial.print(payload);
  //Serial.println();

  if (topic.endsWith(F("1/ctrl_LED")) && payload == F("switch")) {
    PORTF = (~PORTF & B01000000) | (PORTF & B10111111);
    intPublish(F("/room1/ctrl_LED"), digitalRead(ledPin2));
    //updateonce = B0;
    //Serial.println(updateonce);
  } else if (topic.endsWith(F("1/ctrl_AIR")) && payload == F("switch")) {
    PORTF = (~PORTF & B10000000) | (PORTF & B01111111);
    intPublish(F("/room1/ctrl_AIR"), digitalRead(ledPin1));
    //updateonce = B0;
    IRsend irsend;
    irsend.sendRaw(rawData, rawsize, khz);
    irsend.sendRaw(rawData, rawsize, khz);
    sendmail(F("冷氣目前是開著的"), F("冷氣目前已經被關閉"), digitalRead(ledPin1));
  } else if (topic.endsWith(F("1/ctrl_SMARTVISION")) && payload == F("switch")) {
    VisionStatus = VisionStatus ^ B1;
    intPublish(F("/room1/ctrl_SMARTVISION"), VisionStatus);
    //updateonce = B0;
    pixy.setServos(500, 500);
    pixy.setServos(500, 500);
    intPublish(F("/room1/ctrl_ROBOTX"), 500);
    intPublish(F("/room1/ctrl_ROBOTY"), 500);
  } else if (topic.endsWith(F("1/ctrl_ROBOTX"))) {
    robotx = payload.toInt();
    pixy.setServos(robotx, roboty);
    pixy.setServos(robotx, roboty);
    pixy.setServos(robotx, roboty);
  } else if (topic.endsWith(F("1/ctrl_ROBOTY"))) {
    roboty = payload.toInt();
    pixy.setServos(robotx, roboty);
    pixy.setServos(robotx, roboty);
    pixy.setServos(robotx, roboty);
  } else if (topic.endsWith(F("1/ctrl_LAMP"))) {
    lampLui = payload.toInt();
    analogWrite(ledPin4, lampLui * 25);
  } else if (topic.endsWith(F("1/ctrl_SAFEBELL")) && payload == F("switch")) {
    PORTC = (~PORTC & B10000000) | (PORTC & B01111111);
    intPublish(F("/room1/ctrl_SAFEBELL"), digitalRead(ledPin3));
    //updateonce = B0;
    if (digitalRead(ledPin3)) {
      BuzzerTone(1046, 0.7, 0.4);
      BuzzerTone(1318, 0.7, 0.4);
      BuzzerTone(1568, 0.7, 0.4);
      BuzzerTone(1318, 0.7, 0.4);
      BuzzerTone(1046, 0.7, 0.4);
    } else {
      BuzzerTone(1175, 0.7, 0.4);
      BuzzerTone(1397, 0.7, 0.4);
      BuzzerTone(1760, 0.7, 0.4);
      BuzzerTone(1397, 0.7, 0.4);
      BuzzerTone(1175, 0.7, 0.4);
    }
  } else if (topic.endsWith(F("1/ctrl_WEBFLASH"))) {
    intPublish(F("/room1/ctrl_ROBOTX"), robotx);
    intPublish(F("/room1/ctrl_ROBOTY"), roboty);
    intPublish(F("/room1/ctrl_LED"), digitalRead(ledPin2));
    intPublish(F("/room1/ctrl_SAFEBELL"), digitalRead(ledPin3));
    intPublish(F("/room1/ctrl_SMARTVISION"), VisionStatus);
    intPublish(F("/room1/ctrl_AIR"), digitalRead(ledPin1));
    intPublish(F("/room1/ctrl_LAMP"), lampLui);
    //updateonce = B1;
  }
}
