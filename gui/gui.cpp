#include "gui.h"
#include "guiapp.h"
#include "calc.h"
#include "keys.h"
#include "sendfile.h"
#include "wabbiticon.xpm"

#include "skins/ti73.h"
#include "skins/ti81.h"
#include "skins/ti82.h"
#include "skins/ti83.h"
#include "skins/ti83p.h"
#include "skins/ti83pse.h"
#include "skins/ti84p.h"
#include "skins/ti84pse.h"
#include "skins/ti85.h"
#include "skins/ti86.h"

#include "gif.h"
#include "gifhandle.h"

#define BIG_WINDOWS_ICON 0
#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif


BEGIN_EVENT_TABLE(WabbitemuFrame, wxFrame)
	EVT_CLOSE(WabbitemuFrame::OnQuit)
	
END_EVENT_TABLE()

inline wxBitmap wxGetBitmapFromMemory(const unsigned char *data, int length) {
   wxMemoryInputStream is(data, length);
   return wxBitmap(wxImage(is, wxBITMAP_TYPE_PNG, -1), -1);
}

int WabbitemuFrame::gui_draw() {
	return 0;
}

WabbitemuFrame * gui_frame(LPCALC lpCalc) {
	if (!lpCalc->scale) {
    	lpCalc->scale = 2; //Set original scale
	}
    
	WabbitemuFrame *mainFrame = new WabbitemuFrame(lpCalc);
	return mainFrame;
}

void WabbitemuFrame::gui_frame_update() {
}

WabbitemuFrame::WabbitemuFrame(LPCALC lpCalc) : wxFrame(NULL, wxID_ANY, wxT("Wabbitemu"))
{
	this->lpCalc = lpCalc;
}


void WabbitemuFrame::OnQuit(wxCloseEvent& event)
{
}

