
#ifdef ENABLE_DEBUG
       #define DEBUG_ESP_PORT Serial
       #define NODEBUG_WEBSOCKETS
       #define NDEBUG
#endif 

#include <Arduino.h>
#ifdef ESP8266 
       #include <ESP8266WiFi.h>
#endif 
#ifdef ESP32   
       #include <WiFi.h>
#endif

#include "SinricPro.h"
#include "SinricProLight.h"


#define WIFI_SSID         ""    
#define WIFI_PASS         ""
#define APP_KEY           ""      // Should look like "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx"
#define APP_SECRET        ""   // Should look like "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx"
#define LIGHT_ID            ""
#define BAUD_RATE         9600                // Change baudrate to your need

#include "hw_timer.h" 

const byte zcPin = 12;
const byte pwmPin = 13;


byte fade = 1;
byte state = 1;
byte tarBrightness = 200;
byte curBrightness = 0;
byte zcState = 0; // 0 = ready; 1 = processing;


//bool cState = false, pState = false;
int cbrightness = 0;


bool onPowerState(const String &deviceId, bool &cstate) {
  Serial.printf("-> Device: Light | Device ID: %s power turned %s \r\n", deviceId.c_str(), cstate?"on":"off");
  Serial.println(cstate);
  if(cstate) {
    fade = 1;
    state = 1;
    curBrightness = 0;
    tarBrightness = 200;
    //tarBrightness = 200;
    //hw_timer_init(NMI_SOURCE, 0);
    hw_timer_set_func(dimTimerISR);
    }
  else {
    
    tarBrightness = 0;
  }
  return true; // request handled properly
}


bool onBrightness(const String &deviceId, int &brightness) {
  cbrightness = brightness;
  tarBrightness = map(brightness, 0, 100, 0, 255);
  Serial.printf("Device %s brightness level changed to %d\r\n", deviceId.c_str(), brightness);
  return true;
}



void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  IPAddress localIP = WiFi.localIP();
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %d.%d.%d.%d\r\n", localIP[0], localIP[1], localIP[2], localIP[3]);
}

void setupSinricPro() {
  // get a new Light device from SinricPro
  SinricProLight &myLight = SinricPro[LIGHT_ID];

  // set callback function to device
  myLight.onPowerState(onPowerState);
  myLight.onBrightness(onBrightness);

  // setup SinricPro
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
}

void ICACHE_RAM_ATTR zcDetectISR() {
  if (zcState == 0) {
    zcState = 1;
  
    if (curBrightness < 255 && curBrightness > 0) {
      digitalWrite(pwmPin, 0);
      
      int dimDelay = 30 * (255 - curBrightness) + 400;//400
      hw_timer_arm(dimDelay);
    }
  }
}



void setup() {
  Serial.begin(BAUD_RATE); Serial.printf("\r\n\r\n");
  //setupColorTemperatureIndex(); // setup our helper map
  setupWiFi();
  setupSinricPro();
  pinMode(zcPin, INPUT_PULLUP);
  pinMode(pwmPin, OUTPUT);

  attachInterrupt(zcPin, zcDetectISR, RISING);    // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
  hw_timer_init(NMI_SOURCE, 0);
  hw_timer_set_func(dimTimerISR);

}

void loop() {
  // put your main code here, to run repeatedly:
    /*if (Serial.available()){
        int val = Serial.parseInt();
        if (val>0){
          tarBrightness =val;
          Serial.println(tarBrightness);
        }
        
    }*/
    SinricPro.handle();
    /*if(cState != pState){
      digitalWrite(pwmPin, cState);
      if(!cState) tarBrightness = 0;

      pState = cState;
      }*/
    
}


void dimTimerISR() {
    if (fade == 1) {
      if (curBrightness > tarBrightness || (state == 0 && curBrightness > 0)) {
        --curBrightness;
      }
      else if (curBrightness < tarBrightness && state == 1 && curBrightness < 255) {
        ++curBrightness;
      }
    }
    else {
      if (state == 1) {
        curBrightness = tarBrightness;
      }
      else {
        curBrightness = 0;
      }
    }
    
    if (curBrightness == 0) {
      state = 0;
      digitalWrite(pwmPin, 0);
    }
    else if (curBrightness == 255) {
      state = 1;
      digitalWrite(pwmPin, 1);
    }
    else {
      digitalWrite(pwmPin, 1);
    }
    
    zcState = 0;
}
