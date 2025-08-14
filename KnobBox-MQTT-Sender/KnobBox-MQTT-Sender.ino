// Arduino and KY-040 module
#include <WiFi.h>
#include <MQTT.h>
#include "Controller.h"
#include "arduino_secrets.h"

const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;

const char * clientName = "KnobBoxSender";
const uint16_t port = 1883;
const char * server = "192.168.1.29";
String selectorMovementTopic = "layout/selectormovement/";
String speedMovementTopic = "layout/speedmovement/";

Controller speedKnob(4, 5, 6);
Controller selectKnob(7, 15, 16);

WiFiClient wifiClient;
MQTTClient mqttClient;

void setup() {
  Serial.begin (115200);
  WiFi.begin(ssid, password);

  int tryDelay = 500;
  int numberOfTries = 20;

  // Wait for the WiFi event
  bool isConnected = false;

  Serial.println("Start");

  speedKnob.GetMovement();
  selectKnob.GetMovement();

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
  mqttClient.begin(server, wifiClient);
  mqttClient.setKeepAlive(360);
  mqttClient.setTimeout(360);
  connect();
}

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("connecting...");

  while (!mqttClient.connect(clientName)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");
}

void loop() {
  mqttClient.loop();
  if (!mqttClient.connected()) {
    Serial.println("MQTT client disc");
    connect();
  }

  String speedKnobMoved = speedKnob.GetMovement();
  if (speedKnobMoved != "") {
    Serial.println("Speed " + speedKnobMoved);
    mqttClient.publish(speedMovementTopic, speedKnobMoved, false, 0);
    //delay(50);
  }

  String selectorKnobMoved = selectKnob.GetMovement();
  if (selectorKnobMoved != "") {
    Serial.println("Select " + selectorKnobMoved);
    mqttClient.publish(selectorMovementTopic, selectorKnobMoved, false, 0);
    delay(50);
  }

  //speedKnob.CheckMovement();
  //Serial.println(String(speedKnob.InternalCount));
  //delay(50);
}
