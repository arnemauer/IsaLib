/*
 * doorbell.h
 *
 * Created: 7/19/2014 4:02:23 PM
 *  Author: arnefrankmauer
 */ 


#ifndef DOORBELL_H_
#define DOORBELL_H_

extern uint8_t triggered;
extern uint8_t doorbell_last_state;

/**
 * Poll the specified uint16_t out the log port.
 *
 * @param i The specified uint16_t
 */

#ifdef __cplusplus
extern "C" {
	#endif
	
void doorbell_init(void);
void doorbell_enable_interrupt(void);
void doorbell_disable_interrupt(void);
void doorbell_interrupt(void);


#ifdef __cplusplus
}
#endif


#endif /* DOORBELL_H_ */