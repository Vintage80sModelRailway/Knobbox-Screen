// Controller.h
#ifndef Controller_h
#define Controller_h

#include <Arduino.h>
#include "Sensor.h"

class Controller {
  private:
    int encoderPinA; // CLK pin
    int encoderPinB; // DT pin
    int encoderBtn; // SW pin

    int PinA_prev;
    int PinA_value;
    bool ButtonState;
    int id;

    bool CW;
    bool isSelector;
    
    unsigned long millisAtButtonPress;

    bool buttonIsHeld;   

    Sensor clk;
    Sensor dt;

  public:
    int InternalCount;
    Controller(int EncoderPinA, int EncoderPinB, int EncoderBtn);
    String GetMovement();
    bool CheckMovement();
};


#endif
