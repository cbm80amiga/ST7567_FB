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
#include "term8x10_font.h"
#include "term11x24bolddig_font.h"

void setup() 
{
  Serial.begin(9600);
  pinMode(LCD_BACKLIGHT, OUTPUT);
  //analogWrite(LCD_BACKLIGHT,30);
  digitalWrite(LCD_BACKLIGHT, LOW);
  lcd.init();
  lcd.cls();
  /*
  lcd.setFont(c64enh);
  lcd.printStr(ALIGN_CENTER, 28, "Hello World!");
  lcd.drawRect(18,20,127-18*2,63-20*2,1);
  */
  /*
  lcd.setFont(Term11x24b);
  lcd.setDigitMinWd(12);
  lcd.setCharMinWd(5);
  lcd.printStr(0, 0, "12:345678");
  */
  lcd.display();
}

int hour=13,minute=56,second=25,day=20,month=6;
float temperature=23.6;
int humidity=54;
char buf[30];

void loop() 
{
  int x;
  lcd.cls();

  lcd.setFont(Term11x24b);
  lcd.setDigitMinWd(12); lcd.setCharMinWd(5);
  snprintf(buf,25,"%d:%02d",hour,minute);
  char secs[3];
  int xp,wd = lcd.strWidth(buf);
  snprintf(secs,3,"%02d",second);
  lcd.setFont(Term8x10);
  lcd.setDigitMinWd(12); lcd.setCharMinWd(5);
  wd+=lcd.strWidth(secs);
  xp = 4+(SCR_WD-wd)/2;

  lcd.setFont(Term11x24b);
  lcd.setDigitMinWd(12);
  lcd.setCharMinWd(5);
  x=lcd.printStr(xp, 2*8, buf);
  lcd.setFont(Term8x10);
  lcd.printStr(x, 2*8, secs);

  lcd.setFont(Term8x10);
  int t1 = temperature;
  int t10 =temperature*10-t1*10;
  snprintf(buf,25,"%d.%d' ",t1,t10); lcd.printStr(1, 6*8, buf);
  snprintf(buf,25," %d%%",humidity); lcd.printStr(ALIGN_RIGHT, 6*8, buf);
  
  lcd.setFont(c64enh);
  snprintf(buf,25,"%02d.%02d ",day,month); lcd.printStr(ALIGN_LEFT, 0, buf);
  snprintf(buf,25," %s", "Thu");   lcd.printStr(ALIGN_RIGHT, 0, buf);
  lcd.display();
  delay(1000);
  second++;
  if(second>=60) { second=0;minute++; }
  if(minute>=60) { minute=0;hour++; }
}

