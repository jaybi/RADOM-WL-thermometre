#include <Arduino.h>

#include "functions.h"

void serialDebug(String key, String value)
{
    if (DEBUG)
    {
        Serial.print(key);
        Serial.print(": ");
        Serial.println(value);
    }
}

void serialDebug(String key, int value)
{
    if (DEBUG)
    {
        Serial.print(key);
        Serial.print(": ");
        Serial.println(value);
    }
}

void serialDebug(String key, float value, int digit)
{
    if (DEBUG)
    {
        Serial.print(key);
        Serial.print(": ");
        Serial.println(value, digit);
    }
}

void serialDebug(String text)
{
    if (DEBUG)
    {
        Serial.println(text);
    }
}

void initSerial(int speed)
{
    if (DEBUG)
    {
        Serial.begin(speed);
        Serial.println(F("Initialisation..."));
    }
}

void delayIfDebug(int duree)
{
    if (DEBUG)
    {
        delay(duree);
    }
}