#ifndef SED1531_h
#define SED1531_h
/*
    Target independent implementation of the SED1531 Arduino library.
    The dependent mappings of the I/O pins are included in the file SED1531_arduino.h

    This file is part of the SED1531 Arduino library.

    SED1531 Arduino library Copyright (c) 2013 Tom van Zutphen
    http://www.tkkrlab.nl/wiki/Glcd_48x100

    This library is largely based on / ported from from the
    library (c) 2013 by Peter van Merkerk
    http://sourceforge.net/p/glcdsed1531lib/wiki/Home/

    I adapted it for use with Arduino and added some commands to draw circles,
    make it compatible with the arduino print command and add collision detect
    for all drawing functions except text.

    The SED1531 Arduino library is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as published
    by the Free Software Foundation, either version 3 of the License, or (at
    your option) any later version.

    The SED1531 Arduino library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
    Public License for more details.

    You should have received a copy of the GNU General Public License along with
    the SED1531 Arduino library If not, see http://www.gnu.org/licenses/.
*/

//#include "GLCD_SED1531/SED1531_ATMEGA1284P.h"
#include <avr/pgmspace.h>

/* Fallback defines */

#if !defined(GLCD_IO_DELAY_READ)
#define GLCD_IO_DELAY_READ()
#endif

#if !defined(GLCD_IO_DELAY_WRITE)
#define GLCD_IO_DELAY_WRITE()
#endif

/* Types */
typedef struct glcd_font_struct
{
    uint8_t first;
    uint8_t interspacing;
    uint8_t fixedwidth;
    const PROGMEM char* lookup;
} glcd_font_t;

typedef const glcd_font_t* PROGMEM glcd_font_ptr;

/* GLCD Commands */
#define GLCD_CMD_STATIC_INDICATOR_OFF           (0xAC)
#define GLCD_CMD_STATIC_INDICATOR_ON            (0xAD)
#define GLCD_CMD_DISPLAY_OFF                    (0xAE)
#define GLCD_CMD_DISPLAY_ON                     (0xAF)
#define GLCD_CMD_INITIAL_DISPLAY_LINE(start)    (0x40 | (start))
#define GLCD_CMD_SET_PAGE(page)                 (0xB0 | (page))
#define GLCD_CMD_SET_COLUMN_HIGH(col_high)      (0x10 | (col_high))
#define GLCD_CMD_SET_COLUMN_LOW(col_low)        (col_low)
#define GLCD_CMD_ADC_NORMAL                     (0xA0)
#define GLCD_CMD_ADC_REVERSE                    (0xA1)
#define GLCD_CMD_READ_MODIFY_WRITE              (0xE0)
#define GLCD_CMD_DISPLAY_NORMAL                 (0xA6)					//normal display pixels
#define GLCD_CMD_DISPLAY_REVERSE                (0xA7)					//inverted display pixels
#define GLCD_CMD_ENTIRE_DISPLAY_NORMAL          (0xA4)					//display shows data from display ram
#define GLCD_CMD_ENTIRE_DISPLAY_ON              (0xA5)					//all pixels on (does not change display ram)
#define GLCD_CMD_LCD_BIAS_1_8                   (0xA2)
#define GLCD_CMD_LCD_BIAS_1_6                   (0xA3)
#define GLCD_CMD_READ_MODIFY_WRITE              (0xE0)
#define GLCD_CMD_END                            (0xEE)
#define GLCD_CMD_RESET                          (0xE2)
#define GLCD_CMD_POWER_NONE                     (0x28)
#define GLCD_CMD_POWER_FOLLOWER                 (0x29)
#define GLCD_CMD_POWER_REGULATOR                (0x2A)
#define GLCD_CMD_POWER_BOOSTER                  (0x2C)
#define GLCD_CMD_POWER_ALL                      (0x2F)
#define GLCD_CMD_ELECTRONIC_CONTROL(voltage)    (0x80 | (voltage))		//change display contrast

/* Colors */
#define GLCD_COLOR_CLEAR                        (0x00)
#define GLCD_COLOR_SET                          (0x01)
#define GLCD_COLOR_INVERT                       (0x02)
#define GLCD_COLOR_COLLISION                    (0x03)		//used for collision detect, no actual drawing
#define GLCD_COLOR_FILLED                       (0x04)		//add to CLEAR/SET/INVERT for a filled circle

/* GLCD Status bits */
#define GLCD_STATUS_BUSY                        (0x80)
#define GLCD_STATUS_ADC                         (0x40)
#define GLCD_STATUS_ON_OFF                      (0x20)
#define GLCD_STATUS_RESET                       (0x10)

/* Indicators */
#define GLCD_INDICATOR_0                        (20)
#define GLCD_INDICATOR_1                        (31)
#define GLCD_INDICATOR_2                        (32)
#define GLCD_INDICATOR_3                        (57)
#define GLCD_INDICATOR_4                        (69)
#define GLCD_INDICATOR_5                        (78)

class SED1531 {
	public:
		void init();
		void standby();
		void sleep();
		void wakeup();
		void command(uint8_t);
		void setCursor(uint8_t column, uint8_t row);
		void setFont(glcd_font_ptr font);
		void setIndicator(uint8_t indicator, uint8_t enable);
		uint8_t drawBox(uint8_t left, uint8_t top, uint8_t width, uint8_t height, uint8_t color);
		void clearDisplay();
		uint8_t drawPixel(uint8_t x, uint8_t y, uint8_t color);
		uint8_t drawFrame(uint8_t left, uint8_t top, uint8_t width, uint8_t height, uint8_t color, uint8_t line_width);
		uint8_t drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);
		uint8_t drawCircle(uint8_t x, uint8_t y, uint8_t radius, uint8_t color);
		
		virtual size_t write(uint8_t text);
		
		uint8_t drawText(uint8_t page, uint8_t column, glcd_font_ptr font, const char* text);
		uint8_t drawText_P(uint8_t page, uint8_t column, glcd_font_ptr font, const PROGMEM char* text);
		uint8_t drawText(uint8_t page, uint8_t column, glcd_font_ptr font, char text);
	private:
		void pageUpdateStart(uint8_t page, uint8_t column);
		void pageUpdateEnd();
		void updateBuffer_P(const PROGMEM char* buffer, uint8_t count, uint8_t pixelshift=0, uint8_t mask=0);
		uint8_t drawText_int(uint8_t page, uint8_t column, glcd_font_ptr font, const char* text, uint8_t text_in_flash);
		void send(uint8_t data);
		uint8_t updateData(uint8_t pixels, uint8_t mask);
		void sendRepeated(uint8_t data, uint8_t count);

		uint8_t _currcolumn,_currpage;
		glcd_font_ptr _font;

};
#endif
