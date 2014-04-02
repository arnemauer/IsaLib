#ifndef I2C_MASTER_H
#define I2C_MASTER_H

#include <stdint.h>

#define I2C_READ 0x01
#define I2C_WRITE 0x00
#define byte uint8_t

void I2C_init(void);
uint8_t I2C_start(uint8_t address);
uint8_t I2C_write(uint8_t data);
uint8_t I2C_read_ack(void);
uint8_t I2C_read_nack(void);
void I2C_stop(void);

#endif // I2C_MASTER_H
