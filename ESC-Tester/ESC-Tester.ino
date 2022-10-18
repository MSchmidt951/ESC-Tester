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
bool ESCactivated[4];
const int minESCval = 1000;
const int maxESCval = 2000;
const int maxESCdiff = 1000;
const int ESCstep = 10;

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
bool joystickMode = true;
int neutral;

void ABORT() {
  //Shut off and detach ESCs
  for (int i=0; i<4; i++){
    ESC[i].detach();
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

int LEDpercent(int index) {
  float multiplier = ESCval[index]/(float)maxESCval - .5;
  return LED_Max * multiplier * 1.5;
}

int roundStep(int value) {
  return ((value-1+ESCstep/2)/ESCstep) * ESCstep;
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
  radio.setRadiation(RF24_PA_MAX, RF24_2MBPS);
  radio.setChannel(124);
  radio.setPayloadSize(7);
  
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

    //Give the joysticks a deadzone
    for(int i=1; i<=2; i++) {
      if (data[i] >= 124 and data[i] <= 130) {
        data[i] = 127;
      }
    }
    
    //Get input
    reverseMode = bitRead(data[6], 2);
    joystickMode = !bitRead(data[6], 1);
    //Get joystick input
    leftPercent = 1 - data[1]/254.0;
    rightPercent = data[2]/254.0;
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
  for(int i=0; i<4; i++) {
    //If ESC activated, apply change depending on the inputs
    if (ESCactivated[i]) {
      if (joystickMode) {
        if (i < 2) {
          ESCval[i] = leftPercent*maxESCdiff - maxESCdiff/2;
        } else {
          ESCval[i] = rightPercent*maxESCdiff - maxESCdiff/2;
        }
      } else {
        ESCval[i] = potPercent * maxESCdiff/2;
      }

      if (not reverseMode) {
        ESCval[i] = max(ESCval[i]*2, 0);
      }
    } else {
      ESCval[i] = 0;
    }

    ESCval[i] += neutral;

    //Round the ESC value
    ESCval[i] = roundStep(ESCval[i]);
  }

  //Apply to hardware
  if (ESC[0].attached()) {
    for(int i=0; i<4; i++) {
      ESC[i].writeMicroseconds(ESCval[i]);

      //Set LED colour
      if (not ESCactivated[i]) {
        //Turquoise when ESC not activated
        leds[i] = CRGB(0, LED_Max/4, LED_Max/2);
      } else if (ESCval[i] == neutral) {
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
        leds[i] = CRGB(LEDpercent(i), LEDpercent(i), LEDpercent(i));
      }
    }
  }
  FastLED.show();
  digitalWrite(modeLED, reverseMode);

  //Print ESC values to serial
  for(int i=0; i<4; i++) {
    Serial.print(ESCval[i]);
    Serial.print('\t');
  }
  Serial.println();
}
