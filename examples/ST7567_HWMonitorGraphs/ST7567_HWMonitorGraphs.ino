// Arduino HWMonitor - version with 4 separate graphs
// (C)2019-20 Pawel A. Hernik

/*
 Use PC HardwareSerialMonitor from:
 https://cdn.hackaday.io/files/19018813666112/HardwareSerialMonitor_v1.1.zip
*/

// comment out for regular mode
//#define GRAPH_BAR


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
#include "small4x6_font.h"

String inputString = "";
String cpuLoadString;
String cpuTempString;
String cpuClockString;
String ramString;
char buf[30];
int cpuLoad;
int cpuClock;
int cpuTemp;
int usedRam;
int inp=0;

void setup() 
{
  Serial.begin(9600);
  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, LOW);
  inputString.reserve(200);
  
  SPI.begin();
  lcd.init();
  lcd.cls();
  lcd.setFont(c64enh);
  lcd.printStr(ALIGN_CENTER, 28, "Connecting ...");
  lcd.drawRect(16,20,127-16*2,63-20*2,1);
  lcd.display();
}

int readSerial() 
{
  while (Serial.available()) {
    char ch = (char)Serial.read();
    inputString += ch;

    /// for debug
    //lcd.fillRect(x,y,16,8,0); lcd.printChar(x,y,ch);  lcd.display(0);
    //x+=8; if(x>=128) { x=0;y+=8; }
    //if(y>=64) y=0;
  
    if(ch == '|') {  // full info chunk received
      int st,en;
      st = inputString.indexOf("CHC");  // CPU clock: "CHC1768"
      if(st>=0) {
        en = inputString.indexOf("|", st);
        cpuClockString = inputString.substring(st+3, en);
        cpuClock = cpuClockString.toInt();
        inp=3;
      } else {

        st = inputString.indexOf("R");  // used RAM: "R6.9"
        if(st>=0) {
          en = inputString.indexOf("|", st);
          ramString = inputString.substring(st+1 , en-1);
          st = ramString.indexOf(",");
          if(st>=0) ramString.setCharAt(st,'.');
          usedRam = ramString.toFloat()*1024;
          inp=2;
        }

        int cpuTempStart = inputString.indexOf("C"); // CPU temperature: "C52"
        int cpuLoadStart = inputString.indexOf("c"); // CPU load: "c18%"
        if(cpuLoadStart>=0 && cpuTempStart>=0) {
          en = inputString.indexOf("|");
          cpuTempString = inputString.substring(cpuTempStart+1, cpuLoadStart);
          cpuLoadString = inputString.substring(cpuLoadStart+1, en-1);
          cpuTemp = cpuTempString.toInt();
          cpuLoad = cpuLoadString.toInt();
          inp=1;
        }
      }
      inputString = "";
      return 1;
    }
  }
  return 0;
}

#define MAX_RAM 16738
#define MIN_CLOCK 400
#define MAX_CLOCK 2900
#define MIN_TEMP 30
#define MAX_TEMP 80
#define NUM_VAL (16)
int tempTab[NUM_VAL];
int loadTab[NUM_VAL];
int clockTab[NUM_VAL];
int ramTab[NUM_VAL];
int i,ght=31-7;
int x=0;

void addLoad()
{
  for(i=0;i<NUM_VAL-1;i++) loadTab[i]=loadTab[i+1];
  loadTab[NUM_VAL-1] = cpuLoad*(ght-1)/100;
}

void addClock()
{
  for(i=0;i<NUM_VAL-1;i++) clockTab[i]=clockTab[i+1];
  int t = cpuClock;
  if(t<MIN_CLOCK) t=MIN_CLOCK;
  if(t>MAX_CLOCK) t=MAX_CLOCK;
  clockTab[NUM_VAL-1] = (long)(t-MIN_CLOCK)*ght/(MAX_CLOCK-MIN_CLOCK);
}

void addTemp()
{
  for(i=0;i<NUM_VAL-1;i++) tempTab[i]=tempTab[i+1];
  int t = cpuTemp;
  if(t<MIN_TEMP) t=MIN_TEMP;
  if(t>MAX_TEMP) t=MAX_TEMP;
  tempTab[NUM_VAL-1] = (long)(t-MIN_TEMP)*ght/(MAX_TEMP-MIN_TEMP);
}

void addRAM()
{
  for(i=0;i<NUM_VAL-1;i++) ramTab[i]=ramTab[i+1];
  ramTab[NUM_VAL-1] = (long)usedRam*ght/MAX_RAM;
}

void drawGraph(int xg, int yg, int valTab[])
{
  lcd.drawRectD(xg,yg,62,ght,1);
#ifdef GRAPH_BAR
  for(i=1;i<NUM_VAL;i++) {
    lcd.fillRectD(1+xg+(i-1)*4,yg+ght-1-valTab[i],4,valTab[i],1);
    lcd.drawLineH(1+xg+(i-1)*4,1+xg+(i+0)*4-1,yg+ght-1-valTab[i],1);
    if(i>1) lcd.drawLineV(1+xg+(i-1)*4,yg+ght-1-valTab[i-1],yg+ght-1-valTab[i],1);
  }
#else
  for(i=0;i<NUM_VAL-1;i++) {
    int dy = valTab[i+1]-valTab[i];
    for(int j=0;j<4;j++) lcd.drawLineVfastD(1+xg+i*4+j,yg+ght-1-(valTab[i]+dy*j/4),yg+ght-1,1);
    lcd.drawLine(1+xg+i*4,yg+ght-1-valTab[i],1+xg+(i+1)*4,yg+ght-1-valTab[i+1],1);
  }
#endif
}

void loop() 
{
  readSerial();
  //if(readSerial()) 
  {
    int xs=68;
    lcd.cls();
    lcd.setFont(Small4x6PL);
    snprintf(buf,30,"Temp: %d'C",cpuTemp); lcd.printStr(9, 0, buf);
    snprintf(buf,30,"Load: %d%%",cpuLoad); lcd.printStr(13+64, 0, buf);
    snprintf(buf,30,"Clock: %dMHz",cpuClock); lcd.printStr(0, 32+32-6, buf);
    snprintf(buf,30,"RAM: %dMB",usedRam); lcd.printStr(7+64, 32+32-6, buf);

    if(inp==3) addClock();
    if(inp==2) addRAM();
    if(inp==1) { addLoad(); addTemp(); }
    drawGraph(0,7,tempTab);
    drawGraph(66,7,loadTab);
    drawGraph(0,33,clockTab);
    drawGraph(66,33,ramTab);
    lcd.display();
  }
  if(inp>=3) { delay(1000); inp=0; }
}

