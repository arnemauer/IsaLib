#include "I2C_master.h"
#include <stdint.h>
#include "pca9635.h"


#define device_address 0x70; // Default to the all-call address
#define autoincrement_bits 0x80; // Autoincrement all

void pca9635_init(){
  I2C_start((0x40<<1)+I2C_WRITE);
  
  //Autoincrement ALL registers, start at reg 0 - we're initting
  I2C_write(0x80);
  
  //Reg 0x00 = MODE1 - set to 0x80 - autoincrement enabled (this is readonly?), do not respond to subaddresses or allcall
  I2C_write(0x80);
  
  //Reg 0x01 = MODE2 - set to 0x02 - noninverted logic state, open-drain
  I2C_write(0x02);
  
  //Reg 0x02-0x11 - PWM0-15 - LED brightnesses, start at low brightness (0x20)
  
  I2C_write(0x01);
  I2C_write(0x01);
  I2C_write(0x01);
  I2C_write(0x01);
  I2C_write(0x01);
  I2C_write(0x01);
  I2C_write(0x01);
  I2C_write(0x01);
  I2C_write(0x01);
  I2C_write(0x01);
  I2C_write(0x01);
  I2C_write(0x01);
  I2C_write(0x01);
  I2C_write(0x01);
  I2C_write(0x01);
  I2C_write(0x01);
  
  //Reg 0x12 - Group PWM - should not matter
  I2C_write(0xff);

  //Reg 0x13 - Group Freq - should not matter
  I2C_write(0x00);

  //Reg 0x14-0x17 - LED Output State Control - all 0xAA (output state for each LED = 0x2)
  I2C_write(0xaa);
  I2C_write(0xaa);
  I2C_write(0xaa);
  I2C_write(0xaa);

  //Reg 0x18-0x1b - Subaddressing stuff, doesn't matter, just stop the xfer
  I2C_stop();
  
}


/**
* Sets the pwm value for given led, note that it must have previously been enabled for PWM control with set_mode
*
* Remember that led numbers start from 0
*/
uint8_t pca9635_set_led_pwm(uint8_t ledno, uint8_t cycle)
{
	  I2C_start((0x40<<1)+I2C_WRITE);
	uint8_t reg = 0x02 + ledno;
	// 0x80 = autoincrementbits
	I2C_write((reg | 0x80) );
	I2C_write(cycle);
	 I2C_stop();
	return 1;
}
  
  
  /**
  * Sets the pwm value for all leds, note that it must have previously been enabled for PWM control with set_mode
  *
  * 
  */
  void pca9635_set_all_led_pwm( uint8_t cycle )
  {
	  I2C_start((0x40<<1)+I2C_WRITE);
	  I2C_write(0x82); // 10000010 autoincrement + led register one
	  for(uint8_t i = 0; i <= 15; i++){
		I2C_write(cycle);
	  }
	  I2C_stop();
	  return;
  }
  

/**
* Set mode for all leds
* 0=fully off
* 1=fully on (no PWM)
* 2=individual PWM only
* 3=individual and group PWM
*/
void pca9635_set_led_mode(uint8_t mode)
{
	uint8_t value;
	switch (mode)
	{
		case 0:
		value = 0x00; // B00000000
		break;
		case 1:
		value = 0x55; //B01010101
		break;
		case 2:
		value = 0xAA; //B10101010
		break;
		case 3:
		value = 0xFF; //B11111111
		break;
	}
	
	I2C_start((0x40<<1)+I2C_WRITE);
  	I2C_write( 0x94 ); // LEDOUT0 = 0x14 = 10100, autoinc. 0x80 = 10000000, Total= 10010100,
  	for (uint8_t i = 0; i <= 3; ++i){
	   I2C_write(value);	
  	}
			  
  	I2C_stop();
		  
}

/**
* Changes the oscillator mode between sleep (1) and active (0)
*/
uint8_t pca9635_set_sleep(uint8_t sleep)
{
	I2C_start((0x40<<1)+I2C_READ);
	I2C_write(0x80); // autoincrement, mode 0,
	// I2C_stop(); ???nodig?
	uint8_t data = I2C_read_ack();
	
	if(sleep){		// set or unset bit 4
		data |= (1<<4);
	}else{
		data &= ~(1<<4);
	}
	
	uint8_t ack = I2C_write(data);  
	I2C_stop();
	return ack;
}


