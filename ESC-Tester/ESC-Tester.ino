#include <FastLED.h>
#include <RF24.h>
#include <Servo.h>

//Radio vars
RF24 radio(2, 3); //Sets CE and CSN pins of the radio
byte addresses[][6] = {"C", "D"};
char Data[7]; //Input data from radio

//ESC vars
const int ESCpins[4] = {9, 8, 7, 6};
Servo ESC[4];
int ESCval[4];
bool ESCactivated[4] = {false, false, false, false};
const int minESCval = 1000;
const int maxESCval = 2000;

//LED vars
CRGB leds[4];
const int modeLED = 4;
const int LED_Max = 100;

//Misc vars
bool reverseMode = true; //Reverse mode sets the neutral to the mid point
int neutral;

void ABORT() {
  //Shut off ESCs
  for (int i=0; i<10; i++){
    for (int j=0; j<4; j++){
      ESC[j].writeMicroseconds(neutral);
    }
  }

  //Blink the LEDs on and off
  while(true) {
    for (int i=0; i<4; i++){
      fill_solid(leds, 4, CRGB(LED_Max, 0, 0));
    }
    FastLED.show();
    delay(750);
    for (int i=0; i<4; i++){
      fill_solid(leds, 4, CRGB(0, 0, 0));
    }
    FastLED.show();
  }
}

void initESC() {
  //Attach ESCs
  for(int i=0; i<4; i++) {
    ESC[i].attach(ESCpins[i], minESCval, maxESCval);
    ESC[i].writeMicroseconds(neutral);
  }

  //Show that the ESCs are being initialised
  fill_solid(leds, 4, CRGB(LED_Max, 0, LED_Max));
  FastLED.show();

  //Keep ESCs at neutral for a second
  delay(1000);
}

void setup() {
  delay(1000);

  //Set up LEDs
  pinMode(modeLED, OUTPUT);
  FastLED.addLeds<WS2812, A0>(leds, 4);
  fill_solid(leds, 4, CRGB(255, 0, 0));
  FastLED.show();

  //Set up radio
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(124);
  
  //Open pipe
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1, addresses[0]);
  
  //Ready device
  for (int i=0; i<10; i++){
    radio.write(0b01010101, 1);
    delay(5);
  }
  radio.startListening();

  //Start the serial communication
  Serial.begin(57600);
}

void loop() {
  if (radio.available()) {
    //Get data from controller
    while (radio.available()) {
      radio.read(&Data, sizeof(char[7]));
    }

    //Check abort
    if (bitRead(Data[7], 0)) {
      ABORT();
    }

    //Check standby
    if (bitRead(Data[7], 0)) {
      ///standby();
    }
    
    //Get input
    reverseMode = bitRead(Data[7], 2);
    ///Get joysticks
    ///Get pot
    ///Get buttons

    if () { ///TODO - add condition
      initESC();
    }
  }

  //Calculate ESC values
  if (reverseMode) {
    neutral = (maxESCval+minESCval)/2;
  } else {
    neutral = minESCval;
  }

  //Apply to hardware
  if (ESC[0].attached()) {
    for(int i=0; i<4; i++) {
      if (ESCactivated[i]) {
        ESC[i].writeMicroseconds(ESCval);
      } else {
        ESC[i].writeMicroseconds(neutral);
      }

      //Set LED colour
      if (not ESCactivated[i] or ESCval[i] == neutral) {
        //Blue when neutral
        fill_solid(leds, 4, CRGB(0, 0, LED_Max));
      } else if (ESCval[i] == minESCval) {
        //Orange when at minimum
        fill_solid(leds, 4, CRGB(LED_Max, LED_Max/3, 0));
      } else if (ESCval[i] == maxESCval) {
        //Orange when at maximum
        fill_solid(leds, 4, CRGB(0, 0, LED_Max));
      } else {
        //Grey
        fill_solid(leds, 4, CRGB(LED_Max * (ESCval/maxESCval), LED_Max * (ESCval/maxESCval), LED_Max * (ESCval/maxESCval)));
      }
    }
  }
  FastLED.show();
  digitalWrite(modeLED, reverseMode);
}
