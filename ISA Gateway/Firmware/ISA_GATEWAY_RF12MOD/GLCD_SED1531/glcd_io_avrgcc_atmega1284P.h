#ifndef INCLUDED_GLCD_IO_AVRGCC_ATMEGA1284P_H
#define INCLUDED_GLCD_IO_AVRGCC_ATMEGA1284P_H

/*
    This file defines the pin mapping between the GLCD and the ATtiny2313
    using AVR-GCC and AVR Libc. This file should be included before glcd.h. 
    This file also defines some macro's for dealing with data to be stored
    in flash memory.

    Copyright 2013 Peter van Merkerk

    This file is part of the GLCD SED1531 library.

    The GLCD SED1531 library is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as published
    by the Free Software Foundation, either version 3 of the License, or (at
    your option) any later version.

    The GLCD SED1531 library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
    Public License for more details.

    You should have received a copy of the GNU General Public License along with
    the GLCD SED1531 library If not, see http://www.gnu.org/licenses/.
*/

/*
    Port and pin mapping for ATtiny2313 @ 1MHz:

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
#include <avr/io.h>

static void GLCD_IO_INIT(){       
DDRC |= (1 << 0) | (1 << 7); //pc0 (E) and PC7 (RW) are outputs
DDRD |= (1 << 6); // PD6 (AO) is output
}

#define GLCD_IO_PIN_A0_0()              PORTD &= ~(1 << 6)
#define GLCD_IO_PIN_A0_1()              PORTD |= (1 << 6)

#define GLCD_IO_PIN_RW_0()              PORTC &= ~( 1 << 7)
#define GLCD_IO_PIN_RW_1()              PORTC |= (1 << 7)

#define GLCD_IO_PIN_E_0()               PORTC &= ~(1 << 0)
#define GLCD_IO_PIN_E_1()               PORTC |= (1 << 0)

int lcdDataPins[] = {9,8,7,6,5,4,3,2};
char lcdDataPorts[] = {'C','D','C','C','C','D','C','C'};
/*
  Put DATA to display (pixel data)
*/
void GLCD_IO_DATA_OUTPUT(byte lcdData) {
  byte data = lcdData;
 
  for (int i = 7; i >= 0 ; i--) {
	if(data & 0x1){
		if(lcdDataPorts[i] == 'C'){
			PORTC |= (1 << lcdDataPins[i]);
		}else{
			PORTD |= (1 << lcdDataPins[i]);
		}
	}else{
		if(lcdDataPorts[i] == 'C'){
			PORTC &= ~(1 << lcdDataPins[i]);
		}else{
			PORTD &= ~(1 << lcdDataPins[i]);
		}
	}

    data = data >> 1;
  }

}

byte GLCD_IO_DATA_INPUT() {
byte data;
 
  for (int i = 7; i >= 0 ; i--) {
		if(lcdDataPorts[i] == 'C'){
			if((PINC & (1 << lcdDataPins[i]))){
				data |= (1 << lcdDataPins[i]);
			}else{
				data &= ~(1 << lcdDataPins[i]);
			}
		}else{
			if((PIND & (1 << lcdDataPins[i]))){
				data |= (1 << lcdDataPins[i]);
			}else{
				data &= ~(1 << lcdDataPins[i]);
			}		
		}
	}
return data;
}          

static void GLCD_IO_DATA_DIR_INPUT(){
DDRC &= ~((1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6)); // pins input
DDRD &= ~((1 << 4) | (1 << 5));
}


static void GLCD_IO_DATA_DIR_OUTPUT(){           
DDRC |= (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6); // pins input
DDRD |= (1 << 4) | (1 << 5);
}

#define GLCD_IO_DELAY_READ()            __asm volatile ("nop")

/* AVR-GCC Flash definitions */
#include <avr/pgmspace.h>

#define GLCD_FLASH(type, name)          const type const name PROGMEM
#define GLCD_FLASH_READ_BYTE(address)   pgm_read_byte(address)         
#define GLCD_FLASH_READ_WORD(address)   pgm_read_word(address) 
#define GLCD_FLASH_PTR(type)            const type* PROGMEM

#endif // #ifndef INCLUDED_GLCD_IO_AVRGCC_ATMEGA1284P_H