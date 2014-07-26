/*
 * config.h
 *
 * Created: 6-10-2013 19:12:34
 *  Author: Arne
 */ 


#ifndef CONFIG_H_
#define CONFIG_H_


#include <inttypes.h>



/******************************************************
 *                                                    *
 *           C O N F I G U R A T I O N                *
 *                                                    *
 ******************************************************/
#define F_CPU        8000000UL     // 16MHz processor
#define LOG_AVAILABLE /*!< Is Logging available */

//#define UART_AVAILABLE /*!< Is UART available */
#define UART_BAUD_RATE 19200 /*!< UART Baudrate in bit per second */
//#define DUART_RX0_BUFFER_SIZE 512	
//#define DUART_TX0_BUFFER_SIZE 512


#define byte uint8_t



/************************
 * Debug LED for examples
 */

#define LED_PORT		PORTD
#define LED_DDR		DDRD
#define LED_BIT		7

#define BEL_MELDER_PORT PORTD
#define BEL_MELDER_DDR		DDRD
#define BEL_MELDER_PIN		PIND
#define BEL_MELDER_BIT		3

/************************
 * RFM12 PIN DEFINITIONS
 */
#define RFM12B_AVAILABLE
//Pin that the RFM12's slave select is connected to
#define DDR_RFM_CS DDRB
#define PORT_RFM_CS PORTB
#define BIT_RFM_CS 2

//Pin that the RFM12's IRQ  is connected to

// DO NOT USE THIS, USE INT0! 
#define DDR_RFM_IRQ DDRD
#define PORT_RFM_IRQ PORTD
#define PIN_RFM_IRQ PIND
#define BIT_RFM_IRQ 2

//SPI port
#define DDR_SPI DDRB
#define PORT_SPI PORTB
#define PIN_SPI PINB
#define BIT_MOSI 3
#define BIT_MISO 4
#define BIT_SCK  5
#define BIT_SPI_SS 2



/************************
 * RFM12 CONFIGURATION OPTIONS
 */




/************************
 * RFM12 INTERRUPT VECTOR
 * set the name for the interrupt vector here
 */

/* 
//the interrupt vector
#define RFM12_INT_VECT (PCINT1_vect)

//the interrupt mask register
#define RFM12_INT_MSK PCMSK1

//the interrupt bit in the mask register
#define RFM12_INT_BIT (PCINT11)

//the interrupt flag register
#define RFM12_INT_FLAG PCIFR

//the interrupt bit in the flag register
#define RFM12_FLAG_BIT (PCIF1)

//setup the interrupt to trigger on negative edge
#define RFM12_INT_SETUP()   //MCUCR |= (1<<ISC11)
*/



#endif /* CONFIG_H_ */