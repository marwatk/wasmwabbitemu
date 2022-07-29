#include "stdafx.h"

#ifndef GUI_WX_H
#define GUI_WX_H

#include <sys/time.h>
#if (wxUSE_UNICODE)
#endif

#include "guilcd.h"
#include "calc.h"

enum
{
	ID_LCD,
};

class WabbitemuFrame
{
public:
    WabbitemuFrame(LPCALC);
    

	void SetSpeed(int speed);
	int gui_draw();
	void gui_frame_update();
private:
	bool is_resizing;

	
	// We use "isShownVar" instead of "isShown" because "isShown" is
	// an existing wxWidgets method
	int isShownVar = 0;
	
	LPCALC lpCalc;
};
int SetGIFName();
WabbitemuFrame* gui_frame(LPCALC lpCalc);
#endif
