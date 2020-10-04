# ST7567_FB
Fast SPI graphics library for ST7567 128x64 LCD with frame buffer.

Tested on Arduino IDE 1.6.5 and OPEN-SMART 2.6" display module.

YouTube video:

https://youtu.be/YC-wqZnCGQ4

## Features

- proportional fonts support built-in (requires fonts from PropFonts library https://github.com/cbm80amiga/PropFonts)
- simple primitives
  - pixels
  - lines
  - rectangles
  - filled rectangles
  - circles
  - filled circles
  - triangles
  - filled triangles
- fast ordered dithering (17 patterns)
- ultra fast horizontal and vertical line drawing
- bitmaps drawing
- example programs

## Connections:

|LCD pin|LCD pin name|Arduino|
|--|--|--|
 |#01| LED| D6, GND or any pin via resistor|
 |#02| RST| D9 or any pin|
 |#03| CS | D10 or any pin|
 |#04| DC | D8 or any pin|
 |#05| SCK| D13/SCK|
 |#06| SDI| D11/MOSI|
 |#07| 3V3| VCC (3.3V)|
 |#08| GND| GND|
 
If you find it useful and want to buy me a coffee or a beer:

https://www.paypal.me/cbm80amiga

