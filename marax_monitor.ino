
//NodeMCU 1.0 (ESP-12E)
#define PIN_NEXT_RX   D7          // Serial TX Nextion Display
#define PIN_NEXT_TX   D1          // Serial RX Nextion Display
#define PIN_PWR_BTN   D2          // Power Button
#define PIN_PUMP      D3          // Reed switch                                                                                                             
#define PIN_MARA_RX   D5          // Serial TX MaraX
#define PIN_MARA_TX   D6          // Serial RX MaraX
#define PIN_PWR       D0          // Power Drive (set LOW to power off)
#define PWR_OFF_DELAY 600         // Hold button 0.6 sec to power off
#define PWR_OFF_AUTO  3600000     // Auto Power Off after 1 hour

#include "config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <arduino-timer.h>
#include <SoftwareSerial.h>
#include <ArduinoOTA.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);
SoftwareSerial maraSerial(PIN_MARA_RX, PIN_MARA_TX);
SoftwareSerial nextSerial(PIN_NEXT_RX, PIN_NEXT_TX);
auto timer = timer_create_default(); 

byte timerSeconds = 0;
bool timerStarted = false;
long timerStartMillis = 0;
long timerStopMillis = 0;
long displayUpdateMillis = 0;
long autoPowerOffMillis = 0; 

const byte numChars = 32;
char receivedChars[numChars];

boolean newData = false;

//Data sent by MaraX, e.g. C1.19,116,124,095,0560,0
enum { mode_version, steam_temp, steam_target, hx_temp, boost, heater, pump };

void setup() {

  pinMode(PIN_PWR, OUTPUT);
  digitalWrite(PIN_PWR, HIGH);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  //disable to save energy

  pinMode(PIN_PWR_BTN, INPUT);
  pinMode(PIN_PUMP, INPUT_PULLUP);

  Serial.begin(9600);
  maraSerial.begin(9600);
  nextSerial.begin(9600);

  delay(100);

  timer.every(200, shotTimer);
  timer.every(10000, getBattery);
  timer.every(100000, autoPowerOff);

  NexVisible( "nCounter", false );
  getBattery;
  autoPowerOffMillis = millis();
  
  setup_wifi();

  if ( WiFi.status() == WL_CONNECTED) {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

    ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else { // U_FS
        type = "filesystem";
      }

      // NOTE: if updating FS this would be the place to unmount FS using FS.end()
      Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });
    ArduinoOTA.begin();
  }
}

void loop() {

  timer.tick();

  detectPump();

  receiveData();

  if (newData == true) { 
    newData = false; 
    if (millis() - displayUpdateMillis > 5000) { 
      parseData();
      displayUpdateMillis = millis();
    }
  }
  if ( millis() - displayUpdateMillis > 10000 ) {
    //machine was switched off
    strncpy(receivedChars, "0,0,0,0,0,0,0", numChars);
    parseData();
    delay(5000);
  } 

  detectPowerbutton();

  if ( WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      mqttReconnect();
    }
    ArduinoOTA.handle();
    mqttClient.loop();          //MQTT
  }
}

void receiveData() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker1 = 'C';  //Coffee mode
  char startMarker2 = 'V';  //Vapor mode
  char endMarker = '\n';
  char rc;

  while (maraSerial.available() > 0 && newData == false) {
    rc = maraSerial.read();
    delay(5);

    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      } else {
        //add pump status
        ndx--;
        receivedChars[ndx] = ',';
        ndx++;
        if (timerStarted) {
          receivedChars[ndx] = '1';
        } else {
          receivedChars[ndx] = '0';
        }
        ndx++;
        receivedChars[ndx] = '\0';
        recvInProgress = false;
        ndx = 0;
        newData = true;
        Serial.println( receivedChars );
      }
    } else if (rc == startMarker1 || rc == startMarker2) {
      receivedChars[0] = rc;
      ndx++;
      recvInProgress = true;
    }
  }
}

void parseData() {

  char copy[numChars];
  char sep[] = ",";
  char* token;
  int var;
  int input[5];
  int i = 0;
  byte data[7];

  strcpy(copy, receivedChars);

  token = strtok (copy, sep);
  while (token != NULL)
  {
    sscanf (token, "%d", &var);
    data[i++] = var;
    token = strtok (NULL, sep);
  }

  if ( WiFi.status() == WL_CONNECTED) {
    mqttClient.publish(MQTT_TOPIC, receivedChars);
  }

  // y = 2x - 140 (for 3.2" display)
  NexWave( 1, 0, 2 * data[hx_temp] - 140 );
  NexWave( 1, 1, 2 * data[steam_temp] - 140 );
  NexWave( 1, 2, 2 * data[steam_target] - 140 );

  if (timerStarted) {
    NexWave( 1, 3, 15 );
  } else {
    NexWave( 1, 3, 1 );
  }

  NexNumber( "nHX", data[hx_temp] );
  NexNumber( "nSteam", data[steam_temp] );
  NexNumber( "nTarget", data[steam_target] );
  NexNumber( "nShot", timerSeconds );

  NexNumber( "nBoost", data[boost] );

  if(data[heater]) {
    NexCircle("RED");
  } else {
    NexCircle("WHITE");
  }
}

void detectPump() {
  if (!timerStarted && !digitalRead(PIN_PUMP)) {
    timerStartMillis = millis();
    timerStarted = true;

    NexVisible( "sWave", false );
    NexVisible( "nCounter", true );
    Serial.println("Start pump");
  }
  if (timerStarted && digitalRead(PIN_PUMP)) {
    if (timerStopMillis == 0) {
      timerStopMillis = millis();
    }
    if (millis() - timerStopMillis > 500) {
      timerStarted = false;
      timerStopMillis = 0;

      NexVisible( "nCounter", false );
      NexVisible( "sWave", true );

      Serial.println("Stop pump");
    }
  } else {
    timerStopMillis = 0;
  }
}

void detectPowerbutton() {
  long unsigned int start;

  if (digitalRead(PIN_PWR_BTN)) {
    start = millis();
    while (digitalRead(PIN_PWR_BTN)) {
      Serial.println("Button pressed");
      if (millis() > (start + PWR_OFF_DELAY)) {
        Serial.println("Power off");
        while (digitalRead(PIN_PWR_BTN)) {}
        delay(150);
        digitalWrite(PIN_PWR, LOW);
      }
      if (millis() > (start + 100)) {
        //reset auto power counter off when short push
        autoPowerOffMillis = millis();  
      }
    }
  }
}

boolean autoPowerOff(void *) {
  if (millis() - autoPowerOffMillis > PWR_OFF_AUTO) { 
        digitalWrite(PIN_PWR, LOW);
  }
  return true;
}

boolean shotTimer(void *) {
  if (timerStarted) {
    timerSeconds = round((millis() - timerStartMillis) / 1000.0 );
    NexNumber( "nCounter", timerSeconds );
    NexNumber( "nShot", timerSeconds );
  }
  return true;
}

boolean getBattery(void *) {
  unsigned int average = 0;
  for (int i = 0; i < 10; i++)  {
    average += analogRead(A0);
  }
  average = average / 10;
  
  NexNumber( "nBat", (int)round( average * 100.0 / 244 - 65000.0 / 244 ) );
  return true;
}

void NexWave( byte id, byte ch, int val) {

  (val < 0) && ( val = 0 );
  
  String cmd = "add ";
  cmd += id;
  cmd += ",";
  cmd += ch;
  cmd += ",";
  cmd += val;
  nextSerial.print(cmd);
  nextSerial.write(0xff);
  nextSerial.write(0xff);
  nextSerial.write(0xff);
}

void NexNumber( char* id, byte number) {
  String cmd = id;
  char val[10] = {0};

  utoa(number, val, 10);

  cmd += ".val=";
  cmd += val;
  nextSerial.print(cmd);
  nextSerial.write(0xff);
  nextSerial.write(0xff);
  nextSerial.write(0xff);
}

void NexVisible( char* id, bool visibility ) {

  String cmd = "vis ";
  cmd += id;
  cmd += ",";

  if (visibility) {
    cmd += "1";
  } else {
    cmd += "0";
  }

  nextSerial.print(cmd);
  nextSerial.write(0xff);
  nextSerial.write(0xff);
  nextSerial.write(0xff);
}

void NexCircle( char* color ) {

  String cmd = "cirs 359,44,8,";
  cmd += color;
  
  nextSerial.print(cmd);
  nextSerial.write(0xff);
  nextSerial.write(0xff);
  nextSerial.write(0xff);
}

void setup_wifi() {
  short timeout = 0;
  delay(10);
  Serial.println();

  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PWD);

  while (WiFi.status() != WL_CONNECTED) {
    timeout++;
    if (timeout > 10) {
      Serial.println("Can't connect to WIFI");
      return;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqttReconnect() {
  while (!mqttClient.connected()) {
    Serial.println("Reconnecting MQTT...");
    if (!mqttClient.connect("mqttClient")) {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 2 seconds");
      delay(2000);
    }
  }
  Serial.println("MQTT Connected...");
}
