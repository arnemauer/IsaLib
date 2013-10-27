#ifndef SED1531_ATMEGA1284P_H
#define SED1531_ATMEGA1284P_H

/*
    This file defines the pin mapping between the GLCD and the Arduino

    This file is part of the SED1531 Arduino library.

    SED1531 Arduino library Copyright (c) 2013 Tom van Zutphen
    http://www.tkkrlab.nl/wiki/Glcd_48x100

    This library is largely based on / ported from from the
    library (c) 2013 by Peter van Merkerk
    http://sourceforge.net/p/glcdsed1531lib/wiki/Home/

    I adapted it for use with Arduino and added some commands to draw circles,
    make it compatible with the arduino print command and add collision detect
    for all drawing functions except text.

    The SED1531 Arduino library is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as published
    by the Free Software Foundation, either version 3 of the License, or (at
    your option) any later version.

    The SED1531 Arduino library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
    Public License for more details.

    You should have received a copy of the GNU General Public License along with
    the SED1531 Arduino library If not, see http://www.gnu.org/licenses/.
*/

/*
    by default this library uses a different pin mapping for the data pins then the other code examples!

    Port and pin mapping for ATMEGA1284P @ 16MHz:

    AVR      	GLCD
    -----    	----
    PD6 (20)   	A0 (4)
    PC7 (29)   	RW (5)
    PC0 (22)    E  (6)
	
	PC6 (28)	D7 (7)
	PD5 (19)	D6 (8)
	PC5	(27)	D5 (9)
	PC1	(23)	D4 (10)
	PC4	(26)	D3 (11)
	PD4	(18)	D2 (12)
	PC3	(25)	D1 (13)
	PC2	(24)	D0 (14)
		

*/


#define GLCD_IO_INIT()	DDRC |= (1 << 0) | (1 << 7); DDRD |= (1 << 6) // PD6 (AO) is output


#define GLCD_IO_PIN_A0_0()              PORTD &= ~(1 << 6)
#define GLCD_IO_PIN_A0_1()              PORTD |= (1 << 6)

#define GLCD_IO_PIN_RW_0()              PORTC &= ~( 1 << 7)
#define GLCD_IO_PIN_RW_1()              PORTC |= (1 << 7)

#define GLCD_IO_PIN_E_0()               PORTC &= ~(1 << 0)
#define GLCD_IO_PIN_E_1()               PORTC |= (1 << 0)

#define GLCD_IO_DATA_DIR_INPUT() DDRC &= ~((1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6)); DDRD &= ~((1 << 4) | (1 << 5))
// pins input

// pins input
#define GLCD_IO_DATA_DIR_OUTPUT()	DDRC |= (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6); DDRD |= (1 << 4) | (1 << 5)


/*
 * Option 1:
 * GLCD D0..D1 = Arduino PIN8..PIN9 (Atmega328 PB0..PB1)
 * GLCD D2..D7 = Arduino PIN2..7 (Atmega328 PD2..PD7)
 */
					//cli(); WHY?! 
//(PC2>D0 en PC3>D1), (PC4>D3), (PC1>D4), (PC5>D5), PC6>D7)     
// (PD4>D2), (PD5>D6)                               
#define GLCD_IO_DATA_OUTPUT(data)		uint8_t register saveSreg = SREG; PORTC = (PORTB & 0B01111110) | ((data&0B11)<<2) | ((data&0B1000) <<1) | ((data&0B10000)>>3) | (data&0B100000) | ((data&0b10000000)>>1) ; PORTD = (PORTD & 0B11001111) | ((data&0B01000000)>> 1) | ((data&0B00000100)<< 2); SREG=saveSreg
												// D1 D0				// D2					// D3					// D4			// D5				// D6					// D7
#define GLCD_IO_DATA_INPUT()           (((PINC &0B1100)>>2) | ((PIND &0B10000)>>2) | ((PINC &0B10000)>>2) | ((PINC &0B10)<<3) | (PINC &0B100000) | ((PIND &0b100000)<<1) | ((PINC &0B1000000)<<1))


/*
 * Option 2:
 * GLCD D6..D7 = Arduino PIN8..PIN9 (ATmega328 PB0..PB1)
 * GLCD D0..D5 = Arduino PIN2..7 (ATmega328 PD2..PD7)
 */
//#define GLCD_IO_DATA_OUTPUT(data)		uint8_t register saveSreg = SREG; \
										cli();                                      \
										PORTB = (PORTB & 0B11111100) | (data>>6); \
										PORTD = (PORTD & 0B11) | (data<<2); \
										SREG=saveSreg
//#define GLCD_IO_DATA_INPUT()           (((PIND & 0B11111100)>>2) | ((PINB & 0B11)<<6))


#define GLCD_IO_DELAY_READ()           __asm volatile ("nop\nnop\nnop")					//each nop = 62,5ns @16mhz
//#define GLCD_IO_DELAY_WRITE()

#endif // #ifndef SED1531_ATMEGA1284P_H
