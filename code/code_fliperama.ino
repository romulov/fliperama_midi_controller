
/* Fliperama MIDI Controller
/// License///

Licensed under a Creative Commons Attribution4.0 International License (CC BY 4.0). 
Copyrightremains with the author(s)

Inspired by the course "Fazendo música com Arduino", available at hotmart.com.br


*/

// include MIDIUSB library
#include "MIDIUSB.h"
 
#include <Multiplexer4067.h> // Multiplexer CD4067 library
#include <Thread.h> // Threads library 
#include <ThreadController.h> //  include ThreadControlle library


// buttons
const byte muxNumberButtons = 5; // digital inputs on the multiplexer
const byte NumberButtons = 11; // digital inputs used
const byte totalButtons = muxNumberButtons + NumberButtons;
const byte muxButtonPin[muxNumberButtons] = {12, 13, 14, 15, 11}; // digital port pins used
const byte buttonPin[NumberButtons] = {18, 19, 20, 10, 16, 14, 15, 6, 7, 8, 9}; // digital port pins used
int buttonCurrentState[totalButtons] = {0}; // digital port current state
int buttonPreviousState[totalButtons] = {0}; // digital port previous state


// debounce
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 5;    // the debounce time; increase if the output flickers


// potentiometers
const byte NumberPots = 10; // number of analog inputs used on Arduino
const byte muxPotPin[NumberPots] = {8, 9, 10, 1, 0, 3, 2, 5, 4, 6}; // analog port pins used on Arduino
int potCurrentState[NumberPots] = {0}; // analogic port current state
int potPreviousState[NumberPots] = {0}; // analogic port previous state
int potVar = 0; // variation between the previous state value and the current analog port
int lastCcValue[NPots] = {0};


// potentiometer reading
int TIMEOUT = 50; //time for the potentiometer to be read after exceeding the Threshold
byte varThreshold = 4; //threshold for the variation in the potentiometer signal
boolean potMoving = true; // used to verify movement in the potentiometer
unsigned long pTime[NumberPots] = {0}; // previously stored time
unsigned long timer[NumberPots] = {0}; // stores the time that has elapsed since the timer was reset


// midi
byte midiCh = 0; // *MIDI channel
byte note = 36; // maximum range of musical notes
byte cc = 1; /


// Multiplexer
Multiplexer4067 mplex = Multiplexer4067(2, 3, 4, 5, A3); //inserting multiplexer ports


// threads 
ThreadController cpu; //thread master
Thread threadReadPots; // thread for potentiometers



void setup() {

  mplex.begin(); // start the multiplexer

  for (int i = 0; i < NumberButtons; i++) { // start the buttons
    pinMode(buttonPin[i], INPUT_PULLUP);
  }

  pinMode(A3, INPUT_PULLUP);

  
  // threads
  threadReadPots.setInterval(10);
  threadReadPots.onRun(readPots);
  cpu.add(&threadReadPots);
  

}

void loop() {

  cpu.run();
  readButtons();

}



// read buttons

void readButtons() {

  for (int i = 0; i < muxNumberButtons; i++) { //reads buttons on mux
    int buttonReading = mplex.readChannel(muxButtonPin[i]);
    
    if (buttonReading > 1000) {
      buttonCurrentState[i] = HIGH;
    }
    else {
      buttonCurrentState[i] = LOW;
    }
  
  }
  

  for (int i = 0; i < NumberButtons; i++) {  //read buttons on Arduino
    buttonCState[i + muxNumberButtons] = digitalRead(buttonPin[i]); // stores in the rest of buttonCState
  }

  for (int i = 0; i < totalButtons; i++) {

    if ((millis() - lastDebounceTime) > debounceDelay) {

      if (buttonCState[i] != buttonPState[i]) {
        lastDebounceTime = millis();

        if (buttonCState[i] == LOW) {
          noteOn(potMidiCh(), note + i, 127);  // Channel 0, middle C, normal velocity
          MidiUSB.flush();
          buttonPState[i] = buttonCState[i];
        }
        else {
          noteOn(potMidiCh(), note + i, 0);  // Channel 0, middle C, normal velocity
          MidiUSB.flush();
          buttonPState[i] = buttonCState[i];
        }
      }
    }

  }

}



//read potentiometers

void readPots() {

  for (int i = 0; i < NumberPots - 1; i++) { // reads all analog inputs used, except the one dedicated to changing the midi channel
    potCurrentState[i] = mplex.readChannel(muxPotPin[i]);
  }

  for (int i = 0; i < NumberPots; i++) {

    potVar = abs(potCurrentState[i] - potPreviousState[i]); // calculates the variation of the analog port

    if (potVar >= varThreshold) {  //sets a threshold for the variance in the pot entiometer state, if it varies more than x it sends the cc message
      pTime[i] = millis(); // stores the previous time
    }
    timer[i] = millis() - pTime[i]; // timer reset
    if (timer[i] < TIMEOUT) { // if the timer value is less than the maximum time allowed, it means that the potentiometer is still moving
      potMoving = true;
    }
    else {
      potMoving = false;
    }

    if (potMoving == true) { // if the pot is still moving, send the control change
      int ccValue = map(potCurrentState[i], 22, 1022, 0, 127);
      if (lastCcValue[i] != ccValue) {
        controlChange(11, cc + i, ccValue); 
        MidiUSB.flush();
      
        potPreviousState[i] = potCurrentState[i]; // stores the current potentiometer value to compare with the next
        lastCcValue[i] = ccValue;
      }
    }
  }

}


// calculates midi channel based on pot position
int potMidiCh () {
  int potCh =  map(mplex.readChannel(muxPotPin[9]), 22, 1023, 0, 4);

  if (potCh == 4) {
    potCh = 3;
  }
  
  return potCh + midiCh;
}

// Arduino (pro)micro midi functions MIDIUSB Library
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}
