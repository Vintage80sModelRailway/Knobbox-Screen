#include "Sensor.h"
#include <Arduino.h>

Sensor::Sensor(String SensorName, int InputPin, String JMRIID, bool IsInverted, uint8_t PinMode, int DebounceOnMS, int DebounceOffMS)
  : _name(SensorName)
  , _pin(InputPin)
  ,  JMRIId(JMRIID)
  , _inverted(IsInverted)
  , _pinMode(PinMode)
{
  lastKnownState = -1;
  if (DebounceOnMS > -1) {
    usingOnDebounce = true;
    debounceOnMS = DebounceOnMS;
    Serial.println("Debounce on activated on " + JMRIId + " - " + String(debounceOnMS));
    millisAtLastChange = millis();
  }
  else
  {
    usingOnDebounce = false;
    Serial.println("No on debounce");
  }

  if (DebounceOffMS > -1) {
    usingOffDebounce = true;
    debounceOffMS = DebounceOffMS;
    Serial.println("Debounce off activated on " + JMRIId + " - " + String(DebounceOffMS));
    millisAtLastChange = millis();
  }
  else {
    usingOffDebounce = false;
    Serial.println("No off debounce");
  }
}


bool Sensor::UpdateSensor() {
  //Serial.println("D check");
  if (_pin == -1) return false;
  if (JMRIId == "") return false;
  bool hasChanged = false;
  int correctedVal = -1;
  if (_inverted) {
    correctedVal = !digitalRead(_pin);
  }  else {
    correctedVal = digitalRead(_pin);
  }
  
  if (!usingOnDebounce && !usingOffDebounce) {

    if (correctedVal == lastKnownState) {
      return false;
    }
    if (correctedVal == 0)
    {
      State = "INACTIVE";
      value = 0;
    }
    else
    {
      State = "ACTIVE";
      value = 1;
    }

    lastKnownState = correctedVal;
      Serial.println("Out 0");
    return true;
  }
  bool bypassDebounce = false;

  if (!inDebounce && lastKnownState == correctedVal)
  {
    //Serial.println("Out 1");
    return false;
  }

  if (!inDebounce && (usingOffDebounce || usingOnDebounce)) {
    //Serial.println("Into debounce current " + String(lastKnownState) + " new " + String(correctedVal));
    //sensor has gone to off - handle debounce
    millisAtLastChange = millis();
    inDebounce = true;
    debounceValue = correctedVal;
    //Serial.println("In debounce - " + JMRIId);
  }

  if (inDebounce) {
    unsigned long millisSinceLastChange = (millis() - millisAtLastChange);
    if (correctedVal != debounceValue) {
      //Serial.println("Flapping ? debounce " + String(debounceValue) + " new " + String(correctedVal));
      inDebounce = false;
    }
    else {
      if ((correctedVal ==  true && debounceValue == true && millisSinceLastChange >= debounceOnMS)
          || (correctedVal == false && debounceValue == false && millisSinceLastChange >= debounceOffMS) ) {
        if (lastKnownState != correctedVal) {
          //Serial.println("Safe to change now " + String(correctedVal) + " - debounce millis - " + String(millisSinceLastChange));
          lastKnownState = correctedVal;
          inDebounce = false;
          if (correctedVal == 0)
          {
            State = "INACTIVE";
            value = 0;
          }
            
          else
          {
            State = "ACTIVE";
            value = 1;
          }
            
          return true;
        }

        else {
          inDebounce = false;
          return false;
        }
      }
    }
  }
  return false;
}

bool Sensor::UpdateShiftRegisterSensor(int val) {
  int correctedVal = -1;
  if (_inverted) {

    correctedVal = !val;
  }  else {
    correctedVal = val;
  }

  if (correctedVal == lastKnownState) return false;

  if (correctedVal == 0)
    State = "INACTIVE";
  else
    State = "ACTIVE";

  lastKnownState = correctedVal;

  return true;
}

void Sensor::SetPinMode() {
  pinMode(_pin, _pinMode);
}

String Sensor::GetSensorPublishTopic() {
  if (JMRIId.length() > 0) return "track/sensor/" + JMRIId;
  else return "";
}
