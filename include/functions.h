#ifndef FUNCTIONS_H
#define FUNCTION_H

void serialDebug(String);
void serialDebug(String, String);
void serialDebug(String, int);
void serialDebug(String, float, int);
void initSerial(int);
void delayIfDebug(int);

#endif