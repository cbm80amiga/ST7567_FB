// Fast ST7567 128x64 LCD graphics library (with frambuffer)
// (C) 2020 by Pawel A. Hernik

/*
 128x64 ST7567 connections in SPI mode (only 5-6 wires between LCD and MCU):

 #01 LED  -> D6, GND or any pin via resistor
 #02 RST  -> D9 or any pin
 #03 CS   -> D10 or any pin
 #04 DC   -> D8 or any pin
 #05 SCK  -> D13/SCK
 #06 SDI  -> D11/MOSI
 #07 3V3  -> VCC (3.3V)
 #08 GND  -> GND
*/

#include "ST7567_FB.h"

#define fontbyte(x) pgm_read_byte(&cfont.font[x])

#include <SPI.h>

#define CS_IDLE     digitalWrite(csPin, HIGH)
#define CS_ACTIVE   digitalWrite(csPin, LOW)
#define DC_DATA     digitalWrite(dcPin, HIGH)
#define DC_COMMAND  digitalWrite(dcPin, LOW)

//------------------------------
// ST7567 Commands
#define ST7567_POWER_ON         0x2F // internal power supply on
#define ST7567_POWER_CTL        0x28 // internal power supply off
#define ST7567_CONTRAST         0x80 // 0x80 + (0..31)
#define ST7567_SEG_NORMAL       0xA0 // SEG remap normal
#define ST7567_SEG_REMAP        0xA1 // SEG remap reverse (flip horizontal)
#define ST7567_DISPLAY_NORMAL   0xA4 // display ram content
#define ST7567_DISPLAY_TEST     0xA5 // all pixels on
#define ST7567_INVERT_OFF       0xA6 // not inverted
#define ST7567_INVERT_ON        0xA7 // inverted
#define ST7567_DISPLAY_ON       0XAF // display on
#define ST7567_DISPLAY_OFF      0XAE // display off
#define ST7567_STATIC_OFF       0xAC
#define ST7567_STATIC_ON        0xAD
#define ST7567_SCAN_START_LINE  0x40 // scrolling 0x40 + (0..63)
#define ST7567_COM_NORMAL       0xC0 // COM remap normal
#define ST7567_COM_REMAP        0xC8 // COM remap reverse (flip vertical)
#define ST7567_SW_RESET         0xE2 // connect RST pin to GND and rely on software reset
#define ST7567_NOP              0xE3 // no operation
#define ST7567_TEST             0xF0

#define ST7567_COL_ADDR_H       0x10 // x pos (0..95) 4 MSB
#define ST7567_COL_ADDR_L       0x00 // x pos (0..95) 4 LSB
#define ST7567_PAGE_ADDR        0xB0 // y pos, 8.5 rows (0..8)
#define ST7567_RMW              0xE0
#define ST7567_RMW_CLEAR        0xEE

#define ST7567_BIAS_9           0xA2 
#define ST7567_BIAS_7           0xA3

#define ST7567_VOLUME_FIRST     0x81
#define ST7567_VOLUME_SECOND    0x00

#define ST7567_RESISTOR_RATIO   0x20
#define ST7567_STATIC_REG       0x0
#define ST7567_BOOSTER_FIRST    0xF8
#define ST7567_BOOSTER_234      0
#define ST7567_BOOSTER_5        1
#define ST7567_BOOSTER_6        3
//------------------------------

const uint8_t initData[] PROGMEM = {
  ST7567_BIAS_7,
  ST7567_SEG_NORMAL,
  ST7567_COM_REMAP,
  ST7567_POWER_CTL | 0x4,
  ST7567_POWER_CTL | 0x6, // turn on voltage regulator (VC=1, VR=1, VF=0)
  ST7567_POWER_CTL | 0x7, // turn on voltage follower (VC=1, VR=1, VF=1)
  ST7567_RESISTOR_RATIO | 0x6, // set lcd operating voltage (regulator resistor, ref voltage resistor)
  ST7567_SCAN_START_LINE+0,
  ST7567_DISPLAY_ON,
  ST7567_DISPLAY_NORMAL
};
// ----------------------------------------------------------------
inline void ST7567_FB::sendSPI(uint8_t v)
{
  #ifdef USE_HW_SPI
  SPI.transfer(v);
  #else
  shiftOut(sdiPin, clkPin, MSBFIRST, v);
  #endif
}
// ----------------------------------------------------------------
inline void ST7567_FB::sendCmd(uint8_t cmd)
{
  DC_COMMAND;
  sendSPI(cmd);
}
// ----------------------------------------------------------------
inline void ST7567_FB::sendData(uint8_t data)
{
  DC_DATA;
  sendSPI(data);
}
// ----------------------------------------------------------------
ST7567_FB::ST7567_FB(uint8_t dc, uint8_t rst, uint8_t cs)
{
  dcPin   = dc;
  rstPin  = rst;
  csPin   = cs;
  sdiPin  = 11;
  clkPin  = 13;
}
// ----------------------------------------------------------------
ST7567_FB::ST7567_FB(uint8_t dc, uint8_t rst, uint8_t cs, uint8_t sdi, uint8_t clk)
{
  dcPin   = dc;
  rstPin  = rst;
  csPin   = cs;
  sdiPin  = sdi;
  clkPin  = clk;
}
// ----------------------------------------------------------------
byte ST7567_FB::scr[SCR_WD*SCR_HT8];

void ST7567_FB::init(int contrast)
{
  scrWd=SCR_WD;
  scrHt=SCR_HT/8;
  isNumberFun = &isNumber;
  cr = 0;
  cfont.font = NULL;
  dualChar = 0;

  pinMode(csPin, OUTPUT);
  pinMode(dcPin, OUTPUT);
  if(rstPin<255) {
    pinMode(rstPin, OUTPUT);
    digitalWrite(rstPin, HIGH);
    delay(50);
    digitalWrite(rstPin, LOW);
    delay(500);
    digitalWrite(rstPin, HIGH);
    delay(10);
  }

#ifdef USE_HW_SPI
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
#ifdef __AVR__
  SPI.setClockDivider(SPI_CLOCK_DIV2);
#endif
#else
  pinMode(sdiPin, OUTPUT);
  pinMode(clkPin, OUTPUT);
#endif
  initCmds();
  setContrast(contrast);
  //setRotation(0);
}
// ----------------------------------------------------------------
void ST7567_FB::initCmds()
{
  CS_ACTIVE;
  for(int i=0; i<sizeof(initData); i++) sendCmd(pgm_read_byte(initData+i));
  /*
  sendCmd(ST7567_BIAS_7);
  sendCmd(ST7567_SEG_NORMAL);
  sendCmd(ST7567_COM_REMAP);

  sendCmd(ST7567_POWER_CTL | 0x4); delay(50);

  // turn on voltage regulator (VC=1, VR=1, VF=0)
  sendCmd(ST7567_POWER_CTL | 0x6); delay(50);

  // turn on voltage follower (VC=1, VR=1, VF=1)
  sendCmd(ST7567_POWER_CTL | 0x7); delay(10);

  // set lcd operating voltage (regulator resistor, ref voltage resistor)
  sendCmd(ST7567_RESISTOR_RATIO | 0x6);

  // Initial display line
  sendCmd(ST7567_SCAN_START_LINE+0); 
  sendCmd(ST7567_DISPLAY_ON);
  sendCmd(ST7567_DISPLAY_NORMAL);
  */
  CS_IDLE;
}
// ----------------------------------------------------------------
void ST7567_FB::gotoXY(byte x, byte y)
{
  CS_ACTIVE;
  sendCmd(ST7567_PAGE_ADDR | y);
  sendCmd(ST7567_COL_ADDR_H | (x >> 4));
  sendCmd(ST7567_COL_ADDR_L | (x & 0xf));
  sendCmd(ST7567_RMW);
  //CS_IDLE;
}
// ----------------------------------------------------------------
void ST7567_FB::sleep(bool mode)
{
  if(mode) {
    cls();
    display();
    CS_ACTIVE;
    sendCmd(ST7567_DISPLAY_OFF); // power down
    sendCmd(ST7567_DISPLAY_TEST); // all pixels on (saves power)
    CS_IDLE;
  } else initCmds();
}
// ----------------------------------------------------------------
// 0..31
void ST7567_FB::setContrast(byte val)
{
  CS_ACTIVE;
  sendCmd(ST7567_VOLUME_FIRST);
  sendCmd(ST7567_VOLUME_SECOND | (val & 0x3f));
  CS_IDLE;
}
// ----------------------------------------------------------------
// 0..63
void ST7567_FB::setScroll(byte val)
{
  CS_ACTIVE;
  sendCmd(ST7567_SCAN_START_LINE|(val&0x3f));
  CS_IDLE;
}
// ----------------------------------------------------------------
// 0..3, only 0 and 2 supported on ST7567
void ST7567_FB::setRotation(int mode)
{
  rotation = mode;
  CS_ACTIVE;
  switch(mode) {
    case 0:
      sendCmd(ST7567_SEG_NORMAL);
      sendCmd(ST7567_COM_REMAP);
      break;
    case 2:
      sendCmd(ST7567_SEG_REMAP);
      sendCmd(ST7567_COM_NORMAL);
      break;
  }
  CS_IDLE;
}
// ----------------------------------------------------------------
void ST7567_FB::displayInvert(bool mode)
{
  CS_ACTIVE;
  sendCmd(mode ? ST7567_INVERT_ON : ST7567_INVERT_OFF);
  CS_IDLE;
}
// ----------------------------------------------------------------
void ST7567_FB::displayOn(bool mode)
{
  CS_ACTIVE;
  sendCmd(mode ? ST7567_DISPLAY_ON : ST7567_DISPLAY_OFF);
  CS_IDLE;
}
// ----------------------------------------------------------------
// val=ST7567_POWER_ON, ST7567_POWER_OFF, ST7567_DISPLAY_NORMAL, ST7567_DISPLAY_TEST
//     ST7567_INVERT_OFF, ST7567_INVERT_ON, ST7567_DISPLAY_ON, ST7567_DISPLAY_OFF
void ST7567_FB::displayMode(byte val)
{
  CS_ACTIVE;
  sendCmd(val);
  CS_IDLE;
}
// ----------------------------------------------------------------
void ST7567_FB::display()
{
  for(int y8=0; y8<SCR_HT8; y8++) {
    gotoXY(rotation?4:0,y8);
    //CS_ACTIVE;
    DC_DATA;
    for(int x=0; x<SCR_WD; x++) sendSPI(scr[x+y8*SCR_WD]);
  }
  CS_IDLE;
}
// ----------------------------------------------------------------
// copy only part of framebuffer
void ST7567_FB::copy(uint8_t x, uint8_t y8, uint8_t wd, uint8_t ht8)
{
  uint8_t wdb = wd;
  for(int i=0; i<ht8; i++) {
    gotoXY(x+(rotation?4:0),y8+i);
    //CS_ACTIVE;
    DC_DATA;
    for(int x=0; x<wd; x++) sendSPI(*(scr+wdb*i+x));
  }
  CS_IDLE;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void ST7567_FB::cls()
{
  memset(scr,0,SCR_WD*SCR_HT8);
}
// ----------------------------------------------------------------
void ST7567_FB::drawPixel(uint8_t x, uint8_t y, uint8_t col) 
{
  if(x>=SCR_WD || y>=SCR_HT) return;
  switch(col) {
    case 1: scr[(y/8)*scrWd+x] |=   (1 << (y&7)); break;
    case 0: scr[(y/8)*scrWd+x] &=  ~(1 << (y&7)); break;
    case 2: scr[(y/8)*scrWd+x] ^=   (1 << (y&7)); break;
  }
}
// ----------------------------------------------------------------
void ST7567_FB::drawLine(int8_t x0, int8_t y0, int8_t x1, int8_t y1, uint8_t col)
{
  int dx = abs(x1-x0);
  int dy = abs(y1-y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;
 
  while(1) {
    //if(x0>=0 && y0>=0) 
      drawPixel(x0, y0, col);
    if(x0==x1 && y0==y1) return;
    int err2 = err+err;
    if(err2>-dy) { err-=dy; x0+=sx; }
    if(err2< dx) { err+=dx; y0+=sy; }
  }
}
// ----------------------------------------------------------------
void ST7567_FB::drawLineH(uint8_t x0, uint8_t x1, uint8_t y, uint8_t col)
{
  if(x1>x0) for(uint8_t x=x0; x<=x1; x++) drawPixel(x,y,col);
  else      for(uint8_t x=x1; x<=x0; x++) drawPixel(x,y,col);
}
// ----------------------------------------------------------------
void ST7567_FB::drawLineV(uint8_t x, uint8_t y0, uint8_t y1, uint8_t col)
{
  if(y1>y0) for(uint8_t y=y0; y<=y1; y++) drawPixel(x,y,col);
  else      for(uint8_t y=y1; y<=y0; y++) drawPixel(x,y,col);
}
// ----------------------------------------------------------------
// about 4x faster than regular drawLineH
void ST7567_FB::drawLineHfast(uint8_t x0, uint8_t x1, uint8_t y, uint8_t col)
{
  uint8_t mask;
  if(x1<x0) { mask=x0; x0=x1; x1=mask; } // swap
  mask = 1 << (y&7);
  switch(col) {
    case 1: for(int x=x0; x<=x1; x++) scr[(y/8)*scrWd+x] |= mask;   break;
    case 0: for(int x=x0; x<=x1; x++) scr[(y/8)*scrWd+x] &= ~mask;  break;
    case 2: for(int x=x0; x<=x1; x++) scr[(y/8)*scrWd+x] ^= mask;   break;
  }
}
// ----------------------------------------------------------------
// limited to pattern #8
void ST7567_FB::drawLineHfastD(uint8_t x0, uint8_t x1, uint8_t y, uint8_t col)
{
  uint8_t mask;
  if(x1<x0) { mask=x0; x0=x1; x1=mask; } // swap
  if(((x0&1)==1 && (y&1)==0) || ((x0&1)==0 && (y&1)==1)) x0++;
  mask = 1 << (y&7);
  switch(col) {
    case 1: for(int x=x0; x<=x1; x+=2) scr[(y/8)*scrWd+x] |= mask;   break;
    case 0: for(int x=x0; x<=x1; x+=2) scr[(y/8)*scrWd+x] &= ~mask;  break;
    case 2: for(int x=x0; x<=x1; x+=2) scr[(y/8)*scrWd+x] ^= mask;   break;
  }
}
// ----------------------------------------------------------------
byte ST7567_FB::ystab[8]={0xff,0xfe,0xfc,0xf8,0xf0,0xe0,0xc0,0x80};
byte ST7567_FB::yetab[8]={0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f,0xff};
byte ST7567_FB::pattern[4]={0xaa,0x55,0xaa,0x55};
// about 40x faster than regular drawLineV
void ST7567_FB::drawLineVfast(uint8_t x, uint8_t y0, uint8_t y1, uint8_t col)
{
  if(x<0 || x>SCR_WD) return;
  int y8s,y8e;
  if(y1<y0) { y8s=y1; y1=y0; y0=y8s; } // swap
  if(y0<0) y0=0;
  if(y1>=SCR_WD) y1=SCR_WD-1;
  y8s=y0/8;
  y8e=y1/8;

  switch(col) {
    case 1: 
      if(y8s==y8e) scr[y8s*scrWd+x]|=(ystab[y0&7] & yetab[y1&7]);
      else { scr[y8s*scrWd+x]|=ystab[y0&7]; scr[y8e*scrWd+x]|=yetab[y1&7]; }
      for(int y=y8s+1; y<y8e; y++) scr[y*scrWd+x]=0xff;
      break;
    case 0:
      if(y8s==y8e) scr[y8s*scrWd+x]&=~(ystab[y0&7] & yetab[y1&7]);
      else { scr[y8s*scrWd+x]&=~ystab[y0&7]; scr[y8e*scrWd+x]&=~yetab[y1&7]; }
      for(int y=y8s+1; y<y8e; y++) scr[y*scrWd+x]=0x00;
      break;
    case 2: 
      if(y8s==y8e) scr[y8s*scrWd+x]^=(ystab[y0&7] & yetab[y1&7]);
      else { scr[y8s*scrWd+x]^=ystab[y0&7]; scr[y8e*scrWd+x]^=yetab[y1&7]; }
      for(int y=y8s+1; y<y8e; y++) scr[y*scrWd+x]^=0xff;
      break;
  }
}
// ----------------------------------------------------------------
// dithered version
void ST7567_FB::drawLineVfastD(uint8_t x, uint8_t y0, uint8_t y1, uint8_t col)
{
  int y8s,y8e;
  if(y1<y0) { y8s=y1; y1=y0; y0=y8s; } // swap
  y8s=y0/8;
  y8e=y1/8;

  switch(col) {
    case 1: 
      if(y8s==y8e) scr[y8s*scrWd+x]|=(ystab[y0&7] & yetab[y1&7] & pattern[x&3]);
      else { scr[y8s*scrWd+x]|=(ystab[y0&7] & pattern[x&3]); scr[y8e*scrWd+x]|=(yetab[y1&7] & pattern[x&3]); }
      for(int y=y8s+1; y<y8e; y++) scr[y*scrWd+x]|=pattern[x&3];
      break;
    case 0:
      if(y8s==y8e) scr[y8s*scrWd+x]&=~(ystab[y0&7] & yetab[y1&7] & pattern[x&3]);
      else { scr[y8s*scrWd+x]&=~(ystab[y0&7] & pattern[x&3]); scr[y8e*scrWd+x]&=~(yetab[y1&7] & pattern[x&3]); }
      for(int y=y8s+1; y<y8e; y++) scr[y*scrWd+x]&=~pattern[x&3];
      break;
    case 2: 
      if(y8s==y8e) scr[y8s*scrWd+x]^=(ystab[y0&7] & yetab[y1&7] & pattern[x&3]);
      else { scr[y8s*scrWd+x]^=(ystab[y0&7] & pattern[x&3]); scr[y8e*scrWd+x]^=(yetab[y1&7] & pattern[x&3]); }
      for(int y=y8s+1; y<y8e; y++) scr[y*scrWd+x]^=pattern[x&3];
      break;
  }
}
// ----------------------------------------------------------------
void ST7567_FB::drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t col)
{
  if(x>=SCR_WD || y>=SCR_HT) return;
  byte drawVright=1;
  if(x+w>SCR_WD) { w=SCR_WD-x; drawVright=0; }
  if(y+h>SCR_HT) h=SCR_HT-y; else drawLineHfast(x, x+w-1, y+h-1,col);
  drawLineHfast(x, x+w-1, y,col);
  drawLineVfast(x,    y+1, y+h-2,col);
  if(drawVright) drawLineVfast(x+w-1,y+1, y+h-2,col);
}
// ----------------------------------------------------------------
// dithered version
void ST7567_FB::drawRectD(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t col)
{
  if(x>=SCR_WD || y>=SCR_HT) return;
  byte drawVright=1;
  if(x+w>SCR_WD) { w=SCR_WD-x; drawVright=0; }
  if(y+h>SCR_HT) h=SCR_HT-y; else drawLineHfastD(x, x+w-1, y+h-1,col);
  drawLineHfastD(x, x+w-1, y,col);
  drawLineVfastD(x,    y+1, y+h-2,col);
  if(drawVright) drawLineVfastD(x+w-1,y+1, y+h-2,col);
}
// ----------------------------------------------------------------
void ST7567_FB::fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t col)
{
  if(x>=SCR_WD || y>=SCR_HT || w<=0 || h<=0) return;
  if(x+w>SCR_WD) w=SCR_WD-x;
  if(y+h>SCR_HT) h=SCR_HT-y;
  for(int i=x;i<x+w;i++) drawLineVfast(i,y,y+h-1,col);
}
// ----------------------------------------------------------------
// dithered version
void ST7567_FB::fillRectD(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t col)
{
  if(x>=SCR_WD || y>=SCR_HT || w<=0 || h<=0) return;
  if(x+w>=SCR_WD) w=SCR_WD-x;
  if(y+h>=SCR_HT) h=SCR_HT-y;
  for(int i=x;i<x+w;i++) drawLineVfastD(i,y,y+h-1,col);
}
// ----------------------------------------------------------------
// circle
void ST7567_FB::drawCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint8_t col)
{
  int f = 1 - (int)radius;
  int ddF_x = 1;
  int ddF_y = -2 * (int)radius;
  int x = 0;
  int y = radius;
 
  drawPixel(x0, y0 + radius, col);
  drawPixel(x0, y0 - radius, col);
  drawPixel(x0 + radius, y0, col);
  drawPixel(x0 - radius, y0, col);
 
  while(x < y) {
    if(f >= 0) {
      y--; ddF_y += 2; f += ddF_y;
    }
    x++; ddF_x += 2; f += ddF_x;    
    drawPixel(x0 + x, y0 + y, col);
    drawPixel(x0 - x, y0 + y, col);
    drawPixel(x0 + x, y0 - y, col);
    drawPixel(x0 - x, y0 - y, col);
    drawPixel(x0 + y, y0 + x, col);
    drawPixel(x0 - y, y0 + x, col);
    drawPixel(x0 + y, y0 - x, col);
    drawPixel(x0 - y, y0 - x, col);
  }
}
// ----------------------------------------------------------------
void ST7567_FB::fillCircle(uint8_t x0, uint8_t y0, uint8_t r, uint8_t col)
{
  drawLineVfast(x0, y0-r, y0-r+2*r+1, col);
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    drawLineVfast(x0+x, y0-y, y0-y+2*y+1, col);
    drawLineVfast(x0+y, y0-x, y0-x+2*x+1, col);
    drawLineVfast(x0-x, y0-y, y0-y+2*y+1, col);
    drawLineVfast(x0-y, y0-x, y0-x+2*x+1, col);
  }
}
// ----------------------------------------------------------------
// dithered version
void ST7567_FB::fillCircleD(uint8_t x0, uint8_t y0, uint8_t r, uint8_t col)
{
  drawLineVfastD(x0, y0-r, y0-r+2*r+1, col);
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    drawLineVfastD(x0+x, y0-y, y0-y+2*y+1, col);
    drawLineVfastD(x0+y, y0-x, y0-x+2*x+1, col);
    drawLineVfastD(x0-x, y0-y, y0-y+2*y+1, col);
    drawLineVfastD(x0-y, y0-x, y0-x+2*x+1, col);
  }
}
// ----------------------------------------------------------------
void ST7567_FB::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color);
}
#define swap(a, b) { int16_t t = a; a = b; b = t; }
// ----------------------------------------------------------------
// optimized for ST7567 native frame buffer
void ST7567_FB::fillTriangle( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  int16_t a, b, x, last;
  if (x0 > x1) { swap(y0, y1); swap(x0, x1); }
  if (x1 > x2) { swap(y2, y1); swap(x2, x1); }
  if (x0 > x1) { swap(y0, y1); swap(x0, x1); }

  if(x0 == x2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(y1 < a)      a = y1;
    else if(y1 > b) b = y1;
    if(y2 < a)      a = y2;
    else if(y2 > b) b = y2;
    drawLineVfast(x0, a, b, color);
    return;
  }

  int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
  long sa = 0, sb = 0;

  if(x1 == x2) last = x1;
  else         last = x1-1;

  for(x=x0; x<=last; x++) {
    a   = y0 + sa / dx01;
    b   = y0 + sb / dx02;
    sa += dy01;
    sb += dy02;
    if(a > b) swap(a,b);
    drawLineVfast(x, a, b, color);
  }

  sa = dy12 * (x - x1);
  sb = dy02 * (x - x0);
  for(; x<=x2; x++) {
    a   = y1 + sa / dx12;
    b   = y0 + sb / dx02;
    sa += dy12;
    sb += dy02;
    if(a > b) swap(a,b);
    drawLineVfast(x, a, b, color);
  }
}
// ----------------------------------------------------------------
// optimized for ST7567 native frame buffer
void ST7567_FB::fillTriangleD( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  int16_t a, b, x, last;
  if (x0 > x1) { swap(y0, y1); swap(x0, x1); }
  if (x1 > x2) { swap(y2, y1); swap(x2, x1); }
  if (x0 > x1) { swap(y0, y1); swap(x0, x1); }

  if(x0 == x2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(y1 < a)      a = y1;
    else if(y1 > b) b = y1;
    if(y2 < a)      a = y2;
    else if(y2 > b) b = y2;
    drawLineVfastD(x0, a, b, color);
    return;
  }

  int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
  long sa = 0, sb = 0;

  if(x1 == x2) last = x1;
  else         last = x1-1;

  for(x=x0; x<=last; x++) {
    a   = y0 + sa / dx01;
    b   = y0 + sb / dx02;
    sa += dy01;
    sb += dy02;
    if(a > b) swap(a,b);
    drawLineVfastD(x, a, b, color);
  }

  sa = dy12 * (x - x1);
  sb = dy02 * (x - x0);
  for(; x<=x2; x++) {
    a   = y1 + sa / dx12;
    b   = y0 + sb / dx02;
    sa += dy12;
    sb += dy02;
    if(a > b) swap(a,b);
    drawLineVfastD(x, a, b, color);
  }
}
// ----------------------------------------------------------------
const byte ST7567_FB::ditherTab[4*17] PROGMEM = {
  0x00,0x00,0x00,0x00, // 0

  0x00,0x00,0x00,0x88, // 1
  0x00,0x22,0x00,0x88, // 2
  0x00,0xaa,0x00,0x88, // 3
  0x00,0xaa,0x00,0xaa, // 4
  0x44,0xaa,0x00,0xaa, // 5
  0x44,0xaa,0x11,0xaa, // 6
  0x44,0xaa,0x55,0xaa, // 7
  
  0x55,0xaa,0x55,0xaa, // 8
  
  0xdd,0xaa,0x55,0xaa, // 9
  0xdd,0xaa,0x77,0xaa, // 10
  0xdd,0xaa,0xff,0xaa, // 11
  0xff,0xaa,0xff,0xaa, // 12
  0xff,0xee,0xff,0xaa, // 13
  0xff,0xee,0xff,0xbb, // 14
  0xff,0xee,0xff,0xff, // 15

  0xff,0xff,0xff,0xff  // 16
};

void ST7567_FB::setDither(int8_t s)
{
  if(s>16) s=16;
  if(s<-16) s=-16;
  if(s<0) {
    pattern[0] = ~pgm_read_byte(ditherTab-s*4+0);
    pattern[1] = ~pgm_read_byte(ditherTab-s*4+1);
    pattern[2] = ~pgm_read_byte(ditherTab-s*4+2);
    pattern[3] = ~pgm_read_byte(ditherTab-s*4+3);
  } else {
    pattern[0] = pgm_read_byte(ditherTab+s*4+0);
    pattern[1] = pgm_read_byte(ditherTab+s*4+1);
    pattern[2] = pgm_read_byte(ditherTab+s*4+2);
    pattern[3] = pgm_read_byte(ditherTab+s*4+3);
  }
}
// ----------------------------------------------------------------
#define ALIGNMENT \
  if(x==-1) x = SCR_WD-w; \
  else if(x<0) x = (SCR_WD-w)/2; \
  if(x<0) x=0; \
  if(x>=SCR_WD || y>=SCR_HT) return 0; \
  if(x+w>SCR_WD) w = SCR_WD-x; \
  if(y+h>SCR_HT) h = SCR_HT-y
// ----------------------------------------------------------------

int ST7567_FB::drawBitmap(const uint8_t *bmp, int x, uint8_t y, uint8_t w, uint8_t h)
{
  uint8_t wdb = w;
  ALIGNMENT;
  byte i,y8,d,b,ht8=(h+7)/8;
  for(y8=0; y8<ht8; y8++) {
    for(i=0; i<w; i++) {
      d = pgm_read_byte(bmp+wdb*y8+i);
      int lastbit = h - y8 * 8;
      if (lastbit > 8) lastbit = 8;
      for(b=0; b<lastbit; b++) {
         if(d & 1) scr[((y+y8*8+b)/8)*scrWd+x+i] |= (1 << ((y+y8*8+b)&7));
         d>>=1;
      }
    }
  }
  return x+w;
}
// ----------------------------------------------------------------
int ST7567_FB::drawBitmap(const uint8_t *bmp, int x, uint8_t y)
{
  uint8_t w = pgm_read_byte(bmp+0);
  uint8_t h = pgm_read_byte(bmp+1);
  return drawBitmap(bmp+2, x, y, w, h);
}
// ----------------------------------------------------------------
// text rendering
// ----------------------------------------------------------------
void ST7567_FB::setFont(const uint8_t* font)
{
  cfont.font = font;
  cfont.xSize = fontbyte(0);
  cfont.ySize = fontbyte(1);
  cfont.firstCh = fontbyte(2);
  cfont.lastCh  = fontbyte(3);
  cfont.minDigitWd = 0;
  cfont.minCharWd = 0;
  isNumberFun = &isNumber;
  spacing = 1;
  cr = 0;
  invertCh = 0;
}
// ----------------------------------------------------------------
int ST7567_FB::fontHeight()
{
  return cfont.ySize;
}
// ----------------------------------------------------------------
int ST7567_FB::charWidth(uint8_t c, bool last)
{
  c = convertPolish(c);
  if(c < cfont.firstCh || c > cfont.lastCh)
    return c==' ' ?  1 + cfont.xSize/2 : 0;
  if (cfont.xSize > 0) return cfont.xSize;
  int ys8=(cfont.ySize+7)/8;
  int idx = 4 + (c-cfont.firstCh)*(-cfont.xSize*ys8+1);
  int wd = pgm_read_byte(cfont.font + idx);
  int wdL = 0, wdR = spacing; // default spacing before and behind char
  if((*isNumberFun)(c) && cfont.minDigitWd>0) {
    if(cfont.minDigitWd>wd) {
      wdL = (cfont.minDigitWd-wd)/2;
      wdR += (cfont.minDigitWd-wd-wdL);
    }
  } else if(cfont.minCharWd>wd) {
    wdL = (cfont.minCharWd-wd)/2;
    wdR += (cfont.minCharWd-wd-wdL);
  }
  return last ? wd+wdL+wdR : wd+wdL+wdR-spacing;  // last!=0 -> get rid of last empty columns 
}
// ----------------------------------------------------------------
int ST7567_FB::strWidth(char *str)
{
  int wd = 0;
  while (*str) wd += charWidth(*str++);
  return wd;
}
// ----------------------------------------------------------------
int ST7567_FB::printChar(int xpos, int ypos, unsigned char c)
{
  if(xpos >= SCR_WD || ypos >= SCR_HT)  return 0;
  int fht8 = (cfont.ySize + 7) / 8, wd, fwd = cfont.xSize;
  if(fwd < 0)  fwd = -fwd;

  c = convertPolish(c);
  if(c < cfont.firstCh || c > cfont.lastCh)  return c==' ' ?  1 + fwd/2 : 0;

  int x,y8,b,cdata = (c - cfont.firstCh) * (fwd*fht8+1) + 4;
  byte d;
  wd = fontbyte(cdata++);
  int wdL = 0, wdR = spacing;
  if((*isNumberFun)(c) && cfont.minDigitWd>0) {
    if(cfont.minDigitWd>wd) {
      wdL  = (cfont.minDigitWd-wd)/2;
      wdR += (cfont.minDigitWd-wd-wdL);
    }
  } else if(cfont.minCharWd>wd) {
    wdL  = (cfont.minCharWd-wd)/2;
    wdR += (cfont.minCharWd-wd-wdL);
  }
  if(xpos+wd+wdL+wdR>SCR_WD) wdR = max(SCR_WD-xpos-wdL-wd, 0);
  if(xpos+wd+wdL+wdR>SCR_WD) wd  = max(SCR_WD-xpos-wdL, 0);
  if(xpos+wd+wdL+wdR>SCR_WD) wdL = max(SCR_WD-xpos, 0);

  for(x=0; x<wd; x++) {
    //byte mask = 1 << ((xpos+x+wdL)&7);
    for(y8=0; y8<fht8; y8++) {
      d = fontbyte(cdata+x*fht8+y8);
      int lastbit = cfont.ySize - y8 * 8;
      if (lastbit > 8) lastbit = 8;
      for(b=0; b<lastbit; b++) {
         if(d & 1) scr[((ypos+y8*8+b)/8)*scrWd+xpos+x+wdL] |= 1<<((ypos+y8*8+b)&7);  //drawPixel(xpos+x+wdL, ypos+y8*8+b, 1);
         d>>=1;
      }
    }
  }
  return wd+wdR+wdL;
}
// ----------------------------------------------------------------
int ST7567_FB::printStr(int xpos, int ypos, char *str)
{
  unsigned char ch;
  int stl, row;
  int x = xpos;
  int y = ypos;
  int wd = strWidth(str);

  if(x==-1) // right = -1
    x = SCR_WD - wd;
  else if(x<0) // center = -2
    x = (SCR_WD - wd) / 2;
  if(x<0) x = 0; // left

  while(*str) {
    int wd = printChar(x,y,*str++);
    x+=wd;
    if(cr && x>=SCR_WD) { 
      x=0; 
      y+=cfont.ySize; 
      if(y>SCR_HT) y = 0;
    }
  }
  if(invertCh) fillRect(xpos,x-1,y,y+cfont.ySize+1,2);
  return x;
}
// ----------------------------------------------------------------
bool ST7567_FB::isNumber(uint8_t ch)
{
  return isdigit(ch) || ch==' ';
}
// ---------------------------------
bool ST7567_FB::isNumberExt(uint8_t ch)
{
  return isdigit(ch) || ch=='-' || ch=='+' || ch=='.' || ch==' ';
}
// ----------------------------------------------------------------
unsigned char ST7567_FB::convertPolish(unsigned char _c)
{
  unsigned char pl, c = _c;
  if(c==196 || c==197 || c==195) {
	  dualChar = c;
    return 0;
  }
  if(dualChar) { // UTF8 coding
    switch(_c) {
      case 133: pl = 1+9; break; // 'π'
      case 135: pl = 2+9; break; // 'Ê'
      case 153: pl = 3+9; break; // 'Í'
      case 130: pl = 4+9; break; // '≥'
      case 132: pl = dualChar==197 ? 5+9 : 1; break; // 'Ò' and '•'
      case 179: pl = 6+9; break; // 'Û'
      case 155: pl = 7+9; break; // 'ú'
      case 186: pl = 8+9; break; // 'ü'
      case 188: pl = 9+9; break; // 'ø'
      //case 132: pl = 1; break; // '•'
      case 134: pl = 2; break; // '∆'
      case 152: pl = 3; break; // ' '
      case 129: pl = 4; break; // '£'
      case 131: pl = 5; break; // '—'
      case 147: pl = 6; break; // '”'
      case 154: pl = 7; break; // 'å'
      case 185: pl = 8; break; // 'è'
      case 187: pl = 9; break; // 'Ø'
      default:  return c; break;
    }
    dualChar = 0;
  } else   
  switch(_c) {  // Windows coding
    case 165: pl = 1; break; // •
    case 198: pl = 2; break; // ∆
    case 202: pl = 3; break; //  
    case 163: pl = 4; break; // £
    case 209: pl = 5; break; // —
    case 211: pl = 6; break; // ”
    case 140: pl = 7; break; // å
    case 143: pl = 8; break; // è
    case 175: pl = 9; break; // Ø
    case 185: pl = 10; break; // π
    case 230: pl = 11; break; // Ê
    case 234: pl = 12; break; // Í
    case 179: pl = 13; break; // ≥
    case 241: pl = 14; break; // Ò
    case 243: pl = 15; break; // Û
    case 156: pl = 16; break; // ú
    case 159: pl = 17; break; // ü
    case 191: pl = 18; break; // ø
    default:  return c; break;
  }
  return pl+'~'+1;
}
// ---------------------------------
