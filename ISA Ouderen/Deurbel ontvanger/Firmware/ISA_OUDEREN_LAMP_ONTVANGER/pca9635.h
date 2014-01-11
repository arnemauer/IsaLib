// safety againts double-include
#ifndef pca9635_h
#define pca9635_h

//#include <I2C-master-lib-master/I2C_master.h>

void pca9635_init();
uint8_t pca9635_set_led_pwm(uint8_t ledno, uint8_t cycle);
void pca9635_set_led_mode(uint8_t mode);
uint8_t pca9635_set_sleep(uint8_t sleep);

#endif
// *********** END OF CODE **********
