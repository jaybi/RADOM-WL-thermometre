

// Enable debug prints to serial monitor
#define DEBUG 1

#define TX_PIN                6
#define ONE_WIRE_BUS          3
#define LED                   13
#define VREF                  1.099

//liste des bibliothèques
#include <OneWire.h>
#include <VirtualWire.h>
#include <Arduino.h>
#include <LowPower.h>

//Liste des fonctions
float readDS18B20();
void setup();
void loop();
unsigned int getBatteryCapacity();
float getTemp();

OneWire ds(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)

struct batteryCapacity
{
  float voltage;
  int capacity;
};

struct ThermostatData
{
  float temp;
  int batt;
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

void setup() {
  Serial.begin(115200);
  if (DEBUG) {
    Serial.print("Setup");
  }
  vw_set_tx_pin(TX_PIN);
  vw_setup(2000);
  if (DEBUG) {
    Serial.println(" OK");
  }
}


unsigned int getBatteryCapacity() {
  //  float voltage = (1023 * VREF) / analogReadReference();
  analogReference(INTERNAL);
  analogRead(0);
  delay(1);
  unsigned int adc = analogRead(0);
  if (DEBUG) {
    Serial.print("ADC: ");
    Serial.println(adc);
  }

  float voltage = adc * VREF / 1023 / 0.248;
  if (DEBUG) {
    Serial.print("VCC: ");
    Serial.println(voltage, 3);
  }
  for (int i = 0 ; i < ncell ; i++){
    if (DEBUG) {
      Serial.print(i);
      Serial.print(" : ");
      Serial.print(remainingCapacity[i].voltage);
      Serial.print(" | ");
      Serial.println(remainingCapacity[i].capacity);
    }
    if (voltage > remainingCapacity[i].voltage) {
      return remainingCapacity[i].capacity;
    }
  }
  return 0;
}

float readDS18B20() {
  static float lastTemperature;

  float temperature = getTemp();

  // Only send data if temperature has changed and no error
  if (temperature == -127.00) {
    if (DEBUG) {
      Serial.println("DS18B20: read failed");
    }
  }
  if (temperature != -127.00 && temperature != 85.00) {
    // Save new temperatures for next compare
    lastTemperature = temperature;
  }
  return lastTemperature;
}
float getTemp()
{
  //returns the temperature from one DS18S20 in DEG Celsius
  byte data[12];
  byte addr[8];
  if ( !ds.search(addr)) {
    //no more sensors on chain, reset search
    ds.reset_search();
    return -1000;
  }
  if ( OneWire::crc8( addr, 7) != addr[7]) {
    if (DEBUG) {
      Serial.println("CRC is not valid!");
    }
    return -1000;
  }
  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    if (DEBUG) {
      Serial.print("Device is not recognized");
    }
    return -1000;
  }
  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end
  //byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad
  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }
  ds.reset_search();
  byte MSB = data[1];
  byte LSB = data[0];
  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;
  return TemperatureSum;
}

void lowPowerSleep(int minutes)
{
  int seconds = minutes * 60;
  int sleeps = seconds / 8;
  for (int i = 0 ; i < sleeps ; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
}

void loop()
{

  int batteryLevel = getBatteryCapacity();
  float temperature = readDS18B20();
  ThermostatData dataToSend;
  dataToSend.batt = batteryLevel;
  dataToSend.temp = temperature;

  vw_send((byte*) &dataToSend, sizeof(dataToSend)); // On envoie le message
  vw_wait_tx(); // On attend la fin de l'envoi
  if (DEBUG) {
    Serial.print("transmitted battery level OK: ");
    Serial.print(batteryLevel);
    Serial.println("%");
    Serial.print("transmitted temperature OK: ");
    Serial.print(temperature);
    Serial.println("Â°C");
  }

  lowPowerSleep(15);

}
