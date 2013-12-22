/*
 * ISA_GATEWAY_QUADROFLY.cpp
 *
 * Created: 6-10-2013 18:48:53
 *  Author: Arne
 */ 

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
#include "delay.c"
#include "millis.h"
#include "toneAC.h"


int main() {
	
	sei();
	        // Initialize library
	        millis_init();
			

//MELODIE AFSPELEN 
/*
  for (int thisNote = 0; thisNote < 8; thisNote++) {
	  int noteDuration = 1000/noteDurations[thisNote];
	  toneAC(melody[thisNote], 10, noteDuration, true); // Play thisNote at full volume for noteDuration in the background.
	  var_delay_ms(noteDuration * 4 / 3); // Wait while the tone plays in the background, plus another 33% delay between notes.
  }
	toneAC(); // Turn off toneAC, can also use noToneAC().
*/
//MELODIE AFSPELEN 

	while(1){ // Stop (so it doesn't repeat forever driving you crazy--you're welcome).
	
	
// WHOOP UP
	byte x;
		for(x=50;x>0;x++){
			toneAC((250*x/20) );
			_delay_ms(30); // Wait a second.
		}
		toneAC(); // Turn off toneAC, can also use noToneAC().
		_delay_ms(500);
// WHOOP UP

_delay_ms(4000);


// WHOOP DOWN
for(x=50;x>0;x--){
	toneAC((250*x/20) );
	_delay_ms(30); // Wait a second.
}
toneAC(); // Turn off toneAC, can also use noToneAC().
_delay_ms(500);
// WHOOP DOWN
		
	
_delay_ms(4000);		
		
		
// DOORBELL
for(x=0;x<2;x++){
toneAC(1500, 10, 500, true); // Play thisNote at full volume for noteDuration in the background.
_delay_ms(500);
toneAC(1200, 10, 500, true); // Play thisNote at full volume for noteDuration in the background.
_delay_ms(1500);
}
//DOORBELL		
	
	_delay_ms(4000);
	
// Phone ring
byte Duration = 1;
//byte x;
byte y;
	for(y=0;y<3;y++){
	      for(x=0;x<(15*Duration);x++)
	      {
			toneAC(1000, 10, 40); // Play thisNote at full volume for noteDuration in the background.
			toneAC(750, 10, 40); // Play thisNote at full volume for noteDuration in the background.
	      }
		  _delay_ms(1200);
	}
			  
		  
	
// Phone ring			
		
		_delay_ms(4000);

         toneAC(1200,10, 50);
         _delay_ms(100);
         toneAC(1200,10, 50);
         _delay_ms(100);
         toneAC(1200,10, 50);
         _delay_ms(200);
         toneAC(1200,10, 300);
         _delay_ms(100);
         toneAC(1200,10, 300);
         _delay_ms(100);
         toneAC(1200,10, 300);
         _delay_ms(200);
         toneAC(1200,10, 50);
         _delay_ms(100);
         toneAC(1200,10, 50);
         _delay_ms(100);
         toneAC(1200,10, 50);
		
		
		
		
		
		_delay_ms(10000);
	}
	
	
	
	
}


// GEBLEVEN BIJ SPI INIT, RFM INITIALISE funtie die verwijst naar SPIINIT();
// Kijken of bij elke build wel een nieuwe hex wordt gemaakt!



/*
#include "config.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdint.h> 
#include <stdio.h> 
#include <inttypes.h> 
#include <string.h>

#define byte uint8_t

#include "ONTVANGER.h"
#include "binary.h"
#include "uart.c"
#include "log.h"
#include "log.c"
#include "notes.h" //notegen.pl output

//Table consists of 4 notes (1/4 duration each)
const int Music[] = {G4, 4, A4, 4, B4, 4 G4, 4, MUSIC_END};
	
	
void setup(){
//	cli(); // Turn all interrupts off!
	sei(); // Turn interrupts on.
		
	_delay_ms(4000); // Wait for startup!
	
	
			
	
	/* Initialize UART */
	/*
	uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU)); 
	//uart0_puts("U");
		_delay_ms(1000);
		

	log_s("ISA - lamp \n\r");

	log_s("STARTUP OK! \n\r");		
		_delay_ms(1000);
		InitMusic();
		PlayMusic();
		
		
}






int main(void)
{
	setup();
    while(1)
    {

   
   log_s(".");
   _delay_ms(500);


	   
    }
}


*/