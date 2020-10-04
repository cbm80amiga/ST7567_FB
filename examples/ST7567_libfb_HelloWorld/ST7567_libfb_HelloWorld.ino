// ST7567_FB example
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

#define LCD_BACKLIGHT  6
#define LCD_CS         10
#define LCD_DC         8
#define LCD_RST        9

#include "ST7567_FB.h"
#include <SPI.h>
ST7567_FB lcd(LCD_DC, LCD_RST, LCD_CS);

// from PropFonts library
#include "c64enh_font.h"

void setup() 
{
  Serial.begin(9600);
  pinMode(LCD_BACKLIGHT, OUTPUT);
  //analogWrite(LCD_BACKLIGHT,160);
  digitalWrite(LCD_BACKLIGHT, LOW);

  lcd.init();
  lcd.cls();
  lcd.setFont(c64enh);
  lcd.printStr(ALIGN_CENTER, 28, "Hello World!");
  lcd.drawRectD(0,0,128,64,1);
  lcd.drawRect(18,20,127-18*2,63-20*2,1);
  lcd.display();
}

void loop() 
{
}

