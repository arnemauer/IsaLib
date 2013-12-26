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
  



