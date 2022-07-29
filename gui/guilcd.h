#ifndef GUILCD_WX_H
#define GUILCD_WX_H

#include "calc.h"
#include "lcd.h"
#include <stdlib.h>

#include <SDL.h>

#define MAX_SHADES 255
class WabbitemuLCD
{
public:
private:

	unsigned char redColors[MAX_SHADES+1];
	unsigned char greenColors[MAX_SHADES+1];
	unsigned char blueColors[MAX_SHADES+1];

	bool hasDrawnLCD;
	LPCALC lpCalc;
};

#endif
