#include "pca9635.h"

#define device_address = 0x70; // Default to the all-call address
#define autoincrement_bits = 0x80; // Autoincrement all

void pca9635_init(){
  i2c_start((0x60<<1)+I2C_WRITE);
  
  //Autoincrement ALL registers, start at reg 0 - we're initting
  i2c_write(0x80);
  
  //Reg 0x00 = MODE1 - set to 0x80 - autoincrement enabled (this is readonly?), do not respond to subaddresses or allcall
  i2c_write(0x80);
  
  //Reg 0x01 = MODE2 - set to 0x02 - noninverted logic state, open-drain
  i2c_write(0x02);
  
  //Reg 0x02-0x11 - PWM0-15 - LED brightnesses, start at low brightness (0x20)
  
  i2c_write(0x20);
  i2c_write(0x20);
  i2c_write(0x20);
  i2c_write(0x20);
  i2c_write(0x20);
  i2c_write(0x20);
  i2c_write(0x20);
  i2c_write(0x20);
  i2c_write(0x20);
  i2c_write(0x20);
  i2c_write(0x20);
  i2c_write(0x20);
  i2c_write(0x20);
  i2c_write(0x20);
  i2c_write(0x20);
  i2c_write(0x20);
  
  //Reg 0x12 - Group PWM - should not matter
  i2c_write(0xff);

  //Reg 0x13 - Group Freq - should not matter
  i2c_write(0x00);

  //Reg 0x14-0x17 - LED Output State Control - all 0xAA (output state for each LED = 0x2)
  i2c_write(0xaa);
  i2c_write(0xaa);
  i2c_write(0xaa);
  i2c_write(0xaa);

  //Reg 0x18-0x1b - Subaddressing stuff, doesn't matter, just stop the xfer
  i2c_stop();
  
}

  



