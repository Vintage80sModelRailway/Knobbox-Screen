#include "Throttle.h"
#include <Arduino.h>

#define offsetVal 0;

Throttle::Throttle(String MTIndex, int tftLocationX, int tftLocationY, int tftLocationXEnd, int tftLocationYEnd)
  : mtIndex(MTIndex)
  , tftX(tftLocationX)
  , tftY(tftLocationY)
  , tftXEnd(tftLocationXEnd)
  , tftYEnd(tftLocationYEnd)
{
  rosterIndex = 0;
  signalAspect = 'X';
  speedStep = 0;
  newSelectedRosterIndex = -1;
  isSelected = false;
  rosterIndex = -1;
  currentSelectorRosterIndex = -1;
  lastSelectionActivity = millis();
}
