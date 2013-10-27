/*
    Target independent implementation of the SED1531 Arduino library.
    The dependent mappings of the I/O pins must be included before this
    file.

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

//#include "Arduino.h"
#include "GLCD_SED1531/SED1531.h"
#include "GLCD_SED1531/SED1531_ATMEGA1284P.h"
#include <util/delay.h>

/**
 * Intialize display. This function must be called before any
 * other glcd function.
 */
void SED1531::init()
{
	static const char init_data[] PROGMEM = {
        GLCD_CMD_RESET,
        GLCD_CMD_ADC_REVERSE,
        GLCD_CMD_LCD_BIAS_1_8,
        GLCD_CMD_POWER_ALL,
        GLCD_CMD_DISPLAY_NORMAL,
        GLCD_CMD_ENTIRE_DISPLAY_NORMAL,
        GLCD_CMD_DISPLAY_ON,
//        GLCD_CMD_INITIAL_DISPLAY_LINE(0),		//GLCD_CMD_RESET takes care of this already, see datasheet
        GLCD_CMD_ELECTRONIC_CONTROL(15)
    };
    GLCD_IO_INIT();
	GLCD_IO_DATA_DIR_OUTPUT();
//	GLCD_IO_PIN_A0_0();							//not needed after reset
	_delay_ms(100);
    updateBuffer_P(init_data, sizeof(init_data));
}

/**
 * Send command to display. The possible commands are listed in the header file SED1531.h.
 */
void SED1531::command(uint8_t command){
	GLCD_IO_PIN_A0_0();
	send(command);
	}

/**
 * Enabled, disable or toggle and indicator.
 *
 * @param indicator Indicator to change (use one of the GLCD_INDICATOR_* defines listed in the header file SED1531.h).
 * @param enable    0 = disable, 1 = enable, 2 = toggle.
 */
void SED1531::setIndicator(uint8_t indicator, uint8_t enable)
{
    drawPixel(indicator, 48, enable);
}

/**
 * Clear contents of entire display, indicators included.
 */
void SED1531::clearDisplay()
{
    drawBox(0, 0, 132, 64, 0);
}

/**
 * Enable, disable or toggle a single pixel.
 */
uint8_t SED1531::drawPixel(uint8_t x, uint8_t y, uint8_t color)
{
   return drawBox(x, y, 1, 1, color);
}

/**
 * Draw a filled rectangle.
 */
uint8_t SED1531::drawBox(uint8_t left, uint8_t top, uint8_t width, uint8_t height, uint8_t color)
{
    uint8_t x;
    uint8_t mask;
    uint8_t data = 0x00;
    uint8_t page = top / 8;
    uint8_t bit = 1 << (top % 8);
    uint8_t collision=0;

    while(height != 0)
    {
        data |= bit;
        --height;
        bit <<= 1;

        if(bit == 0 || height == 0)
        {
            mask = color != GLCD_COLOR_INVERT ? ~data : 0xFF;

            if(color == GLCD_COLOR_CLEAR)
            {
                data = 0;
            }

            pageUpdateStart(page, left);
            if(mask == 0x00)
            {
                /* The entire byte is to be overwritten so
                   don't bother with read-modify-write. */
                GLCD_IO_PIN_A0_1();
                sendRepeated(data, width);
            }
            else
            {
                if(color == GLCD_COLOR_COLLISION)
                 {
                	mask = 0x00;	//only detect collision, do not write anything
                 }

                for(x = 0; x < width; ++x)
                {
                    if (updateData(data, mask)) collision=1;
                }
            }

            pageUpdateEnd();

            ++page;
            data = 0x00;
            bit = 0x01;
        }
    }
    return collision;
}

/**
 * Draw unfilled rectangle.
 */
uint8_t SED1531::drawFrame(uint8_t left, uint8_t top, uint8_t width, uint8_t height, uint8_t color, uint8_t line_width)
{
    uint8_t collision;
    collision = drawBox(left, top, width, line_width, color);
    collision |= drawBox(left, top, line_width, height,color);
    collision |= drawBox(left, top + height - line_width, width, line_width, color);
    collision |= drawBox(left + width - line_width, top, line_width, height, color);
    return collision;
}

/**
 * Draw line from (x1,y1) to (x2,y2) (inclusive).
 */
uint8_t SED1531::drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
    char xinc1;
    char xinc2;
    char yinc1;
    char yinc2;
    uint8_t deltax;
    uint8_t deltay;
    uint8_t numpixels;
    uint8_t numadd;
    uint8_t num;
    uint8_t den;
    uint8_t i;
    uint8_t collision=0;

    uint8_t  x = x1;
    uint8_t  y = y1;

    if (x2 >= x1)
    {
        deltax = x2 - x1;
        xinc1 = 1;
    }
    else
    {
        deltax = x1 - x2;
        xinc1 = -1;
    }

    xinc2 = xinc1;

    if (y2 >= y1)
    {
        deltay = y2 - y1;
        yinc1 = 1;
    }
    else
    {
        deltay = y1 - y2;
        yinc1 = -1;
    }

    yinc2 = yinc1;

    if (deltax >= deltay)
    {
        xinc1 = 0;
        yinc2 = 0;
        numpixels = deltax;
        numadd = deltay;
    }
    else
    {
        xinc2 = 0;
        yinc1 = 0;
        numpixels = deltay;
        numadd = deltax;
    }

    den = numpixels;
    num = den / 2;

    for (i = 0; i <= numpixels; ++i)
    {
    	collision |= drawPixel(x, y, color);
        num += numadd;

        if (num >= den)
        {
            num -= den;
            x += xinc1;
            y += yinc1;
        }

        x += xinc2;
        y += yinc2;
    }
    return collision;
}

/**
 * Draw circle with center (x0,y0) and radius radius.
 * add GLCD_COLOR_FILLED to the variable color for a filled circle
 */
uint8_t SED1531::drawCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint8_t color)
  {
    char f = 1 - radius;
    char ddF_x = 1;
    char ddF_y = -2 * radius;
    char x = 0;
    char y = radius;
    uint8_t collision;
    bool filled = color & GLCD_COLOR_FILLED;
    if (filled){
    	color^=GLCD_COLOR_FILLED;
    	collision = drawBox(x0, y0 - radius, 1, radius * 2+1, color);
    }
    else{
    collision = drawPixel(x0, y0 + radius, color);
    collision |= drawPixel(x0, y0 - radius, color);
    collision |= drawPixel(x0 + radius, y0, color);
    collision |= drawPixel(x0 - radius, y0, color);
    }
    while(x < y)
    {
      if(f >= 0)
      {
  	    if (filled){
  	    	collision |= drawBox(x0 + y, y0 - x, 1, 2 * x+1, color);
  	    	collision |= drawBox(x0 - y, y0 - x, 1, 2 * x+1, color);
  	    }
        y--;
        ddF_y += 2;
        f += ddF_y;
      }
      x++;
      ddF_x += 2;
      f += ddF_x;
      if (x<=y) {
    	    if (filled){
    	    	collision |= drawBox(x0 + x, y0 - y, 1, 2 * y+1, color);
    	    	collision |= drawBox(x0 - x, y0 - y, 1, 2 * y+1, color);
    	    }
    	    else{
    	    	collision |= drawPixel(x0 + x, y0 + y, color);
    	    	collision |= drawPixel(x0 - x, y0 + y, color);
    	    	collision |= drawPixel(x0 + x, y0 - y, color);
    	    	collision |= drawPixel(x0 - x, y0 - y, color);
    	    	collision |= drawPixel(x0 + y, y0 + x, color);
    	    	collision |= drawPixel(x0 - y, y0 + x, color);
    	    	collision |= drawPixel(x0 + y, y0 - x, color);
    	    	collision |= drawPixel(x0 - y, y0 - x, color);
    	    }
      }
    }
    return collision;
}

uint8_t SED1531::drawText_int(uint8_t page, uint8_t column, glcd_font_ptr font, const char* text, uint8_t text_in_flash)
{
	uint8_t pixelshift = page%8;
	page=page/8;
    pageUpdateStart(page, column);
    GLCD_IO_PIN_A0_1();

    uint8_t first = pgm_read_byte(&(font->first));
    uint8_t  interspacing = pgm_read_byte(&(font->interspacing));
    uint8_t fixedwidth = pgm_read_byte(&(font->fixedwidth));
    const PROGMEM char** lookup = (const PROGMEM char**)pgm_read_word(&(font->lookup));

    uint8_t c;
    if (!text) c = text_in_flash;
    else c = text_in_flash ? pgm_read_byte(text) : *text;
    uint8_t width;
    const PROGMEM char* start;

    while(c != 0)
    {
//        if ( c>=' '){
    	uint8_t index = c - first;
        if (fixedwidth) {
            start = (const char*)lookup+index*fixedwidth;
            width = fixedwidth;
        }
        else {

        	const char** PROGMEM offset_ptr = lookup + index;								//pointer to font lookup table, with index for current char

        	start = (const PROGMEM char*)pgm_read_word(offset_ptr);
        	const PROGMEM char* end = (const PROGMEM char*)pgm_read_word(offset_ptr+1);
        	width = end - start;

        }

        column += width+interspacing;

        if(pixelshift){
        	uint8_t mask=0xff>>(8-pixelshift);
        	updateBuffer_P(start, width, pixelshift, mask);
        	uint8_t count=interspacing;
        	while (count--) updateData(0,mask);
            pageUpdateEnd();
            command(GLCD_CMD_SET_PAGE(page+1));
            send(GLCD_CMD_READ_MODIFY_WRITE);
            pixelshift=8-pixelshift+128;
            mask=~mask;
            updateBuffer_P(start, width, pixelshift, mask);
        	while (interspacing--) updateData(0,mask);
            command(GLCD_CMD_SET_PAGE(page));
        }
        else{
            updateBuffer_P(start, width);
            sendRepeated(0, interspacing);
        }
//        }
        if (!text) c = 0;
        else {
        	++text;
        	c = text_in_flash ? pgm_read_byte(text) : *text;
        }
    }

    pageUpdateEnd();

    return column;
}

/**
 * sets the font to be used with the glcd write function.
 *
 * @param font      Font to use for text.
 *
 */
void SED1531::setFont(glcd_font_ptr font)
{
     _font = font;
}

/**
 * set cursor position for write
 *
 * @param column	horizontal position (pixel) on display to write next char to
 * @param enable    vertical position (pixel/8 = page number) on display to write next char to
 */
void SED1531::setCursor(uint8_t column, uint8_t row){
    GLCD_IO_PIN_A0_0();
    send(GLCD_CMD_SET_COLUMN_HIGH(column >> 4));
    send(GLCD_CMD_SET_COLUMN_LOW(column & 0x0F));
    send(GLCD_CMD_SET_PAGE(row));
	_currcolumn = column;
	_currpage = row;
	}

/**
 * Draw 1 character stored in a variable at the current cursor position.
 * sets cursor to position for next character
 *
 * @param data   variable with the char to print
 */
size_t SED1531::write(uint8_t text) {
    if(text =='\n'){				//newline
    	_currcolumn = 0;
		_currpage+=8;
		if(_currpage==64) _currpage=0;
    }
    else if(text >=' '){
    	_currcolumn = drawText_int(_currpage, _currcolumn, _font, 0, text);
    	return 1;
    }
	return 0;
}

/**
 * Draw 1 character stored in a variable.
 *
 * @param page      Line at which text should be written.
 * @param column    Pixel column at which text should be written.
 * @param font      Font to use for text.
 * @param text      a char variable which holds the character.
 *
 * @return Pixel column where text ends.
 */
uint8_t SED1531::drawText(uint8_t page, uint8_t column, glcd_font_ptr font, char text)
{
    return drawText_int(page, column, font, 0, text);
}

/**
 * Draw text stored in RAM.
 *
 * @param page      Line at which text should be written.
 * @param column    Pixel column at which text should be written.
 * @param font      Font to use for text.
 * @param text      RAM memory pointer to text.
 *
 * @return Pixel column where text ends.
 */
uint8_t SED1531::drawText(uint8_t page, uint8_t column, glcd_font_ptr font, const char* text)
{
    return drawText_int(page, column, font, text, 0);
}

/**
 * Draw text stored in program memory.
 *
 * This function is added for Harvard architecture processors,
 * such as the Atmel 8-bit AVR series, to avoid having to use
 * precious RAM memory for fixed texts.
 *
 * @param page      Line at which text should be written.
 * @param column    Pixel column at which text should be written.
 * @param font      Font to use for text.
 * @param text      Program memory pointer to text.
 *
 * @return Pixel column where text ends.
 */
uint8_t SED1531::drawText_P(uint8_t page, uint8_t column, glcd_font_ptr font, const PROGMEM char* text)
{
    return drawText_int(page, column, font, (const char*)text, 1);
}

/**
 * Set display in standby mode.
 * Stops the operation of the duty LCD display system and turns
 * on only the static drive system to reduce current consumption
 * to the minimum level required for static drive.
 */
void SED1531::standby(){
    command(GLCD_CMD_STATIC_INDICATOR_ON);		//Standby Mode
    send(GLCD_CMD_DISPLAY_OFF);				//Start of compound command
    send(GLCD_CMD_ENTIRE_DISPLAY_ON);			//End of compound command, enter standby or sleep mode
}

/**
 * Set display in sleep mode.
 * This mode stops every operation of the LCD display system,
 * and can reduce current consumption nearly to a static current
 * value if no access is made from the microprocessor.
 */
void SED1531::sleep(){
	command(GLCD_CMD_STATIC_INDICATOR_OFF);	//Sleep Mode
    send(GLCD_CMD_DISPLAY_OFF);				//Start of compound command
    send(GLCD_CMD_ENTIRE_DISPLAY_ON);			//End of compound command, enter standby or sleep mode
}

/**
 * Wakeup display from standby or sleep mode.
 */
void SED1531::wakeup(){
	command(GLCD_CMD_ENTIRE_DISPLAY_NORMAL);	//release sleep or standby
    send(GLCD_CMD_STATIC_INDICATOR_ON);		//release from sleep
    send(GLCD_CMD_DISPLAY_ON);				//turn display on again
}

/* Low level functions */

static inline void enable(){
	GLCD_IO_PIN_E_1();
	GLCD_IO_DELAY_WRITE();
	GLCD_IO_PIN_E_0();
	}

void SED1531::send(uint8_t data)
{
    GLCD_IO_DATA_OUTPUT(data);
	enable();
}

void SED1531::sendRepeated(uint8_t data, uint8_t count)
{
    send(data);
    while (--count)
    {
    	enable();
    }
}

void SED1531::updateBuffer_P(const PROGMEM char* buffer, uint8_t count, uint8_t pixelshift, uint8_t mask)
{
    uint8_t i;
    uint8_t data;
    const PROGMEM char* p = buffer;

    for(i = 0; i < count; ++i)
    {
    	data = pgm_read_byte(p++);
    	if (pixelshift) {
    		if (pixelshift&128) updateData(data>>=(pixelshift&127),mask);
    		else updateData(data<<=pixelshift,mask);
    	}
    	else send(data);
    }
}

uint8_t SED1531::updateData(uint8_t pixels, uint8_t mask)
{
    uint8_t data;
	GLCD_IO_DATA_DIR_INPUT();
	GLCD_IO_PIN_RW_1();
	GLCD_IO_PIN_A0_1();
    // Dummy read; first read is bogus according to SED1530 datasheet.
    enable();
    GLCD_IO_PIN_E_1();
    GLCD_IO_DELAY_READ();
    data = GLCD_IO_DATA_INPUT();
 	GLCD_IO_PIN_E_0();
    GLCD_IO_DATA_DIR_OUTPUT();
    GLCD_IO_PIN_RW_0();
    if (mask) send((data & mask) ^ pixels);
    return (data & pixels);			//collision
}

void SED1531::pageUpdateStart(uint8_t page, uint8_t column)
{
    GLCD_IO_PIN_A0_0();
    send(GLCD_CMD_SET_PAGE(page));
    send(GLCD_CMD_SET_COLUMN_HIGH(column >> 4));
    send(GLCD_CMD_SET_COLUMN_LOW(column & 0x0F));
    send(GLCD_CMD_READ_MODIFY_WRITE);
}

void SED1531::pageUpdateEnd()
{
    GLCD_IO_PIN_A0_0();
    send(GLCD_CMD_END);
}
