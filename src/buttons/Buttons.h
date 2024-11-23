
#ifndef SCHILD_BUTTONS_H
#define SCHILD_BUTTONS_H

#include "Arduino.h"

class Buttons {
public:
    void init();
    bool isButton1Pressed();
    bool isButton2Pressed();
private:
    int button1Pin;
    int button2Pin;
};

#endif //SCHILD_BUTTONS_H
