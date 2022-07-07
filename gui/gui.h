#include "stdafx.h"

#ifndef GUI_WX_H
#define GUI_WX_H

#include <wx/statusbr.h>
#include <wx/frame.h>
#include <wx/numdlg.h>
#include <wx/dnd.h>
#include <wx/mstream.h>
#include <wx/filename.h>
#include <wx/config.h>
#include <sys/time.h>
#if (wxUSE_UNICODE)
#include <wx/encconv.h> 
#endif

#include "guilcd.h"
#include "calc.h"

enum
{
	ID_LCD,
};

class WabbitemuFrame: public wxFrame
{
public:
    WabbitemuFrame(LPCALC);
   	wxWindow *wxLCD;
    
	void keyDown(int keycode);
	void keyUp(int keycode);

	void OnKeyDown(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void SetSpeed(int speed);
	int gui_draw();
	void gui_frame_update();
	void OnTimer(wxTimerEvent& event);
protected:
	DECLARE_EVENT_TABLE()
private:
	bool is_resizing;

	void OnPauseEmulation(wxCommandEvent& event);
	void OnTurnCalcOn(wxCommandEvent& event);
	
	void OnSetSpeed(wxCommandEvent& event);
	void OnSetSpeedCustom(wxCommandEvent& event);
	void OnSetSize(wxCommandEvent& event);
		
	void OnPaint(wxPaintEvent& event);
	// Resize
	void OnResize(wxSizeEvent& event);
	void OnShow(wxShowEvent& event);
	
	// We use "isShownVar" instead of "isShown" because "isShown" is
	// an existing wxWidgets method
	int isShownVar = 0;
	
	LPCALC lpCalc;
	
	void OnLeftButtonDown(wxMouseEvent& event);
	void OnLeftButtonUp(wxMouseEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnQuit(wxCloseEvent& event);
	void FinalizeButtons();
};
int SetGIFName();
WabbitemuFrame* gui_frame(LPCALC lpCalc);
#endif
