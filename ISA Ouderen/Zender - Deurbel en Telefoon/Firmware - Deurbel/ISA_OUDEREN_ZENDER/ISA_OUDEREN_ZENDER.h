/*
 * ISA_GATEWAY_QUADROFLY.h
 *
 * Created: 12-10-2013 17:10:13
 *  Author: Arne
 */ 


#ifndef ISA_GATEWAY_QUADROFLY_H_
#define ISA_GATEWAY_QUADROFLY_H_






uint8_t deep_sleep_ok; // if true timer stopped everything so uc can go in deep sleep



void sendpackage();
static byte waitForAck();
#endif /* ISA_GATEWAY_QUADROFLY_H_ */
