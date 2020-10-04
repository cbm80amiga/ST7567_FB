// ST7567_FB example
// Requires RRE Font library
// (C)2020 Pawel A. Hernik

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

#define LCD_BL         6
#define LCD_CS         10
#define LCD_DC         8
#define LCD_RST        9

#include "ST7567_FB.h"
#include <SPI.h>
ST7567_FB lcd(LCD_DC, LCD_RST, LCD_CS);

#include "RREFont.h"
#include "rre_arialdig47b.h"
#include "rre_arialbldig54b.h"
#include "rre_arialdig62b.h"

RREFont font;

// needed for RREFont library initialization, define your fillRect
void customRect(int x, int y, int w, int h, int c) { return lcd.fillRect(x, y, w, h, c); }
void customRectDither(int x, int y, int w, int h, int c) { return lcd.fillRectD(x, y, w, h, c); }

char buf[20];

void setup() 
{
  Serial.begin(9600);
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, LOW);
  lcd.init();
  lcd.cls();
  font.init(customRect, SCR_WD, SCR_HT); // custom fillRect function and screen width and height values

  font.setFont(&rre_ArialBlDig54b);  font.setDigitMinWd(46);
  font.setFillRectFun(customRectDither);
/*
  lcd.cls();
  lcd.setDither(1);
  lcd.fillRectD(0,13,32,32,1);
//  font.printStr(0,0,"8");
  lcd.display();
  delay(1000);
  //lcd.cls();
  lcd.setDither(-1);
  lcd.fillRectD(0,13,32,32,1);
  //font.printStr(0,0,"8");
  lcd.display();
  delay(5000);*/
}


int cnt=0;

void loop() 
{
  if(++cnt>99) cnt=1;
  //font.setFont(&rre_ArialDig47b);  font.setDigitMinWd(31);
  font.setFont(&rre_ArialBlDig54b);  font.setDigitMinWd(46);
  //font.setFont(&rre_ArialDig62b);  font.setDigitMinWd(43);
  font.setFillRectFun(customRectDither);
  for(int i=1;i<=16;i++) {
    lcd.cls();
    lcd.setDither(-i);
    snprintf(buf,5,"%02d",cnt-1);
    font.printStr(ALIGN_CENTER,(SCR_HT-font.getHeight())/2,buf);

    lcd.setDither(i);
    snprintf(buf,5,"%02d",cnt);
    font.printStr(ALIGN_CENTER,(SCR_HT-font.getHeight())/2,buf);
    lcd.display();
    delay(50);
  }
  delay(500);
}

