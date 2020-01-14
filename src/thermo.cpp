//liste des bibliothèques
#include <OneWire.h>
#include <VirtualWire.h>
#include <Arduino.h>
#include <LowPower.h>

// Enable debug prints to serial monitor
#define DEBUG 0
#define TX_PIN 6
#define ONE_WIRE_BUS 3
#define VREF 1.1
#define LED 13
#define TEMP_OFFSET -0.25 // TODO:Attention à annuler l'offset dans le projet WL-recepteur
#define SLEEPING_TIME 5   // Temps d'attente entre deux émissions de valeurs.

OneWire ds(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)

/* Code de retour de la fonction getTemperature() */
enum DS18B20_RCODES
{
  READ_OK,         // Lecture ok
  NO_SENSOR_FOUND, // Pas de capteur
  INVALID_ADDRESS, // Adresse reçue invalide
  INVALID_SENSOR   // Capteur invalide (pas un DS18B20)
};

struct batteryCapacity
{
  float voltage;
  int capacity;
};

struct ThermometerData
{
  float temp;
  int batt;
};

const batteryCapacity remainingCapacity[] = {
    {4.18, 100},
    {4.15, 99},
    {4.13, 98},
    {4.11, 97},
    {4.09, 96},
    {4.07, 95},
    {4.05, 94},
    {4.04, 93},
    {4.02, 92},
    {4.01, 91},
    {3.99, 90},
    {3.98, 89},
    {3.97, 88},
    {3.96, 87},
    {3.95, 86},
    {3.94, 85},
    {3.93, 84},
    {3.92, 83},
    {3.91, 82},
    {3.90, 80},
    {3.89, 79},
    {3.88, 78},
    {3.87, 77},
    {3.86, 75},
    {3.85, 74},
    {3.84, 73},
    {3.83, 72},
    {3.82, 71},
    {3.82, 70},
    {3.81, 69},
    {3.80, 68},
    {3.79, 67},
    {3.78, 66},
    {3.77, 65},
    {3.76, 64},
    {3.75, 63},
    {3.74, 62},
    {3.73, 61},
    {3.72, 60},
    {3.71, 58},
    {3.70, 57},
    {3.69, 56},
    {3.68, 55},
    {3.67, 54},
    {3.66, 53},
    {3.65, 52},
    {3.64, 51},
    {3.63, 50},
    {3.62, 49},
    {3.61, 48},
    {3.60, 47},
    {3.59, 46},
    {3.58, 45},
    {3.57, 44},
    {3.56, 43},
    {3.55, 42},
    {3.54, 40},
    {3.53, 39},
    {3.52, 38},
    {3.51, 37},
    {3.50, 36},
    {3.49, 34},
    {3.48, 33},
    {3.47, 32},
    {3.46, 30},
    {3.45, 29},
    {3.44, 28},
    {3.43, 27},
    {3.42, 25},
    {3.41, 24},
    {3.40, 23},
    {3.39, 22},
    {3.38, 21},
    {3.37, 20},
    {3.36, 19},
    {3.35, 18},
    {3.33, 17},
    {3.32, 16},
    {3.31, 15},
    {3.29, 14},
    {3.27, 13},
    {3.26, 12},
    {3.24, 11},
    {3.21, 10},
    {3.19, 9},
    {3.16, 8},
    {3.14, 6},
    {3.11, 4},
    {3.07, 2},
    {3.04, 1},
    {3.00, 0},
};

const int ncell = sizeof(remainingCapacity) / sizeof(struct batteryCapacity);

//SETUP****************************************************
// cppcheck-suppress unusedFunction
void setup()
{
  if (DEBUG)
  {
    Serial.begin(9600);
    Serial.print("Setup");
  }
  vw_set_tx_pin(TX_PIN);
  vw_setup(2000);
  if (DEBUG)
  {
    Serial.println(" OK");
  }
}

long readVcc()
{
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);            // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC))
    ;
  result = ADCL;
  result |= ADCH << 8;
  result = 1125300L / result; // Back-calculate AVcc in mV
  return result;
}

unsigned int getBatteryCapacity()
{
  analogReference(INTERNAL);
  unsigned int adc = analogRead(0);
  delay(1);
  adc = analogRead(0);
  if (DEBUG)
  {
    Serial.print("ADC: ");
    Serial.println(adc);
  }

  float voltage = adc * VREF / 1023 / 0.242; // Avec des résistances de R1 = 15k et R2 = 47k,
                                             // on a Vs = Ve * (R1/(R1+R2))
                                             // Vs = Ve * 0.242

  if (DEBUG)
  {
    Serial.print("VCC: ");
    Serial.println(voltage, 3);
  }
  for (int i = 0; i < ncell; i++)
  {
    if (voltage > remainingCapacity[i].voltage)
    {
      if (DEBUG)
      {
        Serial.print(i);
        Serial.print(" : ");
        Serial.print(remainingCapacity[i].voltage);
        Serial.print(" | ");
        Serial.println(remainingCapacity[i].capacity);
      }
      return remainingCapacity[i].capacity;
    }
  }
  return 0;
}

/**
 * Fonction de lecture de la température via un capteur DS18B20.
 */
byte getTemperature(float *temperature, byte reset_search)
{
  byte data[9], addr[8];
  // data[] : Données lues depuis le scratchpad
  // addr[] : Adresse du module 1-Wire détecté

  /* Reset le bus 1-Wire ci nécessaire (requis pour la lecture du premier capteur) */
  if (reset_search)
  {
    ds.reset_search();
  }

  /* Recherche le prochain capteur 1-Wire disponible */
  if (!ds.search(addr))
  {
    // Pas de capteur
    return NO_SENSOR_FOUND;
  }

  /* Vérifie que l'adresse a été correctement reçue */
  if (OneWire::crc8(addr, 7) != addr[7])
  {
    // Adresse invalide
    return INVALID_ADDRESS;
  }

  /* Vérifie qu'il s'agit bien d'un DS18B20 */
  if (addr[0] != 0x28)
  {
    // Mauvais type de capteur
    return INVALID_SENSOR;
  }

  /* Reset le bus 1-Wire et sélectionne le capteur */
  ds.reset();
  ds.select(addr);

  /* Lance une prise de mesure de température et attend la fin de la mesure */
  ds.write(0x44, 1);
  delay(800);

  /* Reset le bus 1-Wire, sélectionne le capteur et envoie une demande de lecture du scratchpad */
  ds.reset();
  ds.select(addr);
  ds.write(0xBE);

  /* Lecture du scratchpad */
  for (byte i = 0; i < 9; i++)
  {
    data[i] = ds.read();
  }

  /* Calcul de la température en degré Celsius */
  *temperature = (int16_t)((data[1] << 8) | data[0]) * 0.0625 + TEMP_OFFSET;

  // Pas d'erreur
  return READ_OK;
}

void lowPowerSleep(int minutes)
{
  int seconds = minutes * 60;
  int sleeps = seconds / 8;
  for (int i = 0; i < sleeps; i++)
  {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
}

//LOOP****************************************************
// cppcheck-suppress unusedFunction
void loop()
{
  int batteryLevel = getBatteryCapacity();
  float temperature;

  if (getTemperature(&temperature, true) != READ_OK)
  {
    if (DEBUG)
    {
      Serial.println(F("Erreur de lecture du capteur"));
    }
    return;
  }

  ThermometerData dataToSend;
  dataToSend.batt = batteryLevel;
  dataToSend.temp = temperature;

  vw_send((byte *)&dataToSend, sizeof(dataToSend)); // On envoie le message
  vw_wait_tx();                                     // On attend la fin de l'envoi
  if (DEBUG)
  {
    Serial.print("transmitted battery level OK: ");
    Serial.print(batteryLevel);
    Serial.println("%");
    Serial.print("transmitted temperature OK: ");
    Serial.print(temperature);
    Serial.println("°C");
    delay(1000);
  }

  if (DEBUG)
  {
    delay(1000); // En DEBUG, les valeurs sont transmises toutes les secondes.
  }
  else
  {
    lowPowerSleep(SLEEPING_TIME);
  }
}
