/*
 * config.h
 *
 * Created: 6-10-2013 19:12:34
 *  Author: Arne
 */ 


#ifndef CONFIG_H_
#define CONFIG_H_





/**** RFM 12 library for Atmel AVR Microcontrollers *******
 * 
 * This software is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * @author Peter Fuhrmann, Hans-Gert Dahmen, Soeren Heisrath
 */

/******************************************************
 *                                                    *
 *           C O N F I G U R A T I O N                *
 *                                                    *
 ******************************************************/
#define F_CPU 16000000

#define LOG_AVAILABLE /*!< Is Logging available */

//#define UART_AVAILABLE /*!< Is UART available */
#define UART_BAUD_RATE 57600UL /*!< UART Baudrate in bit per second */
#define DUART_RX0_BUFFER_SIZE 128
#define DUART_TX0_BUFFER_SIZE 128

/************************
 * Debug LED for examples
 */

#define LED_433RECEIVE_PORT		PORTA
#define LED_433RECEIVE_DDR		DDRA
#define LED_433RECEIVE_BIT		2

#define LED_433SEND_PORT		PORTA
#define LED_433SEND_DDR			DDRA
#define LED_433SEND_BIT			3

#define LED_868RECEIVE_PORT		PORTA
#define LED_868RECEIVE_DDR		DDRA
#define LED_868RECEIVE_BIT		4

#define LED_868SEND_PORT		PORTA
#define LED_868SEND_DDR			DDRA
#define LED_868SEND_BIT			5

#define LED_DCF77_PORT			PORTA
#define LED_DCF77_DDR			DDRA
#define LED_DCF77_BIT			6

#define LED_LCD_PORT			PORTD
#define LED_LCD_DDR				DDRD
#define LED_LCD_BIT				7

/************************
 * ENC28J60 PIN DEFINITIONS
 */

//Pin that the ENC28J60's slave select is connected to
#define DDR_ETH_SS DDRB
#define PORT_ETH_SS PORTB
#define BIT_ETH_SS 1

/************************
 * 16P25M PIN DEFINITIONS
 */

//Pin that the ENC28J60's slave select is connected to
#define DDR_MEM_SS DDRB
#define PORT_MEM_SS PORTB
#define BIT_MEM_SS 4

/************************
 * RFM12 PIN DEFINITIONS
 */
#define RFM12B_AVAILABLE
//Pin that the RFM12's slave select is connected to
#define DDR_RFM_CS DDRA
#define PORT_RFM_CS PORTA
#define BIT_RFM_CS 1

//Pin that the RFM12's IRQ  is connected to
#define DDR_RFM_IRQ DDRB
#define PORT_RFM_IRQ PORTB
#define PIN_RFM_IRQ PINB
#define BIT_RFM_IRQ 3

//SPI port
#define DDR_SPI DDRB
#define PORT_SPI PORTB
#define PIN_SPI PINB
#define BIT_MOSI 5
#define BIT_MISO 6
#define BIT_SCK  7
#define BIT_SPI_SS 4



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