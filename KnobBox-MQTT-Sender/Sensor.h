// Sensor.h
#ifndef Sensor_h
#define Sensor_h

#include <Arduino.h>

class Sensor {
  private:
    int lastKnownState;
    int _pin;    
    String _name;
    bool _inverted;
    uint8_t _pinMode;
    bool usingOnDebounce;
    bool usingOffDebounce;
    int debounceOnMS;
    int debounceValue;
    int debounceOffMS;
    bool inDebounce;
    //Mode - 0 to off only, 1 to on only, 2 both

  public: 
    String JMRIId;
    String State;
    unsigned long millisAtLastChange;
    unsigned long millisAtOnHold;
    int value;
    
    Sensor(String SensorName = "", int InputPin = -1, String JMRIID = "", bool IsInverted = false, uint8_t PinMode = INPUT, int DebounceOnMS = -1, int DebounceOffMS = -1);
    bool UpdateSensor();
    void SetPinMode();
    String GetSensorPublishTopic();
    bool UpdateShiftRegisterSensor(int val);
};


#endif
