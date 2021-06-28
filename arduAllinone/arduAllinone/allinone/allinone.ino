/*
   -------------------------------------------------------------------------------------
   HX711_ADC
   Arduino library for HX711 24-Bit Analog-to-Digital Converter for Weight Scales
   Olav Kallhovd sept2017
   -------------------------------------------------------------------------------------
*/

/*
   This example file shows how to calibrate the load cell and optionally store the calibration
   value in EEPROM, and also how to change the value manually.
   The result value can then later be included in your project sketch or fetched from EEPROM.

   To implement calibration in your project sketch the simplified procedure is as follow:
       LoadCell.tare();
       //place known mass
       LoadCell.refreshDataSet();
       float newCalibrationValue = LoadCell.getNewCalibration(known_mass);
*/

#include <HX711_ADC.h>
#include <Wire.h>
#include "MAX30105.h"

#include "heartRate.h"

#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#endif

//pins:
const int HX711_dout = 4; //mcu > HX711 dout pin
const int HX711_sck = 5; //mcu > HX711 sck pin
int pinx =A0;
int piny =A1;
int pinz =A2;
int sekundenCounter = 0;
int triggerCounter = 0;
int fingerCounter = 0;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

MAX30105 particleSensor;

const int calVal_eepromAdress = 0;
unsigned long t = 0;

void setup() {
  //LEDS mit dem jeweiligen digitalen port an den sie angeschlossen werden müssen
  //von dem digital port ein kabel zum widerstand, vom widerstand zum längern ende der led. das kürzere ende mit ground verbinden
  pinMode(13, OUTPUT); //LED 1
  pinMode(12, OUTPUT); //LED 2
  pinMode(11, OUTPUT); //LED 3

  //der rest ist kalibrierung für gewichtssensor 

  Serial.begin(57600); delay(10);
  Serial.println();
  Serial.println("Starting...");

    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
}

void loop() {

  //mit LOW schaltet man die led aus, mit high wieder ein
  //der jeweilige port je nachdem welche led an sein soll
  //digitalWrite(13,HIGH);
  //delay(1000);
  //digitalWrite(13,LOW);
  //delay(1000);
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
  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  //wenn irvalue über 50000 für 5 sekunden würde ich den trigger senden
  if (irValue < 50000) {
    Serial.print(" No finger?");
  } else {
    if (sekundenCounter < 5000) {
      fingerCounter += 1;
    } 
  }
    
  if (fingerCounter > 450) {
    // hier stattdessen das signal senden
    digitalWrite(13,HIGH);
  }

  Serial.println();

  //hier ist der beschleunigungssensor bzw. der trigger dafür
  if (analogRead(pinx) < 100 || analogRead(pinx) > 500) {
    triggerCounter = triggerCounter + 1;
    //Serial.print ("X:");
    //Serial.print(analogRead(pinx));
    //Serial.println();
  }
  if (analogRead(piny) < 100 || analogRead(piny) > 500) {
    triggerCounter = triggerCounter + 1;
    //Serial.print ("Y:");
    //Serial.print(analogRead(piny));
    //Serial.println();
  }
  if (analogRead(pinz) < 50 || analogRead(pinz) > 400) {
    triggerCounter = triggerCounter + 1;
    //Serial.print ("Z:");
    //Serial.print(analogRead(pinz));
    //Serial.println();
  }

  //fingercounter wird hier auch zurück gesetzt
  if (sekundenCounter > 5000) {
    Serial.print(triggerCounter);
    Serial.println();
    sekundenCounter = 0;
    triggerCounter = 0;
    fingerCounter = 0;
  } 
  sekundenCounter = sekundenCounter + 10;

  //hier wollt ihr den code reinschreiben. Wenn der triggerCounter auf 100 ist muss das signal gesendet werden an die anderen aduinos
  //led soll angehen,
  if (triggerCounter > 100) {
    Serial.print ("trigger: ");
    Serial.print(triggerCounter);
    Serial.println();
    sekundenCounter = 0;
    triggerCounter = 0;
  }

  Serial.print ("X:");
  Serial.print(analogRead(pinx));
  Serial.println();
}
