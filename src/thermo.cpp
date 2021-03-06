// Inclusion des bibliothèques
#include <OneWire.h>
#include <VirtualWire.h>
#include <Arduino.h>
#include <LowPower.h>
// Inclusion des headers
#include "struct.h"
#include "const.h"
#include "functions.h"

// Enable debug prints to serial monitor
//#define DEBUG 0 // Permet des sorties sur le Serial si TRUE
#define TX_PIN 6
#define ONE_WIRE_BUS 3
#define VREF 1.1
#define LED 13
#define TEMP_OFFSET -0.25 // TODO:Attention à annuler l'offset dans le projet WL-recepteur
#define SLEEPING_TIME 5   // Temps d'attente entre deux émissions de valeurs.
#define THERMO_PIN 0      // Broche du thermometre

OneWire ds(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)

/* Code de retour de la fonction getTemperature() */
enum DS18B20_RCODES
{
  READ_OK,         // Lecture ok
  NO_SENSOR_FOUND, // Pas de capteur
  INVALID_ADDRESS, // Adresse reçue invalide
  INVALID_SENSOR   // Capteur invalide (pas un DS18B20)
};

//SETUP****************************************************
void setup()
{
  initSerial(9600);

  vw_set_tx_pin(TX_PIN);
  vw_setup(2000);

  serialDebug("", "Done.");
}

unsigned int getBatteryCapacity()
{
  analogReference(INTERNAL);
  for (int i = 0; i < 5; i++)
  {
    analogRead(THERMO_PIN); // Boucle de rejet des 5 premières valeurs
    delay(1);
  }
  unsigned int adc = analogRead(THERMO_PIN);

  serialDebug("ADC", adc);

  float voltage = adc * VREF / 1023 / 0.242; // Avec des résistances de R1 = 15k et R2 = 47k,
                                             // on a Vs = Ve * (R1/(R1+R2))
                                             // Vs = Ve * 0.242

  serialDebug("VCC", voltage, 3);

  for (int i = 0; i < ncell; i++)
  {
    if (voltage > remainingCapacity[i].voltage)
    {
      serialDebug("Tension", remainingCapacity[i].voltage);
      serialDebug("Capacité", remainingCapacity[i].capacity);
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
void loop()
{
  digitalWrite(LED, HIGH); // La led s'allume au début de la loop()

  int batteryLevel = getBatteryCapacity();
  float temperature;

  if (getTemperature(&temperature, true) != READ_OK)
  {
    serialDebug("Erreur de lecture du capteur.");
    delay(1000); //Temporise le temps de capter la données
    return;
  }

  ThermometerData dataToSend;
  dataToSend.batt = batteryLevel;
  dataToSend.temp = temperature;

  vw_send((byte *)&dataToSend, sizeof(dataToSend)); // On envoie le message
  vw_wait_tx();                                     // On attend la fin de l'envoi

  serialDebug("transmitted battery level OK", batteryLevel);
  serialDebug("transmitted temp OK", temperature);

  digitalWrite(LED, LOW); // La LED s'éteint à la fin de la loop()

  if (DEBUG)
  {
    delay(1000); // En DEBUG, les valeurs sont transmises toutes les secondes.
  }
  else
  {
    lowPowerSleep(SLEEPING_TIME);
  }
}
