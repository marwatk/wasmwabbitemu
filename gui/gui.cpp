#include "gui.h"
#include "guiapp.h"
#include "calc.h"
#include "guiopenfile.h"
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

bool gif_anim_advance;
bool silent_mode = false;
int prevCalcScale;


BEGIN_EVENT_TABLE(WabbitemuFrame, wxFrame)
	EVT_PAINT(WabbitemuFrame::OnPaint)
	EVT_SIZE(WabbitemuFrame::OnResize)
	EVT_CLOSE(WabbitemuFrame::OnQuit)
	
END_EVENT_TABLE()

IMPLEMENT_APP(WabbitemuApp)

inline wxBitmap wxGetBitmapFromMemory(const unsigned char *data, int length) {
   wxMemoryInputStream is(data, length);
   return wxBitmap(wxImage(is, wxBITMAP_TYPE_PNG, -1), -1);
}

int WabbitemuFrame::gui_draw() {
	return 0;
}

extern WabbitemuFrame *frames[MAX_CALCS];
WabbitemuFrame * gui_frame(LPCALC lpCalc) {
	if (!lpCalc->scale) {
    	lpCalc->scale = 2; //Set original scale
	}
    
	WabbitemuFrame *mainFrame = new WabbitemuFrame(lpCalc);
	mainFrame->Show(true);
	frames[lpCalc->slot] = mainFrame;


	lpCalc->running = TRUE;
	return mainFrame;
}

void WabbitemuFrame::gui_frame_update() {
	wxMenuBar *wxMenu = this->GetMenuBar();
	switch(lpCalc->model) {
		default:
			lpCalc->calcSkin = wxGetBitmapFromMemory(TI_83P_png, sizeof(TI_83P_png)).ConvertToImage();
			lpCalc->keymap = wxGetBitmapFromMemory(TI_83PKeymap_png, sizeof(TI_83PKeymap_png)).ConvertToImage();
			break;
	}
	int skinWidth, skinHeight, keymapWidth, keymapHeight;
	
	if (lpCalc->calcSkin.IsOk()) {
		skinWidth = 350;//lpCalc->calcSkin.GetWidth();
		skinHeight = 725;//lpCalc->calcSkin.GetHeight();
	}
	if (lpCalc->keymap.IsOk()) {
		keymapWidth = 350;//lpCalc->keymap.GetWidth();
		keymapHeight = 725;//lpCalc->keymap.GetHeight();
	}
	int foundX = 0, foundY = 0;
	bool foundScreen = false;
	if (((skinWidth != keymapWidth) || (skinHeight != keymapHeight)) && skinHeight > 0 && skinWidth > 0) {
		lpCalc->SkinEnabled = false;
		wxMessageBox(wxT("Skin and Keymap are not the same size"), wxT("Error"),  wxOK, NULL);
	}

	if (!lpCalc->SkinEnabled) {
		// Create status bar
		wxStatusBar *wxStatus = this->GetStatusBar();
		if (wxStatus == NULL) {
			wxStatus = this->CreateStatusBar(2, wxST_SIZEGRIP, wxID_ANY );
		}
		const int iStatusWidths[] = {100, -1};
		wxStatus->SetFieldsCount(2, iStatusWidths);
		wxStatus->SetStatusText(wxEmptyString);
		wxStatus->SetStatusText(CalcModelTxt[lpCalc->model], 1);
		
		wxSize skinSize(128*lpCalc->scale, 64*lpCalc->scale);
		this->SetClientSize(skinSize);
	} else {
		wxStatusBar *wxStatus = this->GetStatusBar();
		wxSize skinSize(350, 725);
		this->SetClientSize(skinSize);
	}
	this->SendSizeEvent();
}

WabbitemuFrame::WabbitemuFrame(LPCALC lpCalc) : wxFrame(NULL, wxID_ANY, wxT("Wabbitemu"))
{
	this->lpCalc = lpCalc;
	
	wxSize skinSize(350, 725);
	lpCalc->SkinSize = skinSize;
	LCD_t *lcd = lpCalc->cpu.pio.lcd;
	int scale = lpCalc->scale;
	int draw_width = lcd->width * scale;
	int draw_height = 64 * scale;
	wxRect lcdRect((128 * scale - draw_width) / 2, 0, draw_width, draw_height);
	lpCalc->LCDRect = lcdRect;
}

void WabbitemuFrame::OnShow(wxShowEvent& event) {
}

void WabbitemuFrame::OnResize(wxSizeEvent& event) {
}

void WabbitemuFrame::OnPaint(wxPaintEvent& event)
{
}

void WabbitemuFrame::OnSetSize(wxCommandEvent &event) {
}

void WabbitemuFrame::OnSetSpeed(wxCommandEvent &event) {
}

void WabbitemuFrame::OnSetSpeedCustom(wxCommandEvent &event) {
}

void WabbitemuFrame::OnPauseEmulation(wxCommandEvent &event) {
}

void WabbitemuFrame::SetSpeed(int speed) {
}
void WabbitemuFrame::keyDown(int keycode)
{
	keyprog_t *kp = keypad_key_press(&lpCalc->cpu, keycode);
	if (kp) {
		if ((lpCalc->cpu.pio.keypad->keys[kp->group][kp->bit] & KEY_STATEDOWN) == 0) {
			lpCalc->cpu.pio.keypad->keys[kp->group][kp->bit] |= KEY_STATEDOWN;
			//for when we finally do button feedback
			this->Update();
			FinalizeButtons();
		}
	}
}

void WabbitemuFrame::keyUp(int key)
{
	if (key == WXK_SHIFT) {
		keypad_key_release(&lpCalc->cpu, WXK_LSHIFT);
		keypad_key_release(&lpCalc->cpu, WXK_RSHIFT);
	} else {
		keypad_key_release(&lpCalc->cpu, key);
	}
	FinalizeButtons();
}

void WabbitemuFrame::OnQuit(wxCloseEvent& event)
{
}

void WabbitemuFrame::FinalizeButtons() {
	int group, bit;
	keypad_t *kp = lpCalc->cpu.pio.keypad;
	for(group = 0; group < 7; group++) {
		for(bit = 0; bit < 8; bit++) {
			if ((kp->keys[group][bit] & KEY_STATEDOWN) &&
				((kp->keys[group][bit] & KEY_MOUSEPRESS) == 0) &&
				((kp->keys[group][bit] & KEY_KEYBOARDPRESS) == 0)) {
					kp->keys[group][bit] &= (~KEY_STATEDOWN);
			}
		}
	}
}
