// safety againts double-include
#ifndef pca9635_h
#define pca9635_h

//#include <I2C-master-lib-master/I2C_master.h>

void pca9635_init();
uint8_t pca9635_set_led_pwm(uint8_t ledno, uint8_t cycle);
void pca9635_set_led_mode(uint8_t mode);
uint8_t pca9635_set_sleep(uint8_t sleep);
void pca9635_set_led_pwm_flash(uint8_t ledno, uint8_t cycle0, uint8_t cycle1, uint8_t cycle2, uint8_t cycle3 );

#endif
// *********** END OF CODE **********
