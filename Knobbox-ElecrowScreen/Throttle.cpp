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
  SpeedStep = 45;
  newSelectedRosterIndex = 2;
  isSelected = false;
  rosterIndex = 17;
}
