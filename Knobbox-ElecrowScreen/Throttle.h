// Throttle.h
#ifndef Throttle_h
#define Throttle_h

#include <Arduino.h>

class Throttle {
  private:
    int id;

  public:
    String TrainName;
    int rosterIndex;
    String mtIndex;
    char signalAspect;
    int tftX;
    int tftY;
    int tftXEnd;
    int tftYEnd;
    int speedStep;
    int currentSelectorRosterIndex;
    int newSelectedRosterIndex;
    bool isSelected;
    Throttle(String MTIndex, int tftLocationX, int tftLocationY, int tftLocationEndX, int tftLocationYEnd);
};


#endif
