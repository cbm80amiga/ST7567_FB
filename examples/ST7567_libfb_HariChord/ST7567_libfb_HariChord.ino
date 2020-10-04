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

#define LCD_BL         6
#define LCD_CS         10
#define LCD_DC         8
#define LCD_RST        9

#include "ST7567_FB.h"
#include <SPI.h>
ST7567_FB lcd(LCD_DC, LCD_RST, LCD_CS);

void setup() 
{
  Serial.begin(9600);
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, LOW);
  lcd.init();
}

int nFrames = 36;

void loop() 
{
  for (int i=0; i<nFrames; i++) HariChord(i);
  for (int i=(nFrames-1); i >= 0; i--) HariChord(i);
}

// by Hari Wiguna
void HariChord(int frame)
{
  lcd.clearDisplay();
  int n = 7;
  int r = frame * 64 / nFrames;
  float rot = frame * 2*PI / nFrames;
  for (int i=0; i<(n-1); i++) {
    float a = rot + i * 2*PI / n;
    int x1 = SCR_WD/2 + cos(a) * r;
    int y1 = SCR_HT/2 + sin(a) * r;
    for (int j=i+1; j<n; j++) {
      a = rot + j * 2*PI / n;
      int x2 = SCR_WD/2 + cos(a) * r;
      int y2 = SCR_HT/2 + sin(a) * r;
      lcd.drawLine(x1,y1, x2,y2, SET);
    }
  }
  lcd.display();
  delay(50);
}

