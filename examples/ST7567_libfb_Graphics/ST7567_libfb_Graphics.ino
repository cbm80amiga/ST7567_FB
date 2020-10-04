// ST7567 LCD FB library example
// (c) 2019-20 Pawel A. Hernik

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

#define LCD_BL  6
#define LCD_CS  10
#define LCD_DC  8
#define LCD_RST 9

#include "ST7567_FB.h"
#include <SPI.h>
ST7567_FB lcd(LCD_DC, LCD_RST, LCD_CS);

void setup() 
{
  Serial.begin(9600);
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, LOW);
  lcd.init();
  lcd.cls();
  lcd.drawRectD(0,0,SCR_WD,SCR_HT,1);

  int x=16,y=10;
  lcd.drawRect(x+8,y-5,20,20,1);
  lcd.fillRect(x+8+30,y-5,20,20,1);
  lcd.fillRectD(x+8+60,y-5,20,20,1);

  lcd.drawCircle(x+5+12,48,y,1);
  lcd.fillCircle(x+5+30+12,y+36,12,1);
  lcd.fillCircleD(x+5+60+12,y+36,12,1);

  lcd.display();
}

void loop() 
{
}

