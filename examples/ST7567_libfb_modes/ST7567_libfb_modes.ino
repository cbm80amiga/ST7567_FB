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

#define LCD_BL  6
#define LCD_CS  10
#define LCD_DC  8
#define LCD_RST 9

#include "ST7567_FB.h"
#include <SPI.h>
ST7567_FB lcd(LCD_DC, LCD_RST, LCD_CS);

#if USESPI==1
#include <SPI.h>
#endif

#include "small4x7_font.h"

void setup() 
{
  Serial.begin(9600);
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, LOW);
  lcd.init();
}

// --------------------------------------------------------------------------
char buf[40];

void loop() 
{
  int i;
  int yt=(SCR_HT-8)/2;
  lcd.setFont(Small4x7PL);

  lcd.cls();
  lcd.drawRect(0,0,SCR_WD,SCR_HT,1);
  lcd.printStr(ALIGN_CENTER, yt, "ST7567_INVERT_OFF");
  lcd.display();
  lcd.displayInvert(false);
  delay(3000);
  lcd.fillRect(10,yt,SCR_WD-20,8,0);
  lcd.printStr(ALIGN_CENTER, yt, "ST7567_INVERT_ON");
  lcd.display();
  lcd.displayInvert(true);
  delay(3000);
  lcd.displayInvert(false);

  lcd.fillRect(10,yt,SCR_WD-20,8,0);
  lcd.printStr(ALIGN_CENTER, yt, "ST7567_DISPLAY_OFF");
  lcd.display();
  delay(1000);
  lcd.displayOn(false);
  delay(1000);
  lcd.fillRect(10,yt,SCR_WD-20,8,0);
  lcd.printStr(ALIGN_CENTER, yt, "ST7567_DISPLAY_ON");
  lcd.display();
  lcd.displayOn(true);
  delay(2000);

  for(i=0;i<=20;i+=2) {
    snprintf(buf,40,"CONTRAST %d", i);
    lcd.fillRect(10,yt,SCR_WD-20,8,0);
    lcd.printStr(ALIGN_CENTER, yt, buf);
    lcd.display();
    lcd.setContrast(i);
    delay(1000);
  }
  lcd.fillRect(10,yt,SCR_WD-20,8,0);
  lcd.printStr(ALIGN_CENTER, yt, "CONTRAST 7");
  lcd.display();
  lcd.setContrast(7);
  delay(2000);

  for(i=0;i<256;i+=10) {
    snprintf(buf,40,"BACKLIGHT %d", i);
    lcd.fillRect(10,yt,SCR_WD-20,8,0);
    lcd.printStr(ALIGN_CENTER, yt, buf);
    lcd.display();
    analogWrite(LCD_BL,i);
    delay(400);
  }
  digitalWrite(LCD_BL, LOW);
  lcd.fillRect(10,yt,SCR_WD-20,8,0);
  lcd.printStr(ALIGN_CENTER, yt, "BACKLIGHT MAX");
  lcd.display();
  delay(1000);

  lcd.cls();
  lcd.printStr(0, 0*8, "0");
  lcd.printStr(0, 1*8, "1");
  lcd.printStr(0, 2*8, "2");
  lcd.printStr(0, 3*8, "3");
  lcd.printStr(0, 4*8, "4");
  lcd.printStr(0, 5*8, "5");
  lcd.printStr(0, 6*8, "6");
  lcd.printStr(0, 7*8, "7");

  lcd.fillRect(10,yt,SCR_WD-10,8,0);
  lcd.printStr(ALIGN_CENTER, yt, " ROTATE 180 ");
  lcd.setRotation(2);
  lcd.display();
  delay(3000);
  lcd.fillRect(10,yt,SCR_WD-10,8,0);
  lcd.printStr(ALIGN_CENTER, yt, "   ROTATE 0  ");
  lcd.setRotation(0);
  lcd.display();
  delay(2000);

  for(i=0;i<64;i++) {
    lcd.fillRect(10,yt,SCR_WD-10,8,0);
    snprintf(buf,40,"    SCROLL %02d    ", i);
    lcd.printStr(ALIGN_CENTER, yt, buf);
    lcd.display();
    lcd.setScroll(i);
    delay(200);
  }
  lcd.fillRect(10,yt,SCR_WD-10,8,0);
  lcd.printStr(ALIGN_CENTER, 4, "  SCROLL 0  ");
  lcd.display();
  lcd.setScroll(0);
  delay(2000);
}

