#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BluefruitLE_SPI.h>
#include "Adafruit_BLEGatt.h"
#include "MAX30105.h"
#include <HX711_ADC.h>
#include "heartRate.h"

// TODO: Code aufräumen!


#define BLUEFRUIT_SPI_CS               8
#define BLUEFRUIT_SPI_IRQ              7
#define BLUEFRUIT_SPI_RST              4

// If set to 'true' enables debug output
#define VERBOSE_MODE                   true  
#define FACTORYRESET_ENABLE            0

const int acc_X = A3;
const int acc_Y = A4;
const int acc_Z = A5;

int sekundenCounter = 0;
int triggerCounter = 0;
int fingerCounter = 0;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred


//TODO SORTIEREN
const int HX711_dout = 6; //mcu > HX711 dout pin
const int HX711_sck = 10; //mcu > HX711 sck pin
const int buttonInput = 5;
const int signalReceivedLed = 12;
const int ledOutput = 13;
int sendSignal = 0;

float beatsPerMinute;
int beatAvg;

int heartbeat_signal_lock = 0;
int longButtonPressCounter = 0;

MAX30105 particleSensor;
HX711_ADC LoadCell(HX711_dout, HX711_sck);

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

uint8_t TEST_RECEIVE_UUID[] = {0x39,0x8C,0x26,0xB3,0xC1,0x0D,0x4C,0xF0,0xAB,
0xD2,0x39,0xB7,0x91,0x4F,0xFC,0x13};


// --- SEND AND RECEIVE SIGNALS FOR ACTIVITIES
uint8_t send_heartbeatactivity[] = {1};
uint8_t send_scaleactivity[] = {2};
uint8_t send_shakeactivity[] = {3};
uint8_t send_btnactivity[] = {4};
uint8_t send_longbtnactivity[] = {5};

byte receive_heartbeatactivity = {0x04};
byte receive_scaleactivity = {0x05};
byte receive_shakeactivity = {0x06};

byte receive_heartbeatactivity_andjoined = {0x07};
byte receive_scaleactivity_andjoined = {0x08};
byte receive_shakeactivity_andjoined = {0x09};

uint8_t bluetooth_receive_signal[1];
byte noSignal = {0x00};

long t = 0;

                           

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
  // pinMode(signalReceivedLed, OUTPUT);
  pinMode(13, OUTPUT); //LED 1  Schütteln
  pinMode(12, OUTPUT); //LED 2  Waage
  pinMode(11, OUTPUT); //LED 3  Herzfrequenz
  pinMode(9, OUTPUT); //LED 4  Activity Joined
  Serial.begin(38400);


  // SCALE SETUP
  LoadCell.begin();
  float calibrationValue; // calibration value (see example file "Calibration.ino")
  calibrationValue = 696.0; // uncomment this if you want to set the calibration value in the sketch
    unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }
  
 
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

  testReadId = gatt.addCharacteristic(TEST_RECEIVE_UUID, GATT_CHARS_PROPERTIES_WRITE  | GATT_CHARS_PROPERTIES_READ | GATT_CHARS_PROPERTIES_NOTIFY,1 ,16,BLE_DATATYPE_BYTEARRAY);
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
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0; //increase value to slow down serial print activity

  // ---------------------- LEDs ----------------------
  ble.update();
  
  gatt.getChar(testReadId, bluetooth_receive_signal, sizeof(bluetooth_receive_signal));
  printHex(bluetooth_receive_signal[0]);


  //Serial.println(bluetooth_receive_signal[0]);
  // Heartbeat LED
  if ((bluetooth_receive_signal[0] == receive_heartbeatactivity) || (bluetooth_receive_signal[0] == receive_heartbeatactivity_andjoined))
  {
    digitalWrite(11, HIGH);
  } else {
    digitalWrite(11, LOW);
  }
  // SCALE LED
  if ((bluetooth_receive_signal[0] == receive_scaleactivity) || (bluetooth_receive_signal[0] == receive_scaleactivity_andjoined))
  {
    digitalWrite(12, HIGH);
  } else {
    digitalWrite(12, LOW);
  }
  // SHAKE LED
  if ((bluetooth_receive_signal[0] == receive_shakeactivity) || (bluetooth_receive_signal[0] == receive_shakeactivity_andjoined))
  {
    digitalWrite(13, HIGH);
  } else {
    digitalWrite(13, LOW);
  }
  // Activity joined LED
  if ((bluetooth_receive_signal[0] == receive_heartbeatactivity_andjoined) || (bluetooth_receive_signal[0] == receive_scaleactivity_andjoined) || (bluetooth_receive_signal[0] == receive_shakeactivity_andjoined))
  {
    digitalWrite(9, HIGH);
  } else {
    digitalWrite(9, LOW);
  }
  /*
  // TODO alle signale ausstellen
  /*if (bluetooth_receive_signal[0] == noSignal)
  {
    digitalWrite(12, LOW);
  }
  */

  // ---------------------- SCALE ----------------------
  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    float j = LoadCell.getData();
      if (j > 100) {
        gatt.setChar(testCharId, send_scaleactivity, sizeof(send_scaleactivity));
        Serial.println("Send Trigger for Scale");
        //digitalWrite(12, HIGH);
      }   
  }
 
  // ---------------------- BUTTON ----------------------
  buttonState = digitalRead(buttonInput);
  if (buttonState == HIGH)
  {
    longButtonPressCounter = longButtonPressCounter + 1;

    if (longButtonPressCounter > 50) {
      gatt.setChar(testCharId, send_longbtnactivity, sizeof(send_longbtnactivity));
      Serial.println("Button pressed long!");
    } else {
      gatt.setChar(testCharId, send_btnactivity, sizeof(send_btnactivity));
      Serial.println("Button pressed!");
    }
  } 

  
  // ---------------------- HEARTBEAT ----------------------
  long irValue = particleSensor.getIR();
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
    //Serial.println(" No finger?");
  } else {
    if (sekundenCounter < 5000) {
      fingerCounter += 1;
    } 
  }
  if (fingerCounter > 150) {
    // hier stattdessen das signal senden
    //digitalWrite(11, HIGH);
    // sende 2 für heartbeat trigger
    Serial.println("Sende Trigger für Heartbeat");
    gatt.setChar(testCharId, send_heartbeatactivity, sizeof(send_heartbeatactivity));

  }
  else {
    digitalWrite(11, LOW);
  }

  
  // ---------------------- SHAKE SENSOR ----------------------
  if (analogRead(acc_X) < 450 || analogRead(acc_X) > 600) {
    triggerCounter = triggerCounter + 1;
  }
  if (analogRead(acc_Y) < 100 || analogRead(acc_Y) > 500) {
    //triggerCounter = triggerCounter + 1;
  }
  if (analogRead(acc_Z) < 100 || analogRead(acc_Z) > 700) {
    triggerCounter = triggerCounter + 1;
  }

  /*
  Serial.print ("X:");
  Serial.print(analogRead(acc_X));
  Serial.print ("Y:");
  Serial.print(analogRead(acc_Y));
  Serial.print ("Z:");
  Serial.print(analogRead(acc_Z));
  Serial.print (" Counter:");
  Serial.print(triggerCounter);
  Serial.println();*/
  if (triggerCounter > 20) {
    triggerCounter = 0;
    gatt.setChar(testCharId, send_shakeactivity, sizeof(send_shakeactivity));
    Serial.println("Send Trigger for Shake");
  }

  // ---------------------- RESET ----------------------
  if (sekundenCounter > 5000) {
    sekundenCounter = 0;
    triggerCounter = 0;
    fingerCounter = 0;
    heartbeat_signal_lock = 0;
    longButtonPressCounter = 0;
  } 
  sekundenCounter = sekundenCounter + 10;
  //Serial.println(sekundenCounter);
}
