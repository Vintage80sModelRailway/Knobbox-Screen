// ScreenButton.h
#ifndef ScreenButtone_h
#define ScreenButton_h

#include <Arduino.h>

class ScreenButton {
  private:

  public:
    String buttonText;
    int tftX;
    int tftY;
    int tftXEnd;
    int tftYEnd;
    bool isSelected;
    int functionIndex;
    bool isFunction;
    uint16_t boxColour;
    uint16_t textColour;
    uint16_t selectedTextColour;
    ScreenButton(String ButtonText, int tftLocationX, int tftLocationY, int tftLocationEndX, int tftLocationYEnd, uint16_t BoxColour, uint16_t TextColour, uint16_t SelectedTextColour, int FunctionIndex = -1);
};


#endif
