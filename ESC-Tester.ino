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
int ESCvals[4];
bool ESCactivated[4] = {false, false, false, false};
const int minESCval = 1000;
const int maxESCval = 2000;

//LED vars
CRGB leds[4];
const int modeLED = 4;

//Misc vars
unsigned long loopStart;
bool reverseMode = true; ///Explain what reverseMode is
int neutral;

void ABORT() {
  for (int i=0; i<10; i++){
    for (int j=0; j<4; j++){
      ESC[j].writeMicroseconds(neutral);
    }
  }

  //Blink the LEDs on and off
  while(true) {
    for (int i=0; i<4; i++){
      fill_solid(leds, 4, CRGB(255, 0, 0));
    }
    FastLED.show();
    delay(750);
    for (int i=0; i<4; i++){
      fill_solid(leds, 4, CRGB(0, 0, 0));
    }
    FastLED.show();
  }
}

void setup() {
  delay(1000);

  FastLED.addLeds<WS2812, A0>(leds, 4);
  pinMode(modeLED, OUTPUT);
  fill_solid(leds, 4, CRGB(255, 0, 0));
  FastLED.show();

  for(int i=0; i<4; i++) {
    ESC[i].attach(ESCpins[i], minESCval, maxESCval);
  }

  //Set up communication
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
  loopStart = micros();

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
  }

  //Calculation stuff ///Change the name of this
  if (reverseMode) {
    neutral = (maxESCval+minESCval)/2;
  } else {
    neutral = minESCval;
  }

  //Apply to hardware
  for(int i=0; i<4; i++) {
    if (ESCactivated[i]) {
      ESC[i].writeMicroseconds(ESCvals);
    } else {
      ESC[i].writeMicroseconds(neutral);
    }
  }
  FastLED.show();
  digitalWrite(modeLED, reverseMode);
}
