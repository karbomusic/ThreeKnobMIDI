/*

  ________                      __ __             __
  /_  __/ /_  ________  ___     / //_/____  ____  / /_
  / / / __ \/ ___/ _ \/ _ \   / ,<  / __ \/ __ \/ __ \
  / / / / / / /  /  __/  __/  / /| |/ / / / /_/ / /_/ /
  /_/ /_/ /_/_/   \___/\___/  /_/ |_/_/ /_/\____/_.___/

  Author: Kary Wall
  Date: 2/16/18
  Version: 1.5

*/

#include "MIDIUSB.h"
#include "colors.h"

enum ScrollDirection //control num
{
  sAuto = 100,
  sHorz = 101,
  sVert = 102,
  zHorz = 103,
  zVert = 104,
  xFlip = 105
};

// usually the rotary encoders three pins have the ground pin in the middle
enum PinAssignments {
  encoderScrollPinA = 2,   // right
  encoderScrollPinB = 3,   // left
  encoderZoomPinA = 0,    // right
  encoderZoomPinB = 1,    // left
  scrollToggleButton = 4,
  zoomToggleButton = 5
};

const int  redPin = 7;
const int  greenPin = 8;
const int  bluePin = 9;
const int  pwrPin = 6;
const int  vertLedScroll = 10;
const int  vertLedZoom = 16;
const int potPin = A2;  // read pin for the analog pot
const int midiChannel = 1;

int scrollPos = 0;
int zoomPos = 0;
int lastScrollDir = 0;
int lastZoomDir = 0;
int midiScrollCC = sHorz;
int midiZoomCC = zHorz;
bool buttonDown = false;
const int debounceDelay = 300;
int lastAutomationValue = 0;      // last pot value

float EMA_a = 0.6;      //initialization of EMA alpha   // Smoothing
int EMA_S = 0;          //initialization of EMA S       // Smoothing

volatile unsigned int scrollEncoderPos = 0;  // a counter for the dial
volatile unsigned int zoomEncoderPos = 0;    // a counter for the dial
volatile unsigned int pinstatus = 0;

unsigned int lastScrollEncoderPos = 1;      // change detection
unsigned int lastZoomEncoderPos = 1;        // change detection
static boolean scrollRotating = false;      // debounce 
static boolean zoomRotating = false;        // debounce 

// interrupt service routine vars (scroll)
boolean A_set = false;
boolean B_set = false;

// interrupt service routine vars (zoom)
boolean C_set = false;
boolean D_set = false;

void controlChange(byte channel, byte control, byte value)
{
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void sendMidi(long control, long value)
{
  Serial.println("Sending: CC: " + (String)control + " Value: " + (String)value);
  controlChange(midiChannel, (byte)control, (byte)value);
  MidiUSB.flush();
  delay(5);
}

void setLedColor(byte color)
{
  analogWrite(redPin, colorTable[(byte)color][0]);
  analogWrite(greenPin, colorTable[(byte)color][1]);
  analogWrite(bluePin, colorTable[(byte)color][2]);
  delay(6);
}

void setup() {

  Serial.begin(115200);
 // Serial1.end(); // kill tx/rx on pins 1/0 just in case

  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(pwrPin, OUTPUT);
  pinMode(vertLedZoom , OUTPUT);
  pinMode(vertLedScroll, OUTPUT);

  pinMode(encoderScrollPinA, INPUT);
  pinMode(encoderScrollPinB, INPUT);
  pinMode(encoderZoomPinA, INPUT);
  pinMode(encoderZoomPinB, INPUT);
  pinMode(scrollToggleButton, INPUT);
  pinMode(zoomToggleButton, INPUT);
  pinMode(potPin, INPUT);

  // turn on pullup resistors
  digitalWrite(encoderScrollPinA, HIGH);
  digitalWrite(encoderScrollPinB, HIGH);
  digitalWrite(encoderZoomPinA, HIGH);
  digitalWrite(encoderZoomPinB, HIGH);

  // force buttons off (on = pulldown)
  digitalWrite(scrollToggleButton, HIGH);
  digitalWrite(zoomToggleButton, HIGH);

  // power is on
  digitalWrite(pwrPin, HIGH);

  // turn off non-power LEDs initially
  digitalWrite(bluePin, LOW);
  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);
  digitalWrite(vertLedZoom, LOW);
  digitalWrite(vertLedScroll, LOW);

  // encoder pin on interrupt 0 (pin 2)
  attachInterrupt(0, doScrollEncoderA, CHANGE);
  // encoder pin on interrupt 1 (pin 3)
  attachInterrupt(1, doScrollEncoderB, CHANGE);
  // encoder pin on interrupt 2 (pin 0)
  attachInterrupt(3, doZoomEncoderA, CHANGE);
  // encoder pin on interrupt 3 (pin 1)
  attachInterrupt(2, doZoomEncoderB, CHANGE);

  EMA_S = analogRead(potPin);
  
  /* Welcome routine */
  for(int i=0;i<8;i++)
  {
    setLedColor(i);
    delay(300);
  }
  
   setLedColor(none);
}

void loop() {

  // ----------------- AUTOMATION ----------------------------

  // read analog absolute encoder
  int value = analogRead(potPin);
  
  Serial.println((String)value);
  
  // smooting to prevent jitter
  EMA_S = (EMA_a * value) + ((1 - EMA_a) * EMA_S);

  if (EMA_S != lastAutomationValue)
  {
    
    // scale range from 1024 down to 128; // deprecated by me --> (EMA_S >> 3);
    unsigned int mappedValue = map(EMA_S, 0, 1023, 0, 127);
    
    // send the midi CC 
    setLedColor(purple);
    sendMidi((byte)sAuto, (byte)mappedValue);
    MidiUSB.flush();
    lastAutomationValue = EMA_S;
    setLedColor(none);
    return;
  }

  // ------------------ ORIENTATION TOGGLE ------------------
  if (digitalRead(scrollToggleButton) == LOW && buttonDown == false)
  {
    buttonDown = true;
    if (digitalRead(zoomToggleButton) == HIGH)
    {
      setLedColor(cyan);
      midiScrollCC = midiScrollCC == sHorz ? sVert : sHorz;
      digitalWrite(vertLedScroll, midiScrollCC == sVert ? LOW : HIGH);
    }
    else
    {
      setLedColor(red);
      sendMidi(xFlip, 127);
    }
    
    delay(debounceDelay);
  }
  else if (digitalRead(zoomToggleButton) == LOW && buttonDown == false)
  {
    setLedColor(white);
    buttonDown = true;
    midiZoomCC = midiZoomCC == zHorz ? zVert : zHorz;
    digitalWrite(vertLedZoom, midiZoomCC == zVert ? HIGH : LOW);
    delay(debounceDelay);
  }
   
  
  buttonDown = false;     // reset the debouncers
  scrollRotating = true;
  zoomRotating = true;

  //----------------- Scroll ---------------------------------

  if (lastScrollEncoderPos != scrollEncoderPos) {

    Serial.print("Index:");
    Serial.println(scrollEncoderPos, DEC);

    int scrollDir = lastScrollEncoderPos > scrollEncoderPos ? +1 : -1;

    if (scrollDir != lastScrollDir)
    {
      scrollPos = 0;
    }

    scrollPos  = scrollPos  + scrollDir;
    setLedColor(orange);
    sendMidi(midiScrollCC, scrollPos);

    lastScrollDir = scrollDir;
    lastScrollEncoderPos = scrollEncoderPos;

  }

  //----------------- ZOOM -----------------------

  if (lastZoomEncoderPos != zoomEncoderPos) {

    Serial.print("Index:");
    Serial.println(zoomEncoderPos, DEC);

    // figure out if the direction changed  - move to interrupt code later if needed
    int zoomDir = lastZoomEncoderPos > zoomEncoderPos ? +1 : -1;

    // reset relative value to 0 if direction changed
    if (zoomDir != lastZoomDir)
    {
      zoomPos = 0;
    }

    zoomPos  = zoomPos  + zoomDir;
    setLedColor(green);
    sendMidi(midiZoomCC, zoomPos);

    lastZoomDir = zoomDir;
    lastZoomEncoderPos = zoomEncoderPos;

  }

    setLedColor(none);
    delay(10);
}

// ----------------- Interrupt handlers ---------------------------------

void doScrollEncoderA() {

  // debounce
  if ( scrollRotating ) delay (1);  // wait a little until the bouncing is done

  // test transition, did things really change?
  if ( digitalRead(encoderScrollPinA) != A_set ) { // debounce once more
    A_set = !A_set;

    // adjust counter + if A leads B
    if ( A_set && !B_set )
      scrollEncoderPos += 1;

    scrollRotating = false;  // no more debouncing until loop() hits again
  }
}

void doZoomEncoderA() {

  // debounce
  if ( zoomRotating ) delay (1);  // wait a little until the bouncing is done

  // test transition, did things really change?
  if ( digitalRead(encoderZoomPinA) != C_set ) { // debounce once more
    C_set = !C_set;

    // adjust counter + if A leads B
    if ( C_set && !D_set )
      zoomEncoderPos += 1;

    zoomRotating = false;  // no more debouncing until loop() hits again
  }
}

void doScrollEncoderB() {

  if ( scrollRotating ) delay (1);
  if ( digitalRead(encoderScrollPinB) != B_set ) {
    B_set = !B_set;
    //  adjust counter - 1 if B leads A
    if ( B_set && !A_set )
      scrollEncoderPos -= 1;

    scrollRotating = false;
  }
}

void doZoomEncoderB() {

  if ( zoomRotating ) delay (1);
  if ( digitalRead(encoderZoomPinB) != D_set ) {
    D_set = !D_set;
    //  adjust counter - 1 if B leads A
    if ( D_set && !C_set )
      zoomEncoderPos -= 1;

    zoomRotating = false;
  }
}
