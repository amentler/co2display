#include "Buttons.h"
#include <Arduino.h>

void Buttons::init() {
    button1Pin = D2;
    button2Pin = D3;
    pinMode(button1Pin, INPUT);
}

bool Buttons::isButton1Pressed() {
    bool result = digitalRead(button1Pin);
    delay(10);
    result = result && digitalRead(button1Pin);

    return result;
}
bool Buttons::isButton2Pressed() {
    return false;
}