/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * DESCRIPTION
 *
 * Example sProcesseurketch showing how to send in DS1820B OneWire temperature readings back to the controller
 * http://www.mysensors.org/build/temp
 */


// Enable debug prints to serial monitor
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#define MY_RF24_CE_PIN        7
#define MY_RF24_CS_PIN        8
#define ONE_WIRE_BUS          3
#define LED                   13
#define SLEEP_TIME            900000L
#define COMPARE_TEMP          0
#define VREF                  1.099

#include <Arduino.h>
#include <MySensors.h>
#include <DallasTemperature.h>
#include <OneWire.h>

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature.

#define CHILD_ID_TEMP         0

struct batteryCapacity
{
  float voltage;
  int capacity;
};

const batteryCapacity remainingCapacity[] = {
  4.20,   100,
  4.10,   96,
  4.00,   92,
  3.96,   89,
  3.92,   85,
  3.89,   81,
  3.86,   77,
  3.83,   73,
  3.80,   69,
  3.77,   65,
  3.75,   62,
  3.72,   58,
  3.70,   55,
  3.66,   51,
  3.62,   47,
  3.58,   43,
  3.55,   40,
  3.51,   35,
  3.48,   32,
  3.44,   26,
  3.40,   24,
  3.37,   20,
  3.35,   17,
  3.27,   13,
  3.20,   9,
  3.1,    6,
  3.00,   3,
};

const int ncell = sizeof(remainingCapacity) / sizeof(struct batteryCapacity);

void before()
{
  Serial.print("MYSENSORS DS18B20 Temperature sensor: ");
  // Startup up the OneWire library
  sensors.begin();
  Serial.println("OK");
}

void setup()
{
  Serial.begin(115200);
  Serial.print("Setup");
  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);
  Serial.println(" OK");
}

void presentation() {
  Serial.print("Presentation");
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("DS18B20 Temperature Sensor", "1.1");

  // Present all sensors to controller
  present(CHILD_ID_TEMP, S_TEMP);
  Serial.println(" OK");
}

unsigned int analogReadReference(void)
{
  /* Elimine toutes charges rÃ©siduelles */
  ADMUX = 0x4F;
  delayMicroseconds(5);
  /* SÃ©lectionne la rÃ©fÃ©rence interne Ã  1.1 volts comme point de mesure, avec comme limite haute VCC */
  ADMUX = 0x4E;
  delayMicroseconds(200);
  /* Active le convertisseur analogique -> numÃ©rique */
  ADCSRA |= (1 << ADEN);
  /* Lance une conversion analogique -> numÃ©rique */
  ADCSRA |= (1 << ADSC);
  /* Attend la fin de la conversion */
  while(ADCSRA & (1 << ADSC));
  /* RÃ©cupÃ¨re le rÃ©sultat de la conversion */
  return ADCL | (ADCH << 8);
}

unsigned int getBatteryCapacity(void)
{
//  float voltage = (1023 * VREF) / analogReadReference();
  analogReference(INTERNAL);
  analogRead(0);
  delay(1);
  unsigned int adc = analogRead(0);
#ifdef MY_DEBUG
  Serial.print("ADC: ");
  Serial.println(adc);
#endif
  float voltage = adc * VREF / 1023 / 0.248;
#ifdef MY_DEBUG
  Serial.print("VCC: ");
  Serial.println(voltage, 3);
#endif
  for (int i = 0 ; i < ncell ; i++){
#ifdef MY_DEBUG
    Serial.print(i);
    Serial.print(" : ");
    Serial.print(remainingCapacity[i].voltage);
    Serial.print(" | ");
    Serial.println(remainingCapacity[i].capacity);
#endif
    if (voltage > remainingCapacity[i].voltage) {
      return remainingCapacity[i].capacity;
    }
  }
  return 0;
}

void readDS18B20(void)
{
  MyMessage tempMsg(0,V_TEMP);
  static float lastTemperature;

  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures();

  // query conversion time and sleep until conversion completed
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
  sleep(conversionTime);

  // Read temperatures and send them to controller
  // Fetch and round temperature to one decimal
  float temperature = static_cast<float>(static_cast<int>((getControllerConfig().isMetric ? sensors.getTempCByIndex(0) : sensors.getTempFByIndex(0)) * 10.)) / 10.;

  // Only send data if temperature has changed and no error
  if (temperature == -127.00) {
    Serial.println("DS18B20: read failed");
  }
  #if COMPARE_TEMP == 1
  if (lastTemperature != temperature && temperature != -127.00 && temperature != 85.00) {
  #else
  if (temperature != -127.00 && temperature != 85.00) {
  #endif
    // Send in the new temperature
    send(tempMsg.setSensor(CHILD_ID_TEMP).set(temperature, 1));
    // Save new temperatures for next compare
    lastTemperature = temperature;
    Serial.print("transmitted temperature OK: ");
    Serial.print(temperature);
    Serial.println("Â°C");
  }
}

void loop()
{
  readDS18B20();
  sleep(SLEEP_TIME);
  int batteryLevel = getBatteryCapacity();
  sendBatteryLevel(batteryLevel);
  Serial.print("transmitted battery level OK: ");
  Serial.print(batteryLevel);
  Serial.println("%");
}
