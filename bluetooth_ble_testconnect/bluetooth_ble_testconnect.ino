#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BluefruitLE_SPI.h>
#include "Adafruit_BLEGatt.h"


#define BLUEFRUIT_SPI_CS               8
#define BLUEFRUIT_SPI_IRQ              7
#define BLUEFRUIT_SPI_RST              4

// If set to 'true' enables debug output
#define VERBOSE_MODE                   true  
#define FACTORYRESET_ENABLE            0

const int buttonInput = 5;
const int signalRecievedLed = 12;
const int ledOutput = 13;
int sendSignal = 0;

// Für LED-Anzeige
int buttonState = 0;

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
Adafruit_BLEGatt gatt(ble);

/* this is just for testing */

int32_t testServiceId;
int32_t testCharId;
int32_t testReadId;



uint8_t SEND_SERVICE_UUID[] = {0xF5,0x61,0x7C,0xD1,0x38,0xE8,0x4E,0x45,
                           0xAD,0x46,0x63,0xB7, 0xD0,0xDB,0x0E,0x01};
                          
uint8_t TEST_SENSOR_UUID[] = {0x39,0x8C,0x26,0xB3,0xC1,0x0D,0x4C,0xF0,0xAB,
                           0xD2,0x39,0xB7,0x91,0x4F,0xFC,0x02};

uint8_t TEST_RECIEVE_UUID[] = {0x39,0x8C,0x26,0xB3,0xC1,0x0D,0x4C,0xF0,0xAB,
0xD2,0x39,0xB7,0x91,0x4F,0xFC,0x03};

uint8_t sendSignalOne[] = {1};
byte noHundred[1] = {0x00};
uint8_t signalOne[1];

byte inputSensorOne = {0x04};
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
  pinMode(ledOutput, OUTPUT);
  pinMode(signalRecievedLed, OUTPUT);
  
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
  if (! ble.sendCommandCheckOK(F( "AT+GAPDEVNAME=MeetingCube1" )) ) {
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
  ble.reset();

  Serial.println();
}


void printHex(uint8_t num) {
  char hexCar[2];

  sprintf(hexCar, "%02X", num);
  Serial.println(hexCar);
}

void loop() {
  ble.update();
  
  gatt.getChar(testReadId, signalOne, sizeof(signalOne));
  printHex(signalOne[0]);
  if (signalOne[0] == inputSensorOne)
  {
    digitalWrite(signalRecievedLed, HIGH);
    Serial.println("Succes");
  }
  if (signalOne[0] == noSignal)
  {
    digitalWrite(signalRecievedLed, LOW);
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
    digitalWrite(ledOutput, LOW);
    //digitalWrite(signalRecievedLed, LOW);
    //gatt.setChar(testCharId, noHundred, sizeof(noHundred));
  }


  
}
