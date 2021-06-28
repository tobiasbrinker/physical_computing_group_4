#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BluefruitLE_SPI.h>
#include "Adafruit_BLEGatt.h"
#include "MAX30105.h"
#include <HX711_ADC.h>
#include "heartRate.h"


#define BLUEFRUIT_SPI_CS               8
#define BLUEFRUIT_SPI_IRQ              7
#define BLUEFRUIT_SPI_RST              4

// If set to 'true' enables debug output
#define VERBOSE_MODE                   true  
#define FACTORYRESET_ENABLE            0

const int acc_X = A2;
const int acc_Y = A3;
const int acc_Z = A3;

int sekundenCounter = 0;
int triggerCounter = 0;
int fingerCounter = 0;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

const int buttonInput = 5;
const int signalRecievedLed = 12;
const int ledOutput = 13;
int sendSignal = 0;

float beatsPerMinute;
int beatAvg;

int heartbeat_signal_lock = 0;

MAX30105 particleSensor;

// Für LED-Anzeige
int buttonState = 0;

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
Adafruit_BLEGatt gatt(ble);

/* this is for bluetooth */

int32_t testServiceId;
int32_t testCharId;
int32_t testReadId;

uint8_t SEND_SERVICE_UUID[] = {0xF5,0x61,0x7C,0xD1,0x38,0xE8,0x4E,0x45,
                           0xAD,0x46,0x63,0xB7, 0xD0,0xDB,0x0E,0x11};
                          
uint8_t TEST_SENSOR_UUID[] = {0x39,0x8C,0x26,0xB3,0xC1,0x0D,0x4C,0xF0,0xAB,
                           0xD2,0x39,0xB7,0x91,0x4F,0xFC,0x12};

uint8_t TEST_RECIEVE_UUID[] = {0x39,0x8C,0x26,0xB3,0xC1,0x0D,0x4C,0xF0,0xAB,
0xD2,0x39,0xB7,0x91,0x4F,0xFC,0x13};

uint8_t sendSignalOne[] = {1};
uint8_t sendSignalTwo[] = {2};
uint8_t signalOne[1];

byte inputSensorOne = {0x04};
byte inputSensorTwo = {0x05};
byte noSignal = {0x00};
                           

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void setup()
{
  delay(500);
  
  pinMode(buttonInput, INPUT);
  // pinMode(ledOutput, OUTPUT);
  // pinMode(signalRecievedLed, OUTPUT);
  pinMode(13, OUTPUT); //LED 1
  pinMode(12, OUTPUT); //LED 2
  pinMode(11, OUTPUT); //LED 3
  pinMode(6, OUTPUT); //LED HEARTBEAT AUSGELÖST (bei anderen)
  
  Serial.begin(115200);
 
  if ( !ble.begin(false) ) error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?")); 
  // Set to false for silent and true for debug
  
  if ( !ble.factoryReset() ) error(F("Couldn't factory reset"));
  
   /* Disable command echo from Bluefruit */
  ble.echo(false);
   Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();


  /* Change the device name to make it easier to find */
  Serial.println(F("Setting device name to 'MeetingCube': "));
  if (! ble.sendCommandCheckOK(F( "AT+GAPDEVNAME=MeetingCube2" )) ) {
    error(F("Could not set device name?"));
  }

  /* Add the testservice to gatt */
  testServiceId = gatt.addService(SEND_SERVICE_UUID);
  if (testServiceId == 0) error(F("Could not add Test service"));

  /* Add the testchar characteristics to gatt */
  testCharId = gatt.addCharacteristic(TEST_SENSOR_UUID, GATT_CHARS_PROPERTIES_NOTIFY,1 ,16,BLE_DATATYPE_BYTEARRAY);
  if (testCharId == 0) error(F("Could not add TestChar service"));

  testReadId = gatt.addCharacteristic(TEST_RECIEVE_UUID, GATT_CHARS_PROPERTIES_WRITE  | GATT_CHARS_PROPERTIES_READ | GATT_CHARS_PROPERTIES_NOTIFY,1 ,16,BLE_DATATYPE_BYTEARRAY);
  if (testReadId == 0) error(F("Could not add TestRead service"));

 
  /* Reset the device for the new service setting changes to take effect */
  Serial.print(F("Performing a SW reset (service changes require a reset): "));
  //ble.reset();

  
   if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED

  ble.reset();
}


void printHex(uint8_t num) {
  char hexCar[2];

  sprintf(hexCar, "%02X", num);
  //Serial.println(hexCar);
}

void loop() {
  ble.update();
  
  gatt.getChar(testReadId, signalOne, sizeof(signalOne));
  printHex(signalOne[0]);

  
  if (signalOne[0] == inputSensorOne)
  {
    //LED an Port 12 = Jemand hat Lust auf Aktivität 2
    digitalWrite(12, HIGH);
    Serial.println("Succes");
  }
  if (signalOne[0] == inputSensorTwo) {
    digitalWrite(6, HIGH);

  }
  if (signalOne[0] == noSignal)
  {
    digitalWrite(12, LOW);
    digitalWrite(6, LOW);
  }
  
 
  
  buttonState = digitalRead(buttonInput);
  if (buttonState == HIGH)
  {
    /* Set 100 as the characteristics */
    gatt.setChar(testCharId, sendSignalOne, sizeof(sendSignalOne));
    Serial.println("Button pressed!");
    
  } 
  else{
    sendSignal = 0;
    digitalWrite(13, LOW);
  }

  long irValue = particleSensor.getIR();
  //heartbeat geschichte
  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }
  /*
  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);
  Serial.println();
  */
  //wenn irvalue über 50000 für 5 sekunden würde ich den trigger senden
  if (irValue < 50000) {
    Serial.print(" No finger?");
  } else {
    if (sekundenCounter < 5000) {
      fingerCounter += 1;
    } 
  }
  if (fingerCounter > 150) {
    // hier stattdessen das signal senden
    digitalWrite(11, HIGH);
    // sende 2 für heartbeat trigger
    Serial.println("Sende Trigger für Heartbeat");
    gatt.setChar(testCharId, sendSignalTwo, sizeof(sendSignalTwo));

  }
  else {
    digitalWrite(11, LOW);
  }
  //hier ist der beschleunigungssensor bzw. der trigger dafür
  if (analogRead(acc_X) < 100 || analogRead(acc_X) > 500) {
    triggerCounter = triggerCounter + 1;
  }
  /*
      Serial.print ("X:");
    Serial.print(analogRead(acc_X));
    Serial.println();
     Serial.print ("Y:");
    Serial.print(analogRead(acc_Y));
    Serial.println();*/
  if (analogRead(acc_Y) < 100 || analogRead(acc_Y) > 500) {
    triggerCounter = triggerCounter + 1;
  }
  if (analogRead(acc_Z) < 50 || analogRead(acc_Z) > 400) {
    //triggerCounter = triggerCounter + 1;
    //Serial.print ("Z:");
    //Serial.print(analogRead(pinz));
    //Serial.println();
  }

      Serial.print("Finger");
    Serial.println(sekundenCounter);
  //fingercounter wird hier auch zurück gesetzt
  if (sekundenCounter > 10000) {
    Serial.print(triggerCounter);
    Serial.println();
    sekundenCounter = 0;
    triggerCounter = 0;
    fingerCounter = 0;
    heartbeat_signal_lock = 0;
  } 
  sekundenCounter = sekundenCounter + 10;
  //Serial.println(sekundenCounter);

  //hier wollt ihr den code reinschreiben. Wenn der triggerCounter auf 100 ist muss das signal gesendet werden an die anderen aduinos
  //led soll angehen,
  if (triggerCounter > 100) {
    Serial.print ("trigger: ");
    Serial.print(triggerCounter);
    Serial.println();
    sekundenCounter = 0;
    triggerCounter = 0;
    digitalWrite(12, HIGH);
  }
  else {
    digitalWrite(12, LOW);
  }
}
