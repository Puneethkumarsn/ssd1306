/*
    MIT License

    Copyright (c) 2018-2019, Alexey Dynda

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "oled_ssd1331.h"
#include "lcd_common.h"
#include "ssd1331_commands.h"
#include "intf/ssd1306_interface.h"
#include "intf/spi/ssd1306_spi.h"
#include "ssd1306_hal/io.h"
#ifdef SDL_EMULATION
#include "sdl_core.h"
#endif
#include "nano_gfx_types.h"

static const PROGMEM uint8_t s_oled96x64_initData[] =
{
#ifdef SDL_EMULATION
    SDL_LCD_SSD1331_X8,
    0x00,
#endif
    SSD1331_DISPLAYOFF,             // display off
    SSD1331_SEGREMAP, 0x00 | 0x20 | 0x10 | 0x02 | 0x01, /* 8-bit rgb color mode */
    SSD1331_SETSTARTLINE, 0x00,    // First line to start scanning from
    SSD1331_SETDISPLAYOFFSET, 0x00, // Set display offset
    SSD1331_NORMALDISPLAY,
    SSD1331_SETMULTIPLEX, 63,       // Reset to default MUX. See datasheet
    SSD1331_SETMASTER, 0x8E,        // Set master mode
    SSD1331_POWERMODE, 0x0B,        // Disable power-safe mode
    SSD1331_SETPRECHARGE, 0x31,     // Phase 1 and Phase 2 periods
    SSD1331_CLOCKDIV, 0xF0,         // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
    SSD1331_PRECHARGEA, 0x64,
    SSD1331_PRECHARGEB, 0x78,
    SSD1331_PRECHARGELEVEL, 0x3A,
    SSD1331_VCOMH, 0x3E,
    SSD1331_MASTERCURRENT, 0x09,
    SSD1331_CONTRASTA, 0x91,        // RED
    SSD1331_CONTRASTB, 0x50,        // GREEN
    SSD1331_CONTRASTC, 0x7D,        // BLUE
    SSD1331_DISPLAYON,
};

static const PROGMEM uint8_t s_oled96x64_initData16[] =
{
#ifdef SDL_EMULATION
    SDL_LCD_SSD1331_X16,
    0x00,
#endif
    SSD1331_DISPLAYOFF,             // display off
    SSD1331_SEGREMAP, 0x40 | 0x20 | 0x10 | 0x02 | 0x01, /* 16-bit rgb color mode */
    SSD1331_SETSTARTLINE, 0x00,    // First line to start scanning from
    SSD1331_SETDISPLAYOFFSET, 0x00, // Set display offset
    SSD1331_NORMALDISPLAY,
    SSD1331_SETMULTIPLEX, 63,       // Reset to default MUX. See datasheet
    SSD1331_SETMASTER, 0x8E,        // Set master mode
    SSD1331_POWERMODE, 0x0B,        // Disable power-safe mode
    SSD1331_SETPRECHARGE, 0x31,     // Phase 1 and Phase 2 periods
    SSD1331_CLOCKDIV, 0xF0,         // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
    SSD1331_PRECHARGEA, 0x64,
    SSD1331_PRECHARGEB, 0x78,
    SSD1331_PRECHARGELEVEL, 0x3A,
    SSD1331_VCOMH, 0x3E,
    SSD1331_MASTERCURRENT, 0x09,
    SSD1331_CONTRASTA, 0x91,        // RED
    SSD1331_CONTRASTB, 0x50,        // GREEN
    SSD1331_CONTRASTC, 0x7D,        // BLUE
    SSD1331_DISPLAYON,
};

#if 0

//////////////////////// SSD1331 NATIVE MODE ///////////////////////////////////

CONTROLLER_NATIVE_SPI_BLOCK_8BIT_CMDS(
     (s_rotation & 1) ? SSD1331_ROWADDR: SSD1331_COLUMNADDR,
     (s_rotation & 1) ? SSD1331_COLUMNADDR: SSD1331_ROWADDR );

//////////////////////////// GENERIC FUNCTIONS ////////////////////////////

void    ssd1331_setMode(lcd_mode_t mode)
{
    if (mode == LCD_MODE_NORMAL)
    {
        s_rotation &= ~0x04;
        ssd1306_lcd.set_block = set_block_native;
        ssd1306_lcd.next_page = next_page_native;
    }
    else if (mode == LCD_MODE_SSD1306_COMPAT )
    {
        s_rotation |= 0x04;
//        ssd1306_lcd.set_block = set_block_compat;
//        ssd1306_lcd.next_page = next_page_compat;
    }
//    ssd1331_setRotation( s_rotation );
    return;
}

void ssd1331_setRotation(uint8_t rotation)
{
    uint8_t ram_mode;
    if ((rotation^s_rotation) & 0x01)
    {
        uint16_t t = ssd1306_lcd.width;
        ssd1306_lcd.width = ssd1306_lcd.height;
        ssd1306_lcd.height = t;
    }
    s_rotation = (rotation & 0x03) | (s_rotation & 0x04);
    ssd1306_intf.start();
    ssd1306_spiDataMode(0);
    ssd1306_intf.send( SSD1331_SEGREMAP );
    switch (s_rotation)
    {
    // NORMAL FULL COLOR MODE
    case 0:
        ram_mode = 0b00110010;
        break;
    case 1: // 90 degree CW
        ram_mode = 0b00110001;
        break;
    case 2: // 180 degree CW
        ram_mode = 0b00100000;
        break;
    case 3: // 270 degree CW
        ram_mode = 0b00100011;
        break;
    // SSD1306_COMPATIBLE mode
    case 4:
        ram_mode = 0b00110011;
        break;
    case 5: // 90 degree CW
        ram_mode = 0b00110000;
        break;
    case 6: // 180 degree CW
        ram_mode = 0b00100001;
        break;
    case 7: // 270 degree CW
        ram_mode = 0b00100010;
        break;
    default: // 270 degree CW
        ram_mode = 0b00100000;
        break;
    }
    ssd1306_intf.send( ram_mode );
    ssd1306_intf.stop();
}

// 16-bit color in 8-bit display mode
static void    ssd1331_sendPixel16_8(uint16_t data)
{
    uint8_t color = RGB16_TO_RGB8(data);
    ssd1306_intf.send( color );
}

// 8-bit color in 16-bit display mode
static void    ssd1331_sendPixel8_16(uint8_t data)
{
    uint16_t color = RGB8_TO_RGB16(data);
    ssd1306_intf.send( color >> 8 );
    ssd1306_intf.send( color & 0xFF );
}

// 16-bit color in 16-bit display mode
static void    ssd1331_sendPixel16(uint16_t color)
{
    ssd1306_intf.send( color >> 8 );
    ssd1306_intf.send( color & 0xFF );
}

void    ssd1331_96x64_init()
{
    ssd1306_lcd.type = LCD_TYPE_SSD1331;
    ssd1306_lcd.height = 64;
    ssd1306_lcd.width = 96;
//    ssd1306_lcd.set_block = set_block_compat;
//    ssd1306_lcd.next_page = next_page_compat;
//    ssd1306_lcd.send_pixels1  = send_pixels_compat;
//    ssd1306_lcd.send_pixels_buffer1 = send_pixels_buffer_compat;

    ssd1306_lcd.send_pixels8 = ssd1306_intf.send;
    ssd1306_lcd.send_pixels16 = ssd1331_sendPixel16_8;
    ssd1306_lcd.set_mode = ssd1331_setMode;
    for( uint8_t i=0; i<sizeof(s_oled96x64_initData); i++)
    {
        ssd1306_sendCommand(pgm_read_byte(&s_oled96x64_initData[i]));
    }
}

void    ssd1331_96x64_init16()
{
    ssd1306_lcd.type = LCD_TYPE_SSD1331;
    ssd1306_lcd.height = 64;
    ssd1306_lcd.width = 96; // ssd1306_lcd.set_block = set_block_compat; // ssd1306_lcd.next_page = next_page_compat;
//    ssd1306_lcd.send_pixels1  = send_pixels_compat16;
//    ssd1306_lcd.send_pixels_buffer1 = send_pixels_buffer_compat16;

    ssd1306_lcd.send_pixels8 = ssd1331_sendPixel8_16;
    ssd1306_lcd.send_pixels16 = ssd1331_sendPixel16;
    ssd1306_lcd.set_mode = ssd1331_setMode;
    for( uint8_t i=0; i<sizeof(s_oled96x64_initData16); i++)
    {
        ssd1306_sendCommand(pgm_read_byte(&s_oled96x64_initData16[i]));
    }
}

void   ssd1331_96x64_spi_init(int8_t rstPin, int8_t cesPin, int8_t dcPin)
{
    if (rstPin >=0)
    {
        ssd1306_resetController( rstPin, 10 );
    }
    ssd1306_spiInit(cesPin, dcPin);
    ssd1331_96x64_init();
}

void   ssd1331_96x64_spi_init16(int8_t rstPin, int8_t cesPin, int8_t dcPin)
{
    if (rstPin >=0)
    {
        ssd1306_resetController( rstPin, 10 );
    }
    ssd1306_spiInit(cesPin, dcPin);
    ssd1331_96x64_init16();
}

void ssd1331_drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t color)
{
    ssd1306_intf.start();
    ssd1306_spiDataMode(0);
    ssd1306_intf.send(SSD1331_DRAWLINE);
    ssd1306_intf.send(x1);
    ssd1306_intf.send(y1);
    ssd1306_intf.send(x2);
    ssd1306_intf.send(y2);
    ssd1306_intf.send( (color & 0x03) << 4 );
    ssd1306_intf.send( (color & 0x1C) << 2 );
    ssd1306_intf.send( (color & 0xE0) >> 2 );
    ssd1306_intf.stop();
}

void ssd1331_copyBlock(uint8_t left, uint8_t top, uint8_t right, uint8_t bottom, uint8_t newLeft, uint8_t newTop)
{
    ssd1306_intf.start();
    ssd1306_spiDataMode(0);
    ssd1306_intf.send(0x23);
    ssd1306_intf.send(left);
    ssd1306_intf.send(top);
    ssd1306_intf.send(right);
    ssd1306_intf.send(bottom);
    ssd1306_intf.send(newLeft);
    ssd1306_intf.send(newTop);
    ssd1306_intf.stop();
}

#endif

extern uint8_t s_ssd1306_invertByte;

DisplaySSD1331::DisplaySSD1331(IWireInterface &intf, int8_t rstPin, int8_t dcPin)
    : NanoDisplayOps<8>(intf)
    , m_rstPin( rstPin)
    , m_dcPin( dcPin )
{
}

void DisplaySSD1331::begin()
{
    if (m_rstPin >=0)
    {
        ssd1306_resetController( m_rstPin, 10 );
    }
    m_w = 96;
    m_h = 64;
    for( uint8_t i=0; i < sizeof(s_oled96x64_initData); i++)
    {
        commandStart();
        m_intf.send(pgm_read_byte(&s_oled96x64_initData[i]));
        m_intf.stop();
    }
    setRotation( m_rotation );
}

void DisplaySSD1331::setBlock(lcduint_t x, lcduint_t y, lcduint_t w)
{
    uint8_t rx = w ? (x + w - 1) : (m_w - 1);
    m_intf.start();
    spiDataMode(0);
    m_intf.send((m_rotation & 1) ? SSD1331_ROWADDR: SSD1331_COLUMNADDR);
    m_intf.send(x);
    m_intf.send(rx < m_w ? rx : (m_w - 1));
    m_intf.send((m_rotation & 1) ? SSD1331_COLUMNADDR: SSD1331_ROWADDR);
    m_intf.send(y);
    m_intf.send(m_h - 1);
    spiDataMode(1);
}

void DisplaySSD1331::nextPage()
{
}

void DisplaySSD1331::spiDataMode(uint8_t mode)
{
    // TODO:
    if (m_dcPin)
    {
        digitalWrite(m_dcPin, mode ? HIGH : LOW);
    }
}

void DisplaySSD1331::commandStart(void)
{
    m_intf.start();
    spiDataMode(0);
}

void DisplaySSD1331::setRotation(uint8_t rotation)
{
    uint8_t ram_mode;
    if ((rotation^m_rotation) & 0x01)
    {
        uint16_t t = m_w;
        m_w = m_h;
        m_h = t;
    }
    m_rotation = (rotation & 0x03) | (m_rotation & 0x04);
    m_intf.start();
    spiDataMode(0);
    m_intf.send( SSD1331_SEGREMAP );
    switch (m_rotation)
    {
    // NORMAL FULL COLOR MODE
    case 0:
        ram_mode = 0b00110010;
        break;
    case 1: // 90 degree CW
        ram_mode = 0b00110001;
        break;
    case 2: // 180 degree CW
        ram_mode = 0b00100000;
        break;
    case 3: // 270 degree CW
    default:
        ram_mode = 0b00100011;
        break;
    }
    m_intf.send( ram_mode );
    m_intf.stop();
}

void DisplaySSD1331::negativeMode()
{
    s_ssd1306_invertByte = 0xFF;
}

void DisplaySSD1331::positiveMode()
{
    s_ssd1306_invertByte = 0x00;
}

void DisplaySSD1331::drawLine(lcdint_t x1, lcdint_t y1, lcdint_t x2, lcdint_t y2, uint16_t color)
{
    m_intf.start();
    spiDataMode(0);
    m_intf.send(SSD1331_DRAWLINE);
    m_intf.send(x1);
    m_intf.send(y1);
    m_intf.send(x2);
    m_intf.send(y2);
    m_intf.send( (color & 0x03) << 4 );
    m_intf.send( (color & 0x1C) << 2 );
    m_intf.send( (color & 0xE0) >> 2 );
    m_intf.stop();
}

void DisplaySSD1331::copyBlock(uint8_t left, uint8_t top,
     uint8_t right, uint8_t bottom, uint8_t newLeft, uint8_t newTop)
{
    m_intf.start();
    spiDataMode(0);
    m_intf.send(0x23);
    m_intf.send(left);
    m_intf.send(top);
    m_intf.send(right);
    m_intf.send(bottom);
    m_intf.send(newLeft);
    m_intf.send(newTop);
    m_intf.stop();
}



void DisplaySSD1331_96x64_SPI::begin()
{
//    m_spi.begin();
//    m_lcd.type = LCD_TYPE_SSD1331;
    DisplaySSD1331::begin();
}

void DisplaySSD1331_96x64_SPI::end()
{
//    m_spi.end();
}

//////////////////////////////////////////////////////////////////////////////
//                  16-bit
//////////////////////////////////////////////////////////////////////////////

DisplaySSD1331x16::DisplaySSD1331x16(IWireInterface &intf, int8_t rstPin, int8_t dcPin)
    : NanoDisplayOps<16>(intf)
    , m_rstPin( rstPin)
    , m_dcPin( dcPin )
{
}

void DisplaySSD1331x16::begin()
{
    if (m_rstPin >=0)
    {
        ssd1306_resetController( m_rstPin, 10 );
    }
    m_w = 96;
    m_h = 64;
    for( uint8_t i=0; i < sizeof(s_oled96x64_initData16); i++)
    {
        commandStart();
        m_intf.send(pgm_read_byte(&s_oled96x64_initData16[i]));
        m_intf.stop();
    }
    setRotation( m_rotation );
}

void DisplaySSD1331x16::setBlock(lcduint_t x, lcduint_t y, lcduint_t w)
{
    uint8_t rx = w ? (x + w - 1) : (m_w - 1);
    m_intf.start();
    spiDataMode(0);
    m_intf.send((m_rotation & 1) ? SSD1331_ROWADDR: SSD1331_COLUMNADDR);
    m_intf.send(x);
    m_intf.send(rx < m_w ? rx : (m_w - 1));
    m_intf.send((m_rotation & 1) ? SSD1331_COLUMNADDR: SSD1331_ROWADDR);
    m_intf.send(y);
    m_intf.send(m_h - 1);
    spiDataMode(1);
}

void DisplaySSD1331x16::nextPage()
{
}

void DisplaySSD1331x16::spiDataMode(uint8_t mode)
{
    // TODO:
    if (m_dcPin)
    {
        digitalWrite(m_dcPin, mode ? HIGH : LOW);
    }
}

void DisplaySSD1331x16::commandStart(void)
{
    m_intf.start();
    spiDataMode(0);
}

void DisplaySSD1331x16::setRotation(uint8_t rotation)
{
    uint8_t ram_mode;
    if ((rotation^m_rotation) & 0x01)
    {
        uint16_t t = m_w;
        m_w = m_h;
        m_h = t;
    }
    m_rotation = (rotation & 0x03) | (m_rotation & 0x04);
    m_intf.start();
    spiDataMode(0);
    m_intf.send( SSD1331_SEGREMAP );
    switch (m_rotation)
    {
    // NORMAL FULL COLOR MODE
    case 0:
        ram_mode = 0b00110010;
        break;
    case 1: // 90 degree CW
        ram_mode = 0b00110001;
        break;
    case 2: // 180 degree CW
        ram_mode = 0b00100000;
        break;
    case 3: // 270 degree CW
    default:
        ram_mode = 0b00100011;
        break;
    }
    m_intf.send( ram_mode | 0x40 );
    m_intf.stop();
}

void DisplaySSD1331x16::negativeMode()
{
    s_ssd1306_invertByte = 0xFF;
}

void DisplaySSD1331x16::positiveMode()
{
    s_ssd1306_invertByte = 0x00;
}

void DisplaySSD1331x16::drawLine(lcdint_t x1, lcdint_t y1, lcdint_t x2, lcdint_t y2, uint16_t color)
{
    m_intf.start();
    spiDataMode(0);
    m_intf.send(SSD1331_DRAWLINE);
    m_intf.send(x1);
    m_intf.send(y1);
    m_intf.send(x2);
    m_intf.send(y2);
    m_intf.send( (color & 0x001F) << 1 );
    m_intf.send( (color & 0x07E0) >> 5 );
    m_intf.send( (color & 0xF800) >> 10 );
    m_intf.stop();
}

void DisplaySSD1331x16::copyBlock(uint8_t left, uint8_t top,
     uint8_t right, uint8_t bottom, uint8_t newLeft, uint8_t newTop)
{
    m_intf.start();
    spiDataMode(0);
    m_intf.send(0x23);
    m_intf.send(left);
    m_intf.send(top);
    m_intf.send(right);
    m_intf.send(bottom);
    m_intf.send(newLeft);
    m_intf.send(newTop);
    m_intf.stop();
}

void DisplaySSD1331_96x64x16_SPI::begin()
{
//    m_spi.begin();
//    m_lcd.type = LCD_TYPE_SSD1331;
    DisplaySSD1331x16::begin();
}

void DisplaySSD1331_96x64x16_SPI::end()
{
//    m_spi.end();
}
