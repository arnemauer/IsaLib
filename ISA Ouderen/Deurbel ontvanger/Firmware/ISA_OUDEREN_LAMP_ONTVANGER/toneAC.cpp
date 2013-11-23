// ---------------------------------------------------------------------------
// Created by Tim Eckel - teckel@leethost.com
// Copyright 2013 License: GNU GPL v3 http://www.gnu.org/licenses/gpl-3.0.html
//
// See "toneAC.h" for purpose, syntax, version history, links, and more.
// ---------------------------------------------------------------------------
#include "config.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include <avr/pgmspace.h>
#define byte uint8_t
#include "millis.h"
#include "toneAC.h"

unsigned long _tAC_time; // Used to track end note with timer when playing note in the background.

uint8_t _tAC_volume[] = { 200, 100, 67, 50, 40, 33, 29, 22, 11, 2 }; // Duty for linear volume control.



void toneAC(unsigned long frequency, uint8_t volume, unsigned long length, uint8_t background) {
  if (frequency == 0 || volume == 0) { noToneAC(); return; } // If frequency or volume are 0, turn off sound and return.
  if (volume > 10) volume = 10;                              // Make sure volume is in range (1 to 10).

  
  PWMT1DREG |= _BV(PWMT1AMASK) | _BV(PWMT1BMASK); // Set timer 1 PWM pins to OUTPUT (because analogWrite does it too).

  uint8_t prescaler = _BV(CS10);                 // Try using prescaler 1 first.
  unsigned long top = F_CPU / frequency / 2 - 1; // Calculate the top.
  if (top > 65535) {                             // If not in the range for prescaler 1, use prescaler 256 (122 Hz and lower @ 16 MHz).
    prescaler = _BV(CS12);                       // Set the 256 prescaler bit.
    top = top / 256 - 1;                         // Calculate the top using prescaler 256.
  }

  unsigned int duty = top / _tAC_volume[volume - 1]; // Calculate the duty cycle (volume).



  if (length > 0 && background) {  // Background tone playing, returns control to your sketch.

    _tAC_time = millis() + length; // Set when the note should end.
    TIMSK1 |= _BV(OCIE1A);         // Activate the timer interrupt.
  }

  ICR1   = top;                         // Set the top.
  if (TCNT1 > top) TCNT1 = top;         // Counter over the top, put within range.
  TCCR1B = _BV(WGM13)  | prescaler;     // Set PWM, phase and frequency corrected (top=ICR1) and prescaler.
  OCR1A  = OCR1B = duty;                // Set the duty cycle (volume).
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(COM1B0); // Inverted/non-inverted mode (AC).


  if (length > 0 && !background) { 
	  while(length--) {	_delay_ms(1); }
	  noToneAC(); 
	  } // Just a simple delay, doesn't return control till finished.

}

void noToneAC() {
  TIMSK1 &= ~_BV(OCIE1A);     // Remove the timer interrupt.
  TCCR1B  = _BV(CS11);        // Default clock prescaler of 8.
  TCCR1A  = _BV(WGM10);       // Set to defaults so PWM can work like normal (PWM, phase corrected, 8bit).
  PWMT1PORT &= ~_BV(PWMT1AMASK); // Set timer 1 PWM pins to LOW.
  PWMT1PORT &= ~_BV(PWMT1BMASK); // Other timer 1 PWM pin also to LOW.
}

ISR(TIMER1_COMPA_vect) { // Timer interrupt vector.
	//noToneAC();
  if (millis() >= _tAC_time) noToneAC(); // Check to see if it's time for the note to end.
}