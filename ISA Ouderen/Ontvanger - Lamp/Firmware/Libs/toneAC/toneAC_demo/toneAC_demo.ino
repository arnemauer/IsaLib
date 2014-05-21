// ---------------------------------------------------------------------------
// Connect your piezo buzzer (without internal oscillator) or speaker to these pins:
//   Pins  9 & 10 - ATmega328, ATmega128, ATmega640, ATmega8, Uno, Leonardo, etc.
//   Pins 11 & 12 - ATmega2560/2561, ATmega1280/1281, Mega
//   Pins 12 & 13 - ATmega1284P, ATmega644
//   Pins 14 & 15 - Teensy 2.0
//   Pins 25 & 26 - Teensy++ 2.0
// Be sure to include an inline 100 ohm resistor on one pin as you normally do when connecting a piezo or speaker.
// ---------------------------------------------------------------------------

#include <toneAC.h>

// Melody liberated from the toneMelody Arduino example sketch by Tom Igoe.
int melody[] = { 262, 261,260,259,258, 240 };
int noteDurations[] = { 1,1,1,1,1, 5 };

void setup() {} // Nothing to setup, just start playing!

void loop() {
 
  /*
  for (unsigned long freq = 125; freq <= 15000; freq += 10) {  
    toneAC(freq); // Play the frequency (125 Hz to 15 kHz sweep in 10 Hz steps).
    delay(1);     // Wait 1 ms so you can hear it.
  }
  */
  toneAC(); // Turn off toneAC, can also use noToneAC().

  delay(1000); // Wait a second.
  
  while(1){ // Stop (so it doesn't repeat forever driving you crazy--you're welcome).
/*
  for (unsigned long freq = 1500; freq > 1400; freq--) {  
    toneAC(freq); // Play the frequency (125 Hz to 15 kHz sweep in 10 Hz steps).
    delay(5);     // Wait 1 ms so you can hear it.
  }
    toneAC(); // Turn off toneAC, can also use noToneAC().

   delay(1000); // Wait a second.
*/
byte x;


for(x=50;x>0;x++){
            toneAC((250*x/20) );
              delay(40); // Wait a second.

        }        

    toneAC(); // Turn off toneAC, can also use noToneAC().


  //  toneAC(1500, 10, 300); // Play thisNote at full volume for noteDuration in the background.
  //  toneAC(1200, 10, 400); // Play thisNote at full volume for noteDuration in the background.
   delay(10000); // Wait a second.


   
         
         
  /*for (int thisNote = 0; thisNote < 8; thisNote++) {
    int noteDuration = 1000/noteDurations[thisNote];
    toneAC(melody[thisNote], 10, noteDuration, true); // Play thisNote at full volume for noteDuration in the background.
    delay(noteDuration * 4 / 3); // Wait while the tone plays in the background, plus another 33% delay between notes.
  }
   delay(1000); // Wait a second.
*/
  }
}
