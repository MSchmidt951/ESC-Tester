#include <FastLED.h>
#include <RF24.h>
#include <Servo.h>

//Radio vars
RF24 radio(2, 3); //Sets CE and CSN pins of the radio
byte addresses[][6] = {"C", "D"};
byte data[7]; //Input data from radio

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
const int LED_Max = 40;

//Input vars
bool lastButtonInput[4];
bool buttonInput[4];
float leftPercent;
float rightPercent;
float potPercent;

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
    delay(500);
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

  //Keep ESCs at neutral for two seconds
  delay(2000);
}

void setup() {
  delay(1000);

  //Set up LEDs
  pinMode(modeLED, OUTPUT);
  FastLED.addLeds<WS2812, A0>(leds, 4);
  fill_solid(leds, 4, CRGB(LED_Max, 0, 0));
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
      radio.read(&data, sizeof(byte[7]));
    }

    //Check abort
    if (bitRead(data[6], 0)) {
      ABORT();
    }

    //Check standby
    if (bitRead(data[6], 1)) {
      ///standby();
    }
    
    //Get input
    reverseMode = bitRead(data[6], 2);
    //Get joystick input
    leftPercent = 1 - data[1]/255.0;
    rightPercent = data[2]/255.0;
    //Get potentiometer input
    potPercent = (data[4]-3)/252.0;
    //Enable/disable ESCs
    for(int i=0; i<4; i++) {
      if (not lastButtonInput[i] and buttonInput[i]) {
        ESCactivated[i] = !ESCactivated[i];
      }
      lastButtonInput[i] = buttonInput[i];
    }
    buttonInput[0] = bitRead(data[5], 0);
    buttonInput[1] = bitRead(data[5], 1);
    buttonInput[2] = bitRead(data[5], 4);
    buttonInput[3] = bitRead(data[5], 5);

    //If the right switch is on start the ESCs
    if (bitRead(data[6], 5) and not ESC[0].attached()) {
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
        leds[i] = CRGB(0, 0, LED_Max);
      } else if (ESCval[i] == minESCval) {
        //Orange when at minimum
        leds[i] = CRGB(LED_Max, LED_Max/3, 0);
      } else if (ESCval[i] == maxESCval) {
        //Green when at maximum
        leds[i] = CRGB(0, LED_Max, 0);
      } else {
        //Grey
        leds[i] = CRGB(LED_Max * (ESCval[i]/maxESCval), LED_Max * (ESCval[i]/maxESCval), LED_Max * (ESCval[i]/maxESCval));
      }
    }
  }
  FastLED.show();
  digitalWrite(modeLED, reverseMode);
}
