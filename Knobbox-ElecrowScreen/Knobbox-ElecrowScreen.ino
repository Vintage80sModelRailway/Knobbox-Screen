#include <WiFi.h>
#include <MQTT.h>
#include "RosterEntry.h"
#include "arduino_secrets.h"
#include "Throttle.h"
#include "ScreenButton.h"
#include "gfx_conf.h"

#define NUMBER_OF_THROTTLES 6
#define NUMBER_OF_BUTTONS 21
#define ROSTER_SIZE 30
#define TFT_GREY 0x5AEB
#define RED2RED 0
#define GREEN2GREEN 1
#define BLUE2BLUE 2
#define BLUE2RED 3
#define GREEN2RED 4
#define RED2GREEN 5

#define METERXOFFSET 26
#define METERYOFFSET 5

int32_t lastTouchX;
int32_t lastTouchY;
String prefix;
String locoName;
String delimeter = "";
String receivedLine = "";
int locoArrayCounter = 0;
int rosterCounter = 0;
int maxFunctions = 68;
bool rosterCollecting = false;
bool actionCollecting = false;
bool trackPowerCollecting = false;
bool trackPowerState = false;
String actionReceivedLine = "";
String actionControllerIndex = "";
int numberOfLocosInRoster = -1;
bool screenPressDetected = false;
RosterEntry currentLoco;
RosterEntry roster[ROSTER_SIZE];

const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;
const char * clientName = "KnobBoxTFT";
const uint16_t port = 1883;
const char * server = "192.168.1.29";
String cabSignalTopic = "layout/cabsignal/";
String locoMovementTopic = "layout/locomovement/";
String selectorMovementTopic = "layout/selectormovement/";
String speedMovementTopic = "layout/speedmovement/";

WiFiClient wifiClient;
WiFiClient wifiClientMQTT;
MQTTClient mqttClient;

Throttle throttles[NUMBER_OF_THROTTLES] = {
  Throttle("1", 0, 0, 160, 160),
  Throttle("2", 160, 0, 320, 160),
  Throttle("3", 0, 160, 160, 320),
  Throttle("4", 160, 160, 320, 320),
  Throttle("5", 0, 320, 160, 480),
  Throttle("6", 160, 320, 360, 480)
};

ScreenButton fButtons[NUMBER_OF_BUTTONS] = {
  ScreenButton("Track power", 600, 0, 780, 40, TFT_GREEN, TFT_WHITE, TFT_BLACK),
  ScreenButton("Stop all trains", 600, 60, 780, 100, TFT_RED, TFT_WHITE, TFT_BLACK),
  ScreenButton("F0", 580, 130, 640, 170, TFT_WHITE, TFT_WHITE, TFT_BLACK, 0),
  ScreenButton("F1", 650, 130, 710, 170, TFT_WHITE, TFT_WHITE, TFT_BLACK, 1),
  ScreenButton("F2", 720, 130, 780, 170, TFT_WHITE, TFT_WHITE, TFT_BLACK, 2),
  ScreenButton("F3", 580, 190, 640, 230, TFT_WHITE, TFT_WHITE, TFT_BLACK, 3),
  ScreenButton("F4", 650, 190, 710, 230, TFT_WHITE, TFT_WHITE, TFT_BLACK, 4),
  ScreenButton("F5", 720, 190, 780, 230, TFT_WHITE, TFT_WHITE, TFT_BLACK, 5),
  ScreenButton("F6", 580, 250, 640, 290, TFT_WHITE, TFT_WHITE, TFT_BLACK, 6),
  ScreenButton("F7", 650, 250, 710, 290, TFT_WHITE, TFT_WHITE, TFT_BLACK, 7),
  ScreenButton("F8", 720, 250, 780, 290, TFT_WHITE, TFT_WHITE, TFT_BLACK, 8),
  ScreenButton("F9", 580, 310, 640, 350, TFT_WHITE, TFT_WHITE, TFT_BLACK, 9),
  ScreenButton("F10", 650, 310, 710, 350, TFT_WHITE, TFT_WHITE, TFT_BLACK, 10),
  ScreenButton("F11", 720, 310, 780, 350, TFT_WHITE, TFT_WHITE, TFT_BLACK, 11),
  ScreenButton("F12", 580, 370, 640, 410, TFT_WHITE, TFT_WHITE, TFT_BLACK, 12),
  ScreenButton("F13", 650, 370, 710, 410, TFT_WHITE, TFT_WHITE, TFT_BLACK, 13),
  ScreenButton("F14", 720, 370, 780, 410, TFT_WHITE, TFT_WHITE, TFT_BLACK, 14),
  ScreenButton("F15", 580, 430, 640, 470, TFT_WHITE, TFT_WHITE, TFT_BLACK, 15),
  ScreenButton("F16", 650, 430, 710, 470, TFT_WHITE, TFT_WHITE, TFT_BLACK, 16),
  ScreenButton("F17", 720, 430, 780, 470, TFT_WHITE, TFT_WHITE, TFT_BLACK, 17),
  ScreenButton("Release", 360, 400, 540, 440, TFT_RED, TFT_WHITE, TFT_BLACK, 999)
};

void setup() {
  Serial.begin (19200);
  lcd.init();
  lcd.fillScreen(TFT_BLACK);
  WiFi.begin(ssid, password);

  int tryDelay = 500;
  int numberOfTries = 20;
  bool isConnected = false;

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
        Serial.println("[WiFi] WiFi connected!");
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
      WiFi.disconnect();
      return;
    } else {
      numberOfTries--;
    }
  }

  mqttClient.begin(server, wifiClientMQTT);
  mqttClient.setKeepAlive(360);
  mqttClient.setTimeout(360);
  mqttClient.onMessage(messageReceived);
  connect();

  if (!wifiClient.connect(IPAddress(192, 168, 1, 29), 12090)) {
    Serial.println("Connection to host failed");
  }
  else {
    wifiClient.write("NKnobBoxTFT70\n");
    Serial.println("Connected to WiiThrottle");
  }
  prefix = "";
  locoName = "";

  for (int i = 0; i < NUMBER_OF_THROTTLES; i++) {
    clearThrottle(i);
  }
  drawPermanentButtons();
}

void setPowerButton(bool isOn) {
  int powerButtonIndex = -1;
  for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
    if (fButtons[i].buttonText == "Track power") {
      powerButtonIndex = i;
      break;
    }
  }

  if (powerButtonIndex == -1) return;

  if (isOn) {
    lcd.fillRect(fButtons[powerButtonIndex].tftX, fButtons[powerButtonIndex].tftY, (fButtons[powerButtonIndex].tftXEnd - fButtons[powerButtonIndex].tftX), (fButtons[powerButtonIndex].tftYEnd - fButtons[powerButtonIndex].tftY), fButtons[powerButtonIndex].boxColour);
    lcd.setTextColor(fButtons[powerButtonIndex].selectedTextColour, fButtons[powerButtonIndex].boxColour);
    int middle = (fButtons[powerButtonIndex].tftXEnd - fButtons[powerButtonIndex].tftX) / 2;
    lcd.drawCentreString(fButtons[powerButtonIndex].buttonText, fButtons[powerButtonIndex].tftX + middle, fButtons[powerButtonIndex].tftY + 10, 4);
  }
  else {
    lcd.fillRect(fButtons[powerButtonIndex].tftX, fButtons[powerButtonIndex].tftY, (fButtons[powerButtonIndex].tftXEnd - fButtons[powerButtonIndex].tftX), (fButtons[powerButtonIndex].tftYEnd - fButtons[powerButtonIndex].tftY), TFT_BLACK);
    lcd.drawRect(fButtons[powerButtonIndex].tftX, fButtons[powerButtonIndex].tftY, (fButtons[powerButtonIndex].tftXEnd - fButtons[powerButtonIndex].tftX), (fButtons[powerButtonIndex].tftYEnd - fButtons[powerButtonIndex].tftY), fButtons[powerButtonIndex].boxColour);
    lcd.setTextColor(fButtons[powerButtonIndex].textColour, TFT_BLACK);
    int middle = (fButtons[powerButtonIndex].tftXEnd - fButtons[powerButtonIndex].tftX) / 2;
    lcd.drawCentreString(fButtons[powerButtonIndex].buttonText, fButtons[powerButtonIndex].tftX + middle, fButtons[powerButtonIndex].tftY + 10, 4);
  }
}

void drawPermanentButtons() {
  for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
    Serial.println("Button 1 is function " + String(fButtons[i].isFunction));
    if (!fButtons[i].isFunction) {
      lcd.drawRect(fButtons[i].tftX, fButtons[i].tftY, (fButtons[i].tftXEnd - fButtons[i].tftX), (fButtons[i].tftYEnd - fButtons[i].tftY), fButtons[i].boxColour);
      lcd.setTextColor(fButtons[i].textColour, TFT_BLACK);
      int middle = (fButtons[i].tftXEnd - fButtons[i].tftX) / 2;
      lcd.drawCentreString(fButtons[i].buttonText, fButtons[i].tftX + middle, fButtons[i].tftY + 10, 4);
    }
  }
}

void drawFunctionButtons(int throttleIndex) {

  int fButtonIndex = -1;
  for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
    //Serial.println("Button 1 is function " + String(fButtons[i].isFunction));
    if (fButtons[i].isFunction && throttles[throttleIndex].isSelected) {
      fButtonIndex++;
      bool state = roster[throttles[throttleIndex].rosterIndex].functionState[fButtonIndex];
      Serial.println("Button " + String(fButtonIndex) + " function state " + String(state));
      if (state == false) {
        lcd.fillRect(fButtons[i].tftX, fButtons[i].tftY, (fButtons[i].tftXEnd - fButtons[i].tftX), (fButtons[i].tftYEnd - fButtons[i].tftY), TFT_BLACK);
        lcd.drawRect(fButtons[i].tftX, fButtons[i].tftY, (fButtons[i].tftXEnd - fButtons[i].tftX), (fButtons[i].tftYEnd - fButtons[i].tftY), fButtons[i].boxColour);
        lcd.setTextColor(fButtons[i].textColour, TFT_BLACK);
        int middle = (fButtons[i].tftXEnd - fButtons[i].tftX) / 2;
        lcd.drawCentreString(fButtons[i].buttonText, fButtons[i].tftX + middle, fButtons[i].tftY + 10, 4);
      }
      else {
        lcd.fillRect(fButtons[i].tftX, fButtons[i].tftY, (fButtons[i].tftXEnd - fButtons[i].tftX), (fButtons[i].tftYEnd - fButtons[i].tftY), fButtons[i].boxColour);
        lcd.setTextColor(fButtons[i].selectedTextColour, fButtons[i].boxColour);
        int middle = (fButtons[i].tftXEnd - fButtons[i].tftX) / 2;
        lcd.drawCentreString(fButtons[i].buttonText, fButtons[i].tftX + middle, fButtons[i].tftY + 10, 4);
      }
    }
    else if (!fButtons[i].isFunction) {
      lcd.drawRect(fButtons[i].tftX, fButtons[i].tftY, (fButtons[i].tftXEnd - fButtons[i].tftX), (fButtons[i].tftYEnd - fButtons[i].tftY), fButtons[i].boxColour);
      lcd.setTextColor(fButtons[i].textColour, TFT_BLACK);
      int middle = (fButtons[i].tftXEnd - fButtons[i].tftX) / 2;
      lcd.drawCentreString(fButtons[i].buttonText, fButtons[i].tftX + middle, fButtons[i].tftY + 10, 4);
    }
  }
}

void drawSelectedThrottle (int i) {
  Serial.println("Draw selected current dir " + String(roster[throttles[i].rosterIndex].currentDirection));
  int value = throttles[i].speedStep;
  if (value < 0) value = 0;
  if (roster[throttles[i].rosterIndex].currentDirection < 1)
    ringMeter(value, 0, 128, 345, 180, 104, "R" , GREEN2RED);
  else
    ringMeter(value, 0, 128, 345, 180, 104, "F" , GREEN2RED);
}

void drawSelectedThrottleSignal(int i) {
  if (throttles[i].rosterIndex < 0) return;
  char aspect = throttles[i].signalAspect;
  uint16_t colour = 0;
  switch (aspect) {
    case 'P':
      colour = TFT_GREEN;
      break;
    case 'C':
      colour = TFT_ORANGE;
      break;
    case 'D':
      colour = TFT_RED;
      break;
  }
  if (colour > 0)
    lcd.fillCircle(520, 130, 25, colour);
  else {
    lcd.fillCircle(520, 130, 25, TFT_BLACK);
    lcd.drawCircle(520, 130, 25, TFT_GREEN);
  }
}

void drawThrottle(int i) {
  Serial.println("Draw current dir " + String(roster[throttles[i].rosterIndex].currentDirection));
  int value = throttles[i].speedStep;
  if (value < 0) value = 0;
  if (throttles[i].isSelected == true) {
    if (roster[throttles[i].rosterIndex].currentDirection < 1) ringMeter(value, 0, 128, throttles[i].tftX + METERXOFFSET, throttles[i].tftY + METERYOFFSET, 52, "R" , GREEN2RED); // Draw analogue meter
    else ringMeter(value, 0, 128, throttles[i].tftX + METERXOFFSET, throttles[i].tftY + METERYOFFSET, 52, "F" , GREEN2RED); // Draw analogue meter
    Serial.println("Draw throttle - " + String(i) + " is selected");
    lcd.drawRect(throttles[i].tftX, throttles[i].tftY, 159, 159, TFT_RED);
  }
  else {
    Serial.println("Draw throttle - " + String(i) + " is not selected");
    if (roster[throttles[i].rosterIndex].currentDirection < 1) ringMeter(value, 0, 128, throttles[i].tftX + METERXOFFSET, throttles[i].tftY + METERYOFFSET, 52, "R" , GREEN2RED); // Draw analogue meter
    else ringMeter(value, 0, 128, throttles[i].tftX + METERXOFFSET, throttles[i].tftY + METERYOFFSET, 52, "F" , GREEN2RED); // Draw analogue meter
  }
}

void updateSelector(int currentlySelectedRosterIndex) {
  if (currentlySelectedRosterIndex < 0 || currentlySelectedRosterIndex >= ROSTER_SIZE) return;
  lcd.fillRect(320, 0, 270, 480, TFT_BLACK);
  lcd.fillRect(580, 130, 220, 350, TFT_BLACK);
  drawSelectedTrainNameCentered(560, 160, "Select loco", 4);
  drawSelectedTrainNameCentered(560, 240, roster[currentlySelectedRosterIndex].Name, 4);
  drawSelectedTrainNameCentered(560, 320, String(roster[currentlySelectedRosterIndex].Id), 4);
}

void clearThrottle(int i) {
  clearThrottleScreen(i);
  drawThrottle(i);
  //drawTrainName(throttles[i].tftX+1, throttles[i].tftY-1, "Unassigned");
  drawTrainNameCentered(throttles[i].tftX, throttles[i].tftY, "Unassigned");
  drawSignal(i);
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  String strTopic = (String)topic;
  int posLast = strTopic.lastIndexOf("/");
  int posFirst = strTopic.indexOf("/");
  String justTheID = strTopic.substring(posLast + 1);
  bool signalAspectHasChanged = false;
  for (int i = 0; i < NUMBER_OF_THROTTLES; i++) {
    if (roster[throttles[i].rosterIndex].Id == justTheID && topic == cabSignalTopic) {
      char aspect = payload.charAt(0);
      Serial.println("Found signal set throttle for " + roster[throttles[i].rosterIndex].Name + " - " + String(aspect));
      if (aspect != throttles[i].signalAspect) {
        Serial.println("Aspect changed from " + String(throttles[i].signalAspect) + " to " + String(aspect));
        throttles[i].signalAspect = aspect;
        signalAspectHasChanged = true;
        drawSignal(i);
      }
    }
    else if (topic == selectorMovementTopic && throttles[i].isSelected) {
      if (payload == "C") {
        throttles[i].currentSelectorRosterIndex++;
        updateSelector(throttles[i].currentSelectorRosterIndex);
      }
      else if (payload == "AC") {
        throttles[i].currentSelectorRosterIndex--;
        updateSelector(throttles[i].currentSelectorRosterIndex);
      }
      else if (payload = "SEL") {
        changeSelectedLoco(i);
      }
    }
    else if (topic == speedMovementTopic && throttles[i].isSelected) {
      if (payload == "C") {
        changeSpeed(i, 1);
      }
      else if (payload == "AC") {
        changeSpeed(i, -1);
      }
      else if (payload = "SEL") {
        changeSpeed(i, 0);
      }
    }
  }
}

void changeSelectedLoco(int throttleIndex) {
  //deselect train from MT
  throttles[throttleIndex].TrainName = "";
  throttles[throttleIndex].rosterIndex = -1;
  if (throttles[throttleIndex].currentSelectorRosterIndex > -1) {
    Serial.println("Select new loco detected - assign currently selected train - " + roster[throttles[throttleIndex].currentSelectorRosterIndex].Name);
    throttles[throttleIndex].TrainName = roster[throttles[throttleIndex].currentSelectorRosterIndex].Name;
    throttles[throttleIndex].signalAspect = 'X';

    if (throttles[throttleIndex].rosterIndex >= 0) {
      String rel = "M" + throttles[throttleIndex].mtIndex + "-" + roster[throttles[throttleIndex].rosterIndex].IdType + roster[throttles[throttleIndex].rosterIndex].Id + "<;>r\n";
      Serial.print("Released else Write " + rel);
      writeString(rel);
      mqttClient.unsubscribe(cabSignalTopic + roster[throttles[throttleIndex].rosterIndex].Id);
    }

    //Handle newly assigned loco
    throttles[throttleIndex].rosterIndex = throttles[throttleIndex].currentSelectorRosterIndex;
    String assign = "M" + throttles[throttleIndex].mtIndex + "+" + roster[throttles[throttleIndex].currentSelectorRosterIndex].IdType +  roster[throttles[throttleIndex].currentSelectorRosterIndex].Id + "<;>" + roster[throttles[throttleIndex].currentSelectorRosterIndex].IdType +  roster[throttles[throttleIndex].currentSelectorRosterIndex].Id + "\n";
    Serial.print("Assigned Write " + assign);

    writeString(assign);

    mqttClient.subscribe(cabSignalTopic + roster[throttles[throttleIndex].rosterIndex].Id);
    Serial.println("Sub " + cabSignalTopic + roster[throttles[throttleIndex].rosterIndex].Id);
    throttles[throttleIndex].signalAspect = 'A';
    throttles[throttleIndex].speedStep = 0;
    clearThrottleScreen(throttleIndex);
    drawThrottle(throttleIndex);
    //drawTrainName(throttles[throttleIndex].tftX, throttles[throttleIndex].tftY, throttles[throttleIndex].TrainName);
    drawTrainNameCentered(throttles[throttleIndex].tftX, throttles[throttleIndex].tftY, throttles[throttleIndex].TrainName);
    drawSignal(throttleIndex);

    lcd.fillRect(320, 0, 270, 480, TFT_BLACK);
    lcd.fillRect(580, 130, 220, 350, TFT_BLACK);

    drawSelectedThrottle(throttleIndex);
    drawSelectedTrainNameCentered(455, 30, roster[throttles[throttleIndex].rosterIndex].Name, 4);
    drawSelectedTrainNameCentered(455, 80, String(roster[throttles[throttleIndex].rosterIndex].Id), 4);
    drawSelectedThrottleSignal(throttleIndex);
    drawFunctionButtons(throttleIndex);

    roster[throttles[throttleIndex].rosterIndex].initialResponseIncoming = true;
  }
}

void changeSpeed(int throttleIndex, int speedChange) {
  if (throttleIndex < 0 || throttleIndex > NUMBER_OF_THROTTLES) return;
  if (speedChange == 0) {
    throttles[throttleIndex].speedStep = 0;
  }
  else {
    throttles[throttleIndex].speedStep = throttles[throttleIndex].speedStep + speedChange;
  }

  String spd = "M" + throttles[throttleIndex].mtIndex + "A" + roster[throttles[throttleIndex].rosterIndex].IdType + roster[throttles[throttleIndex].rosterIndex].Id + "<;>V" + String(throttles[throttleIndex].speedStep) + "\n";
  Serial.print("New speed " + String(throttles[throttleIndex].speedStep) + " writing " + spd);
  writeString(spd);
  drawThrottle(throttleIndex);
}

void loop() {

  if (!wifiClient.connected()) {
    if (!wifiClient.connect(IPAddress(192, 168, 1, 29), 12090)) {
      Serial.println("Connection to host failed");
    }
    else {
      wifiClient.write("NESP32 Knob Throttle Sig\n");
      Serial.println("Connected to WiiThrottle");
    }
  }

  while (wifiClient.available() && wifiClient.available() > 0) {
    readFromWiiTHrottle();
  }

  mqttClient.loop();
  if (!mqttClient.connected()) {
    Serial.println("MQTT client disc");
    connect();
  }

  for (int i = 0; i < NUMBER_OF_THROTTLES; i++) {
    if (throttles[i].rosterIndex >= 0 && throttles[i].rosterIndex < ROSTER_SIZE) {
      if (roster[throttles[i].rosterIndex].functionButtonUpdateRequired) {
        drawFunctionButtons(i);
        roster[throttles[i].rosterIndex].functionButtonUpdateRequired = false;
      }
    }
  }

  int32_t x, y;
  if (lcd.getTouch(&x, &y)) {
    //Serial.printf("X:%d Y:%d\n", x, y);
    //lcd.fillCircle(x, y, 15, TFT_WHITE);
    if (!screenPressDetected) {
      int throttleIndex = GetThrottleIndexFromXY(x, y);
      Serial.println("Throttle index " + String(throttleIndex));
      if (throttleIndex >= 0 && throttleIndex < NUMBER_OF_THROTTLES) {
        selectThrottle(throttleIndex);
      }
      else {
        //could be a button
        int buttonIndex = GetButtonIndexFromXY(x, y);
        if (buttonIndex >= 0) {
          Serial.println ("Button index pressed " + String(buttonIndex));
          if (!fButtons[buttonIndex].isSelected) {
            fButtons[buttonIndex].isSelected = true;
            processButtonPress(buttonIndex, true);
            //drawFunctionButtons();
          }
        }
      }
      screenPressDetected = true;
    }
  }
  else {
    //Serial.println("No touch");
    if (screenPressDetected) {
      screenPressDetected = false;
      for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
        if (fButtons[i].isSelected) {
          fButtons[i].isSelected = false;
          if (fButtons[i].isFunction) {
            processButtonPress(i, false);
          }
          
          Serial.println("Button " + String(i) + " released");

        }
      }
    }
  }
}

void processButtonPress(int buttonIndex, bool pressed) {
  Serial.println("Process button press " + String(buttonIndex) + " - " + String(pressed) + " is func " + String(fButtons[buttonIndex].isFunction));
  if (fButtons[buttonIndex].functionIndex == 999) {
    //release selected loco
  }
  else if (fButtons[buttonIndex].isFunction) {
    //function button press
    Serial.println("Is function " + String(buttonIndex));
    for (int i = 0; i < NUMBER_OF_THROTTLES; i++) {
      if (throttles[i].isSelected && throttles[i].rosterIndex >= 0) {
        String func = "M" + throttles[i].mtIndex + "A" + roster[throttles[i].rosterIndex].IdType + roster[throttles[i].rosterIndex].Id + "<;>F" + String(pressed) + String(fButtons[buttonIndex].functionIndex) + "\n";
        writeString(func);
        Serial.println("Function string " + func);
        drawFunctionButtons(throttles[i].rosterIndex);
      }
    }

  }
  else if (fButtons[buttonIndex].buttonText = "Track power") {
    //track power
    trackPowerState = !trackPowerState;
    writeString("PPA" + String(trackPowerState) + "\n"); //send power instruction to WT
    Serial.println("Track power "+String(trackPowerState));
  }
  else if (fButtons[buttonIndex].buttonText = "Stop all trains") {

  }
}

int GetThrottleIndexFromXY(int32_t x, int32_t y) {
  int throttleIndex = -1;
  for (int i = 0; i < NUMBER_OF_THROTTLES; i++) {
    //Serial.println("X "+String(x)+" Y "+String(y));
    //Serial.println(String(throttles[i].tftX)+"  "+throttles[i].tftY+" "+String(throttles[i].tftXEbd)+" "+String(throttles[i].tftYEnd));
    if (x >= throttles[i].tftX && y >= throttles[i].tftY && x < throttles[i].tftXEnd && y < throttles[i].tftYEnd) {
      throttleIndex = i;
      break;
    }
  }
  return throttleIndex;
}

int GetButtonIndexFromXY(int32_t x, int32_t y) {
  int buttonIndex = -1;
  for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
    //Serial.println("X "+String(x)+" Y "+String(y));
    //Serial.println(String(throttles[i].tftX)+"  "+throttles[i].tftY+" "+String(throttles[i].tftXEbd)+" "+String(throttles[i].tftYEnd));
    if (x >= fButtons[i].tftX && y >= fButtons[i].tftY && x < fButtons[i].tftXEnd && y < fButtons[i].tftYEnd) {
      buttonIndex = i;
      break;
    }
  }
  return buttonIndex;
}

void selectThrottle(int throttleIndex) {
  Serial.println("Starting selectThrottle - " + String(throttles[throttleIndex].isSelected) + "; " + String(throttleIndex));
  lcd.fillRect(320, 0, 270, 480, TFT_BLACK);
  lcd.fillRect(580, 130, 220, 350, TFT_BLACK);
  if (throttles[throttleIndex].isSelected == true) {
    throttles[throttleIndex].isSelected = false;
    Serial.println("throttle " + String(throttleIndex) + " unselected");
    lcd.fillRect(320, 0, 270, 480, TFT_BLACK);
    lcd.fillRect(580, 130, 220, 350, TFT_BLACK);
  }
  else {
    throttles[throttleIndex].isSelected = true;
    Serial.println("throttle " + String(throttleIndex) + " selected");
    if (throttles[throttleIndex].rosterIndex >= 0) {
      drawSelectedThrottle(throttleIndex);
      drawSelectedTrainNameCentered(455, 30, roster[throttles[throttleIndex].rosterIndex].Name, 4);
      drawSelectedTrainNameCentered(455, 80, String(roster[throttles[throttleIndex].rosterIndex].Id), 4);
      drawSelectedThrottleSignal(throttleIndex);
      drawFunctionButtons(throttleIndex);
    }
  }

  if (throttles[throttleIndex].rosterIndex >= 0) {
    clearThrottleScreen(throttleIndex);
    drawThrottle(throttleIndex);
    drawTrainNameCentered(throttles[throttleIndex].tftX, throttles[throttleIndex].tftY, throttles[throttleIndex].TrainName);
    drawSignal(throttleIndex);
  }
  else {
    clearThrottle(throttleIndex);
  }

  for (int i = 0; i < NUMBER_OF_THROTTLES; i++) {
    if (i != throttleIndex && throttles[i].isSelected == true) {
      throttles[i].isSelected = false;
      //Serial.println("loop throttle "+String(i)+" unselected");
      if (throttles[i].rosterIndex >= 0) {
        clearThrottleScreen(i);
        drawThrottle(i);
        drawTrainNameCentered(throttles[i].tftX, throttles[i].tftY, throttles[i].TrainName);
        drawSignal(i);
      }
      else {
        clearThrottle(i);
      }

    }
  }
  Serial.println("Finished selectThrottle");
}

void writeString(String stringData) { // Used to serially push out a String with Serial.write()

  for (int i = 0; i < stringData.length(); i++)
  {
    wifiClient.write(stringData[i]);   // Push each char 1 by 1 on each loop pass
  }

}// end writeString

void readFromWiiTHrottle() {
  char x = wifiClient.read();
  prefix += x;
  receivedLine += x;
  int rosterIndex = -1;
  if (trackPowerCollecting == true) {
    if (x == '\n') {
      //Serial.println(receivedLine);
      String cTrackPowerState = receivedLine.substring(3, 4);
      Serial.println("Track power message "+cTrackPowerState+" - ");
      if (cTrackPowerState == "1") {
        trackPowerState = true;
      }
      else {
        trackPowerState = false;
      }
      setPowerButton(trackPowerState);
      trackPowerCollecting = false;
    }
  }
  if (actionCollecting == true) {
    if (x == '\n') {
      //Serial.println("WT action: " + actionReceivedLine);

      int index = actionReceivedLine.indexOf("<;>");
      String firstHalf = actionReceivedLine.substring(0, index);
      // Serial.println("Firsthalf "+firstHalf);

      int addressIndex = firstHalf.indexOf("AS");
      if (addressIndex == -1) {
        addressIndex = firstHalf.indexOf("AL");
      }
      String throttleIndex = firstHalf.substring(1, addressIndex);
      int iThrottleIndex = throttleIndex.toInt();
      //Serial.println("Controller index "+controllerIndex);
      String address = firstHalf.substring(addressIndex + 1);
      //Serial.println("Address "+address);
      String secondHalf = actionReceivedLine.substring(index + 3);
      //Serial.println("Secondhalf "+secondHalf);

      String actionType = secondHalf.substring(0, 1);
      String actionInstruction = secondHalf.substring(1);

      int rosterIndexToUpdate = -1;
      for (int r = 0; r < numberOfLocosInRoster; r++) {
        String locoAddress = roster[r].IdType + roster[r].Id;
        if (locoAddress == address) {
          rosterIndexToUpdate = r;
          rosterIndex = rosterIndexToUpdate;
          //Serial.println("Matched loco " + roster[r].Name);
          break;
        }
      }
      if (actionType == "V") {
        //velocity/speed
        if (throttles[iThrottleIndex - 1].rosterIndex > -1) {
          int originalKnobPosition = throttles[iThrottleIndex - 1].speedStep;
          int newSpeed = actionInstruction.toInt();
          int diff = newSpeed -  roster[rosterIndexToUpdate].currentSpeed;
          if (diff > 5 || diff < -5)
          {
            roster[rosterIndexToUpdate].currentSpeed = actionInstruction.toInt();
            //Serial.println("Set " + roster[rosterIndexToUpdate].Name + " to speed " + String(roster[rosterIndexToUpdate].currentSpeed));
            //Serial.println("Action type " + actionType + " instruction " + actionInstruction + " controller " + String(throttleIndex));
            if (throttles[iThrottleIndex - 1].rosterIndex == rosterIndexToUpdate) {
              throttles[iThrottleIndex - 1].speedStep = roster[rosterIndexToUpdate].currentSpeed;
              Serial.println("Set active controller roster knob speed");
            }
            int diff = originalKnobPosition -  roster[rosterIndexToUpdate].currentSpeed;

            //update speed on screen?
            for (int i = 0; i < NUMBER_OF_THROTTLES; i++) {
              if (throttles[i].rosterIndex == rosterIndexToUpdate) {
                drawThrottle(i);
                drawSelectedThrottle(i);
                Serial.println("Speed update on throttle " + String(i));
              }
            }
          }
        }
      }
      else if (actionType == "R") {
        //direction
        roster[rosterIndexToUpdate].currentDirection = actionInstruction.toInt();
        Serial.println("Set " + roster[rosterIndexToUpdate].Name + " to direction " + String(roster[rosterIndexToUpdate].currentDirection));
        if (roster[rosterIndexToUpdate].initialResponseIncoming) {
          roster[rosterIndexToUpdate].initialResponseIncoming = false;
          roster[rosterIndexToUpdate].functionButtonUpdateRequired = true;
          Serial.println("Initial response complete, function button update required");
        }
        for (int i = 0; i < NUMBER_OF_THROTTLES; i++) {
          if (throttles[i].rosterIndex == rosterIndexToUpdate) {
            drawThrottle(i);
            drawSelectedThrottle(i);
            Serial.println("Dir update on throttle " + String(i));
          }
        }

      }
      else if (actionType == "X") {
        //emergency stop
        roster[rosterIndexToUpdate].currentSpeed = 0;
        String spd = "M" + throttleIndex + "A*<;>V0\n";
        writeString(spd);
      }
      else if (actionType == "F") {
        //function
        String functionState = actionInstruction.substring(0, 1);
        String functionAddress = actionInstruction.substring(1);

        int iFunctionState = functionState.toInt();
        int iFunctionAddress = functionAddress.toInt();

        Serial.println("Function - " + roster[rosterIndexToUpdate].Name + "storing state " + functionState + " for address " + functionAddress);

        if (iFunctionAddress < maxFunctions) {
          roster[rosterIndexToUpdate].functionState[iFunctionAddress] = iFunctionState;
          if (!roster[rosterIndexToUpdate].initialResponseIncoming) {
            Serial.println("Initisl response complete, function button update required");
            roster[rosterIndexToUpdate].functionButtonUpdateRequired = true;
          }
        }
      }

      actionReceivedLine = "";
    }
    actionReceivedLine += x;
    if (actionReceivedLine.length() == 1)
    {
      actionControllerIndex = x;

    }
  }
  if (rosterCollecting == true) {
    if (x == '{' || x == '|' || x == '[' || x == '}' || x == ']' || x == '\\' || x == '\n') {
      delimeter += x;
      if (delimeter == "]\\[" || x == '\n') {
        //new loco
        Serial.println("Loco " + locoName);
        if (numberOfLocosInRoster < 0) {
          numberOfLocosInRoster = locoName.toInt();
          Serial.println ("Number of locos " + locoName);
          locoName = "";
        }
        else {
          //Serial.println ("Idtype " + locoName);
          currentLoco.IdType = locoName;
          locoName = "";
          locoArrayCounter = 0;
          roster[rosterCounter] = currentLoco;
          Serial.println(currentLoco.Name + " - " + String(rosterCounter));
          rosterCounter++;
        }

      }
      else if (delimeter == "}|{") {
        if (locoArrayCounter == 0) {
          Serial.println ("Name " + locoName);
          currentLoco.Name = locoName;
          locoName = "";
        }
        else if (locoArrayCounter == 1) {
          Serial.println ("Id " + locoName);
          currentLoco.Id = locoName;
          locoName = "";
        }
        else if (locoArrayCounter == 2 ) {

          locoName = "";
        }
        locoArrayCounter++;
        //Serial.println(delimeter);
      }
    }
    else {
      delimeter = "";
      locoName += x;
    }
  }
  if (x == '\n') {
    //Serial.print("Nreline");
    prefix = "";
    if (rosterCollecting) {
      //Serial.println("Roster complete");

    }
    else if (actionCollecting) {
      if (rosterIndex >= 0) {
        //drawFunctionButtons(rosterIndex);
      }

    }

    rosterCollecting = false;
    actionReceivedLine = "";
    actionCollecting = false;
    trackPowerCollecting = false;
    receivedLine = "";
  }
  if (prefix == "RL") {
    //Serial.println("Roster line");
    rosterCollecting = true;
  }
  if (prefix == "M") {
    //Serial.println("Incoming action");
    actionCollecting = true;
    actionReceivedLine += x;
  }
  if (prefix == "PPA") {
    Serial.println("Track powerr prefix " + prefix);
    trackPowerCollecting = true;
  }
  //Serial.print(x);

}

void printLocoToScreen(int i) {
  clearThrottleScreen(i);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  String centeredId = centreString(roster[throttles[i].newSelectedRosterIndex].Id, 10);
  String centeredName = centreString(roster[throttles[i].newSelectedRosterIndex].Name.substring(0, 24), 26);
  lcd.drawString(centeredId, throttles[i].tftX, throttles[i].tftY + 20, 6);
  lcd.drawString(centeredName, throttles[i].tftX, throttles[i].tftY + 100, 2);
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

  mqttClient.subscribe(selectorMovementTopic);
  mqttClient.subscribe(cabSignalTopic);
  mqttClient.subscribe(speedMovementTopic);
}


String centreString(String text, int maxLength) {
  int len = text.length();
  //Serial.println(text);
  //Serial.println("len " + String(len));
  int diff = maxLength - len;
  //Serial.println("diff " + String(diff));
  int halfDiff = diff / 2;
  String paddingString = "";
  for (int i = 0; i < halfDiff; i++) {
    paddingString = paddingString + " ";
  }
  return paddingString + text + paddingString;
}

void drawSignal(int i) {
  int mod = i % 2;
  int sigX = 17;
  //signal icons are placed at the edge of the screen so coordinates are different for left vs right screen meters
  //work out if this throttle is on the left or right of the screen then adjust X coordinates appropriately
  if (mod != 0) sigX = 301;
  uint16_t colour = 0;
  switch (throttles[i].signalAspect) {
    case 'P':
      colour = TFT_GREEN;
      break;
    case 'C':
      colour = TFT_ORANGE;
      break;
    case 'D':
      colour = TFT_RED;
      break;
  }
  if (colour > 0)
    lcd.fillCircle(sigX, throttles[i].tftY + 17, 15, colour);
  else {
    lcd.fillCircle(sigX, throttles[i].tftY + 17, 15, TFT_BLACK);
    lcd.drawCircle(sigX, throttles[i].tftY + 17, 15, TFT_GREEN);
  }

}

void drawTrainName(int x, int y, String trainName) {
  String centeredString = centreString(trainName.substring(0, 24), 26);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.drawString(centeredString, x, y + 110, 2);
}

void drawTrainNameCentered(int x, int y, String trainName) {
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.drawCentreString(trainName, x + 80, y + 110, 2);
}

void drawSelectedTrainNameCentered(int x, int y, String trainName, int fontSize) {
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.drawCentreString(trainName, x, y, fontSize);
}

//ringMeter and rainbow voids by Bodmer
//https://www.instructables.com/Arduino-analogue-ring-meter-on-colour-TFT-display/
int ringMeter(int value, int vmin, int vmax, int x, int y, int r, char *units, byte scheme)
{
  // Minimum value of r is about 52 before value text intrudes on ring
  // drawing the text first is an option

  x += r; y += r;   // Calculate coords of centre of ring

  int w = r / 4;    // Width of outer ring is 1/4 of radius

  int angle = 150;  // Half the sweep angle of meter (300 degrees)

  int text_colour = 0; // To hold the text colour

  int v = map(value, vmin, vmax, -angle, angle); // Map the value to an angle v

  byte seg = 5; // Segments are 5 degrees wide = 60 segments for 300 degrees
  byte inc = 10; // Draw segments every 5 degrees, increase to 10 for segmented ring

  // Draw colour blocks every inc degrees
  for (int i = -angle; i < angle; i += inc) {

    // Choose colour from scheme
    int colour = 0;
    switch (scheme) {
      case 0: colour = TFT_RED; break; // Fixed colour
      case 1: colour = TFT_GREEN; break; // Fixed colour
      case 2: colour = TFT_BLUE; break; // Fixed colour
      case 3: colour = rainbow(map(i, -angle, angle, 0, 127)); break; // Full spectrum blue to red
      case 4: colour = rainbow(map(i, -angle, angle, 63, 127)); break; // Green to red (high temperature etc)
      case 5: colour = rainbow(map(i, -angle, angle, 127, 63)); break; // Red to green (low battery etc)
      default: colour = TFT_BLUE; break; // Fixed colour
    }

    // Calculate pair of coordinates for segment start
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (r - w) + x;
    uint16_t y0 = sy * (r - w) + y;
    uint16_t x1 = sx * r + x;
    uint16_t y1 = sy * r + y;

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * 0.0174532925);
    float sy2 = sin((i + seg - 90) * 0.0174532925);
    int x2 = sx2 * (r - w) + x;
    int y2 = sy2 * (r - w) + y;
    int x3 = sx2 * r + x;
    int y3 = sy2 * r + y;

    if (i < v) { // Fill in coloured segments with 2 triangles
      lcd.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
      lcd.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
      text_colour = colour; // Save the last colour drawn
    }
    else // Fill in blank segments
    {
      lcd.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_GREY);
      lcd.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_GREY);
    }
  }

  // Convert value to a string
  if (value >= 0) {
    char buf[10];
    byte len = 4; if (value > 999) len = 5;
    dtostrf(value, len, 0, buf);

    // Set the text colour to default
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    // Uncomment next line to set the text colour to the last segment value!
    // tft.setTextColor(text_colour, ILI9341_BLACK);

    // Print value, if the meter is large then use big font 6, othewise use 4
    //int padding = tft.textWidth("128", 4);
    lcd.setTextPadding(50);
    if (r > 84) lcd.drawCentreString(buf, x - 5, y - 20, 6); // Value in middle
    else lcd.drawCentreString(buf, x - 5, y - 20, 4); // Value in middle

    // Print units, if the meter is large then use big font 4, othewise use 2
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    Serial.println(units);
    if (r > 84) {
      //int padding = tft.textWidth("Word", 4);
      lcd.setTextPadding(50);
      lcd.drawCentreString(units, x, y + 30, 4); // Units display
    }
    else {
      //int padding = tft.textWidth("Word", 2);
      lcd.setTextPadding(50);
      lcd.drawCentreString(units, x, y + 5, 2); // Units display
    }
  }
  // Calculate and return right hand side x coordinate
  return x + r;
}

// #########################################################################
// Return a 16 bit rainbow colour
// #########################################################################
unsigned int rainbow(byte value)
{
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  byte red = 0; // Red is the top 5 bits of a 16 bit colour value
  byte green = 0;// Green is the middle 6 bits
  byte blue = 0; // Blue is the bottom 5 bits

  byte quadrant = value / 32;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}

void clearThrottleScreen(int i) {
  Serial.println("Clear throttle screen - X " + String(throttles[i].tftX) + " Y " + String(throttles[i].tftY));
  lcd.fillRect( throttles[i].tftX, throttles[i].tftY, 160, 160, TFT_BLACK);
}
