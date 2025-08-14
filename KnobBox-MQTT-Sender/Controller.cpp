#include "Controller.h"
#include <Arduino.h>
#include "Sensor.h"

Controller::Controller(int EncoderPinA, int EncoderPinB, int EncoderBtn)
  : encoderPinA(EncoderPinA)
  , encoderPinB(EncoderPinB)
  , encoderBtn(EncoderBtn)
{
  pinMode (encoderPinA, INPUT);
  pinMode (encoderPinB, INPUT);
  pinMode(encoderBtn, INPUT_PULLUP);
  PinA_prev = digitalRead(encoderPinA);
  ButtonState = !digitalRead(encoderBtn);
  clk = Sensor("clk",EncoderPinA,"clk",false,INPUT,1,1);
  dt = Sensor("dt",EncoderPinB,"dt",false,INPUT,1,1);
}

String Controller::GetMovement() {
  bool clkChanged = clk.UpdateSensor();
  //if (clkChanged) {
  //  PinA_value = clk.value;
    //dt.UpdateSensor();    
  //}
  PinA_value = clk.value;
  String movement = "";

  if (PinA_value != PinA_prev && PinA_value == HIGH) {
    //bool dtChanged = dt.UpdateSensor();
    int dtVal = dt.value;
    //Serial.println(String(dtVal));
    
    if (dtVal == HIGH) {
      movement = "AC";
    } else {
      // the encoder is rotating in clockwise direction => increase the counter
      movement = "C";
    }
  }
  
  PinA_prev = PinA_value;
  clk.UpdateSensor();
  dt.UpdateSensor();
  // check if button is pressed (pin SW)
  bool newButtonState = !digitalRead(encoderBtn);
  if (newButtonState != ButtonState) {    
    buttonIsHeld = 0;
    millisAtButtonPress = millis();
    if (newButtonState == HIGH) {
      //Serial.println("Button Pressed");
      ButtonState = 1;    
      //Serial.println("Button change pressed"); 
      movement = "SEL";
    }
    else {
      //Serial.println("Button Released");
      ButtonState = 0;
      //Serial.println("Button change released");      
    }
  }
  else {
    if (ButtonState == 1 && (millis() - millisAtButtonPress > 1000) && buttonIsHeld == 0) {
      buttonIsHeld = 1;
      //Serial.println("Button held");
    }    
  }
  
  return movement;
}

bool Controller::CheckMovement() {
  PinA_value = digitalRead(encoderPinA);
  bool movementDetected = false;

  if (PinA_value != PinA_prev) { // check if knob is rotating
    // if pin A state changed before pin B, rotation is clockwise
    movementDetected = true;
    
    if (digitalRead(encoderPinB) != PinA_value) {
      CW = true;
      InternalCount++;
      //Serial.println("Int "+String(InternalCount));
    } else {
      // if pin B state changed before pin A, rotation is counter-clockwise
      CW = false;
      InternalCount--;
      //Serial.println("Int "+String(InternalCount));
    }
    /*
    if (((InternalCount % 2) == 0)) {
      if (CW) {
        Serial.print("Clockwise selector ");
        Serial.println(" CW movement");
      } else {
        Serial.print("Anticlockwise selector ");
        Serial.println(" AC movement");
      }
    }
    */
  }
  PinA_prev = PinA_value;

  // check if button is pressed (pin SW)
  bool newButtonState = !digitalRead(encoderBtn);
  if (newButtonState != ButtonState) {    
    buttonIsHeld = 0;
    millisAtButtonPress = millis();
    if (newButtonState == HIGH) {
      //Serial.println("Button Pressed");
      ButtonState = 1;    
      Serial.println("Button change pressed"); 
    }
    else {
      //Serial.println("Button Released");
      ButtonState = 0;
      Serial.println("Button change released");
    }
  }
  else {
    if (ButtonState == 1 && (millis() - millisAtButtonPress > 1000) && buttonIsHeld == 0) {
      buttonIsHeld = 1;
      Serial.println("Button held");
    }    
  }
  return movementDetected;
}
