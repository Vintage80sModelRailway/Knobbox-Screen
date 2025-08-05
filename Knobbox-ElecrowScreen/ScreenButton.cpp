#include "ScreenButton.h"
#include <Arduino.h>

ScreenButton::ScreenButton(String ButtonText, int tftLocationX, int tftLocationY, int tftLocationXEnd, int tftLocationYEnd, uint16_t BoxColour, uint16_t TextColour, uint16_t SelectedTextColour, int FunctionIndex)
  : buttonText(ButtonText)
  , tftX(tftLocationX)
  , tftY(tftLocationY)
  , tftXEnd(tftLocationXEnd)
  , tftYEnd(tftLocationYEnd)
  , functionIndex (FunctionIndex)
  , boxColour(BoxColour)
  , textColour(TextColour)
  , selectedTextColour(SelectedTextColour)
{
  if (functionIndex >=0) {
    isFunction = true;
  }
  else {
    isFunction = false;
  }
}
