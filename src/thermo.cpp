//liste des bibliothèques
#include <OneWire.h>
#include <VirtualWire.h>
#include <Arduino.h>
#include <LowPower.h>

// Enable debug prints to serial monitor
#define DEBUG 1
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
    {4.20, 100},
    {4.10, 96},
    {4.00, 92},
    {3.96, 89},
    {3.92, 85},
    {3.89, 81},
    {3.86, 77},
    {3.83, 73},
    {3.80, 69},
    {3.77, 65},
    {3.75, 62},
    {3.72, 58},
    {3.70, 55},
    {3.66, 51},
    {3.62, 47},
    {3.58, 43},
    {3.55, 40},
    {3.51, 35},
    {3.48, 32},
    {3.44, 26},
    {3.40, 24},
    {3.37, 20},
    {3.35, 17},
    {3.27, 13},
    {3.20, 9},
    {3.1, 6},
    {3.00, 3},
};

const int ncell = sizeof(remainingCapacity) / sizeof(struct batteryCapacity);

//SETUP****************************************************
// cppcheck-suppress unusedFunction
void setup()
{
  Serial.begin(9600);
  if (DEBUG)
  {
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
