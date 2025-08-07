// Arduino and KY-040 module
#include <WiFi.h>
#include "Controller.h"

const char* ssid     = "BTHub4-C5Q8";
const char* password = "db6d389eb9";

Controller speedKnob(4, 5, 6);
//Controller selectorKnob;
WiFiClient wifiClient;


void setup() {
  Serial.begin (115200);
  WiFi.begin(ssid, password);
  //speedKnob = Controller(34, 35, 32);

  int tryDelay = 500;
  int numberOfTries = 20;

  // Wait for the WiFi event
  bool isConnected = false;

  Serial.println("Start");

  while (!isConnected) {

    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL:
        Serial.println("[WiFi] SSID not found");
        break;
      case WL_CONNECT_FAILED:
        Serial.print("[WiFi] Failed - WiFi not connected! Reason: ");
        return;
        break;
      case WL_CONNECTION_LOST:
        Serial.println("[WiFi] Connection was lost");
        break;
      case WL_SCAN_COMPLETED:
        Serial.println("[WiFi] Scan is completed");
        break;
      case WL_DISCONNECTED:
        Serial.println("[WiFi] WiFi is disconnected");
        break;
      case WL_CONNECTED:
        Serial.println("[WiFi] WiFi is connected!");
        Serial.print("[WiFi] IP address: ");
        Serial.println(WiFi.localIP());

        isConnected = true;
        break;
      default:
        Serial.print("[WiFi] WiFi Status: ");
        Serial.println(WiFi.status());
        break;
    }
    delay(tryDelay);

    if (numberOfTries <= 0) {
      Serial.print("[WiFi] Failed to connect to WiFi!");
      // Use disconnect function to force stop trying to connect
      WiFi.disconnect();
      return;
    } else {
      numberOfTries--;
    }
  }

}



void loop() {
  
  String skMoved = speedKnob.GetMovement();
  if (skMoved != "") {
    Serial.println(skMoved);
    delay(50);
  }
  
  //speedKnob.CheckMovement();
  //Serial.println(String(speedKnob.InternalCount));
  //delay(50);
}
