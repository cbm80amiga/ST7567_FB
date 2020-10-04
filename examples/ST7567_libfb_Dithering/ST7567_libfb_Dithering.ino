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
#include "tiny3x7sq_font.h"

char buf[20];

void setup() 
{
  Serial.begin(9600);
  pinMode(LCD_BACKLIGHT, OUTPUT);
  //analogWrite(LCD_BACKLIGHT,30);
  digitalWrite(LCD_BACKLIGHT, LOW);
  lcd.init();
  lcd.cls();
  for(int j=1;j<17;j++) { lcd.setDither(j); lcd.fillRectD((j-1)*8,0,8,64,1); }
  lcd.display(); delay(5000);
}

void sample(int s)
{
  lcd.cls();
  lcd.setDither(s);
  lcd.setFont(c64enh);
  snprintf(buf,20,"Pattern: %02d",s);
  lcd.printStr(ALIGN_CENTER, 2, buf);
  lcd.fillRectD(4,13,68,32,1);
  lcd.fillCircleD(100,28,16,1);
  for(int j=1;j<17;j++) {
    lcd.setFont(Tiny3x7SqPL);
    snprintf(buf,20,"%2d",j);
    lcd.printStr((j-1)*8, 64-8*2, buf);
    lcd.setDither(j);
    lcd.fillRectD((j-1)*8,64-8,8,8,1);
  }
  lcd.display();
  delay(500);
}

void loop() 
{
  for(int i=0;i<=16;i++) sample(i);
  for(int i=16;i>0;i--) sample(i);
}

