int pinx =A0;
int piny =A1;
int pinz =A2;
int sekundenCounter = 0;
int triggerCounter = 0;

void setup()

{
  Serial.begin(9600);
}

void loop () 
{

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
  sekundenCounter = sekundenCounter + 10;
  if (sekundenCounter > 5000) {
    Serial.print(triggerCounter);
    Serial.println();
    sekundenCounter = 0;
    triggerCounter = 0;
  } 
  if (triggerCounter > 100) {
    Serial.print ("trigger: ");
    Serial.print(triggerCounter);
    Serial.println();
    sekundenCounter = 0;
    triggerCounter = 0;
  }
delay(10);
}
