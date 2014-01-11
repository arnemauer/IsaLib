/*
 * tone.c
 *
 * Created: 1/11/2014 3:22:29 PM
 *  Author: arnefrankmauer
 */ 

#include "toneAC.h"

uint8_t _tAC_volume[] = { 200, 100, 67, 50, 40, 33, 29, 22, 11, 2 }; // Duty for linear volume counsigned long _tAC_time;ntrol.

void tone_init(){
		DDRB |= (1 << 1); // Set pin PB1 to OUTPUT
	
}

void tone(unsigned long frequency, uint8_t volume) {
	if (frequency == 0 || volume == 0) { noTone(); return; } // If frequency or volume are 0, turn off sound and return.
	if (volume > 10) volume = 10;                              // Make sure volume is in range (1 to 10).
		

		uint8_t prescaler = _BV(CS10);                 // Try using prescaler 1 first.
		unsigned long top = F_CPU / frequency / 2 - 1; // Calculate the top.
		if (top > 65535) {                             // If not in the range for prescaler 1, use prescaler 256 (122 Hz and lower @ 16 MHz).
			prescaler = _BV(CS12);                       // Set the 256 prescaler bit.
			top = top / 256 - 1;                         // Calculate the top using prescaler 256.
		}
	
		unsigned int duty = top / _tAC_volume[volume - 1]; // Calculate the duty cycle (volume).
		
		ICR1   = top;							// Set the top.
		if (TCNT1 > top) TCNT1 = top;			// Counter over the top, put within range.
			TCCR1B = _BV(WGM13)  | prescaler;   // Set PWM, phase and frequency corrected (top=ICR1) and prescaler.
			OCR1A  = duty;						// Set the duty cycle (volume).
			TCCR1A = _BV(COM1A1);				// Clear OC1A/OC1B on Compare Match (Set output to low level).

}


	void noTone() {
	//	TIMSK1 &= ~_BV(OCIE1A);     // Remove the timer interrupt.
		 TCCR1B  &= ~(1<<0) | ~(1<<1) | ~(1<<2) ;        // No clock source (Timer/Counter stopped).
		// TCCR1A  = _BV(WGM10);       // Set to defaults so PWM can work like normal (PWM, phase corrected, 8bit).
		PORTB &= ~(1 << 1); // Set pin PB1 to LOW.
		//PWMT1PORT &= ~_BV(PWMT1BMASK); // Other timer 1 PWM pin also to LOW.
	}


		ISR(TIMER1_COMPA_vect) { // Timer interrupt vector.
			if (millis() >= _tAC_time) noToneAC(); // Check to see if it's time for the note to end.
		}

	
	