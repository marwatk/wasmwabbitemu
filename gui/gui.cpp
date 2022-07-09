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
enum
{
	ID_File_New,
	ID_File_Open,
	ID_File_Save,
	ID_File_Gif,
	ID_File_Close,
	ID_File_Quit,

	ID_View_Skin,
	ID_View_Vars,

	ID_Calc_Copy,
	ID_Calc_Sound,
	ID_Calc_Pause,
	ID_Calc_Connect,
	ID_Calc_Options,
	
	ID_Speed_400,
	ID_Speed_500,
	ID_Speed_200,
	ID_Speed_100,
	ID_Speed_50,
	ID_Speed_25,
	ID_Speed_Custom,
	
	ID_Size_100,
	ID_Size_200,
	ID_Size_300,
	ID_Size_400,
	
	ID_Debug_Reset,
	ID_Debug_Open,
	ID_Debug_On,
	
	ID_Help_Setup,
	ID_Help_About,
	ID_Help_Website
};

BEGIN_EVENT_TABLE(WabbitemuFrame, wxFrame)
	EVT_PAINT(WabbitemuFrame::OnPaint)
	EVT_SIZE(WabbitemuFrame::OnResize)
	
	EVT_MENU(ID_Calc_Pause, WabbitemuFrame::OnPauseEmulation)
	EVT_MENU(ID_Speed_Custom, WabbitemuFrame::OnSetSpeedCustom)
	EVT_MENU(ID_Speed_500, WabbitemuFrame::OnSetSpeed)
	EVT_MENU(ID_Speed_400, WabbitemuFrame::OnSetSpeed)
	EVT_MENU(ID_Speed_200, WabbitemuFrame::OnSetSpeed)
	EVT_MENU(ID_Speed_100, WabbitemuFrame::OnSetSpeed)
	EVT_MENU(ID_Speed_50, WabbitemuFrame::OnSetSpeed)
	EVT_MENU(ID_Speed_25, WabbitemuFrame::OnSetSpeed)
	
	EVT_MENU(ID_Size_100, WabbitemuFrame::OnSetSize)
	EVT_MENU(ID_Size_200, WabbitemuFrame::OnSetSize)
	EVT_MENU(ID_Size_300, WabbitemuFrame::OnSetSize)
	EVT_MENU(ID_Size_400, WabbitemuFrame::OnSetSize)
	
	EVT_KEY_DOWN(WabbitemuFrame::OnKeyDown)
	EVT_KEY_UP(WabbitemuFrame::OnKeyUp)
	EVT_CLOSE(WabbitemuFrame::OnQuit)
	
	EVT_LEFT_DOWN(WabbitemuFrame::OnLeftButtonDown)
	EVT_LEFT_UP(WabbitemuFrame::OnLeftButtonUp)
END_EVENT_TABLE()

IMPLEMENT_APP(WabbitemuApp)

inline wxBitmap wxGetBitmapFromMemory(const unsigned char *data, int length) {
   wxMemoryInputStream is(data, length);
   return wxBitmap(wxImage(is, wxBITMAP_TYPE_PNG, -1), -1);
}

int WabbitemuFrame::gui_draw() {

	if (lpCalc->gif_disp_state != GDS_IDLE) {
		static int skip = 0;
		if (skip == 0) {
			gif_anim_advance = true;
			this->Update();
		}
		skip = (skip + 1) % 4;
	}
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
	mainFrame->SetSpeed(100);
	
	mainFrame->Center();   //Centres the frame
	
	mainFrame->gui_frame_update();
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
	} else {
		lpCalc->SkinSize.SetWidth(skinWidth);
		lpCalc->SkinSize.SetHeight(skinHeight);		//find the screen size
		for(int y = 0; y < skinHeight && foundScreen == false; y++) {
			for (int x = 0; x < skinWidth && foundScreen == false; x++) {
				if (lpCalc->keymap.GetBlue(x, y) == 0 &&
						lpCalc->keymap.GetRed(x, y) == 255 &&
						lpCalc->keymap.GetGreen(x, y) == 0) {
					//81 92
					foundX = x;
					foundY = y;
					foundScreen = true;
				}
			}
		}
		lpCalc->LCDRect.SetLeft(foundX);
		lpCalc->LCDRect.SetTop(foundY);
		do {
			foundX++;
		} while (lpCalc->keymap.GetBlue(foundX, foundY) == 0 &&
				lpCalc->keymap.GetRed(foundX, foundY) == 255 &&
				lpCalc->keymap.GetGreen(foundX, foundY) == 0);
		lpCalc->LCDRect.SetRight(--foundX);
		do {
			foundY++;
		}while (lpCalc->keymap.GetBlue(foundX, foundY) == 0 &&
				lpCalc->keymap.GetRed(foundX, foundY) == 255 &&
				lpCalc->keymap.GetGreen(foundX, foundY) == 0);
		lpCalc->LCDRect.SetBottom(--foundY);
	}
	if (!foundScreen) {
		wxMessageBox(wxT("Unable to find the screen box"), wxT("Error"), wxOK, NULL);
		lpCalc->SkinEnabled = false;
	}

	if (!lpCalc->SkinEnabled) {
		if (wxMenu != NULL) {
			wxMenu->Check(ID_View_Skin, false);
		}
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
		if (wxMenu != NULL) {
			wxMenu->Check(ID_View_Skin, true);
		}
		wxStatusBar *wxStatus = this->GetStatusBar();
		if (wxStatus != NULL) {
			wxStatus->Destroy();
		}
		wxSize skinSize(350, 725);
		this->SetClientSize(skinSize);
	}
	this->SendSizeEvent();
}

WabbitemuFrame::WabbitemuFrame(LPCALC lpCalc) : wxFrame(NULL, wxID_ANY, wxT("Wabbitemu"))
{
	this->lpCalc = lpCalc;
	
	this->SetWindowStyleFlag(wxBORDER_RAISED);
	wxSize skinSize(350, 725);
	lpCalc->SkinSize = skinSize;
	LCD_t *lcd = lpCalc->cpu.pio.lcd;
	int scale = lpCalc->scale;
	int draw_width = lcd->width * scale;
	int draw_height = 64 * scale;
	wxRect lcdRect((128 * scale - draw_width) / 2, 0, draw_width, draw_height);
	lpCalc->LCDRect = lcdRect;

	wxSize windowSize;
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxMenuBar *m_menubar = new wxMenuBar( 0 );
		
	windowSize.Set(128 * scale, 64 * scale);
	
	this->Connect(wxEVT_SHOW, (wxObjectEventFunction) &WabbitemuFrame::OnShow);
	
	this->SetSize(windowSize);


}

void WabbitemuFrame::OnShow(wxShowEvent& event) {
	this->isShownVar = 1;
}

void WabbitemuFrame::OnResize(wxSizeEvent& event) {
	event.Skip(true);
	if (lpCalc->SkinEnabled ) {
		return;
	}
	if (is_resizing) {
		return;
	}
	
	int width_scale = GetClientSize().GetWidth() / 128;
	int height_scale = GetClientSize().GetHeight() / 64;
	int scale = max(2, max(width_scale, height_scale));

	lpCalc->scale = min(2, scale);
	wxSize size(scale * 128, scale * 64);
	is_resizing = true;
	this->SetClientSize(size);
	this->Move(scale * (128 - lpCalc->cpu.pio.lcd->width), 0);
	is_resizing = false;
}

void WabbitemuFrame::OnPaint(wxPaintEvent& event)
{
}

void WabbitemuFrame::OnSetSize(wxCommandEvent &event) {
	/* This function is called when user changes size of LCD in menu */
    wxMenuBar *wxMenu = this->GetMenuBar();
    if (lpCalc->SkinEnabled) {
		return;
	}
	int eventID;
	wxMenu->Check(ID_Size_100,false);
	wxMenu->Check(ID_Size_200,false);
	wxMenu->Check(ID_Size_300,false);
	wxMenu->Check(ID_Size_400,false);
	
	eventID = event.GetId();
	
	switch (eventID) {
		case ID_Size_100:
			lpCalc->scale = 1;  //This is half of the Wabbit default, but equals real LCD
			wxMenu->Check(ID_Size_100, true);
			break;
		case ID_Size_200:
			lpCalc->scale = 2; //Wabbit default, twice the LCD
			wxMenu->Check(ID_Size_200, true);
			break;
		case ID_Size_300:
			lpCalc->scale = 3;
			wxMenu->Check(ID_Size_300, true);
			break;
		case ID_Size_400:
			lpCalc->scale = 4;
			wxMenu->Check(ID_Size_400, true);
			break;
	}
	if (!lpCalc->SkinEnabled) {
		/* Update size of frame to match the new LCD Size */
		this->SetSize(128 * lpCalc->scale, 64 * lpCalc->scale);
	}
}

void WabbitemuFrame::OnSetSpeed(wxCommandEvent &event) {
	wxMenuBar *wxMenu = this->GetMenuBar();
	int eventID;
	wxMenu->Check(ID_Speed_500, false);
	wxMenu->Check(ID_Speed_400, false);
	wxMenu->Check(ID_Speed_200, false);
	wxMenu->Check(ID_Speed_100, false);
	wxMenu->Check(ID_Speed_50, false);
	wxMenu->Check(ID_Speed_25, false);
	wxMenu->Check(ID_Speed_Custom, false);
	wxMenu->SetLabel(ID_Speed_Custom, wxString(wxT("Custom...")));
	wxMenu->Check(ID_Speed_Custom, false);
	
	eventID = event.GetId();
	switch (eventID) {
		case ID_Speed_25:
			this->SetSpeed(25);
			wxMenu->Check(ID_Speed_25, true);
			break;
		case ID_Speed_50:
			this->SetSpeed(50);
			wxMenu->Check(ID_Speed_50, true);
			break;
		case ID_Speed_100:
			this->SetSpeed(100);
			wxMenu->Check(ID_Speed_100, true);
			break;
		case ID_Speed_200:
			this->SetSpeed(200);
			wxMenu->Check(ID_Speed_200, true);
			break;
		case ID_Speed_400:
			this->SetSpeed(400);
			wxMenu->Check(ID_Speed_400, true);
			break;
		case ID_Speed_500:
			this->SetSpeed(500);
			wxMenu->Check(ID_Speed_500, true);
			break;
	}
}

void WabbitemuFrame::OnSetSpeedCustom(wxCommandEvent &event) {
	wxMenuBar *wxMenu = this->GetMenuBar();
	long resp;
	resp = wxGetNumberFromUser(wxString(wxT("Enter the speed (in percentage) you wish to set:")), wxString(wxT("")), wxString(wxT("Wabbitemu - Custom Speed")), 100, 0, 10000);
	if (resp != -1) {
		wxMenu->SetLabel(ID_Speed_Custom, wxString::Format(wxT("%i%%"),resp));
		wxMenu->Check(ID_Speed_500, false);
		wxMenu->Check(ID_Speed_400, false);
		wxMenu->Check(ID_Speed_200, false);
		wxMenu->Check(ID_Speed_100, false);
		wxMenu->Check(ID_Speed_50, false);
		wxMenu->Check(ID_Speed_25, false);
		wxMenu->Check(ID_Speed_Custom, true);
		this->SetSpeed(resp);
	} else {
		// note to self: does the "entered something invalid" apply?? supposedly, OnSetSpeedCustom is checked...
		/* Dirty, evil hack... but I'm too lazy to create another var to indicate
		 * custom speed status, soo... :P */
		if (wxMenu->GetLabel(ID_Speed_Custom) == wxString(wxT("Custom..."))) {
			// Do nothing
			wxMenu->Check(ID_Speed_Custom, false);
		} else {
			wxMenu->Check(ID_Speed_Custom, true);
		}
	}
}

void WabbitemuFrame::OnPauseEmulation(wxCommandEvent &event) {
	wxMenuBar *wxMenu = this->GetMenuBar();
	if (lpCalc->running) {
		//Tick is checked and emulation stops
		lpCalc->running = FALSE;
		wxMenu->Check(ID_Calc_Pause, true);
	} else {
		//Tick is unchecked and emulation resumes
		lpCalc->running = TRUE;
		wxMenu->Check(ID_Calc_Pause, false);
	}
}

void WabbitemuFrame::SetSpeed(int speed) {
	lpCalc->speed = speed;
}
void WabbitemuFrame::keyDown(int keycode)
{
	if (keycode == WXK_F8) {
		if (lpCalc->speed == 100) {
			SetSpeed(400);
		} else {
			SetSpeed(100);
		}
	}

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

void WabbitemuFrame::OnKeyDown(wxKeyEvent& event)
{
	std::cout << "gui KeyDown\n";
	int keycode = event.GetKeyCode();
	this->keyDown(keycode);
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

void WabbitemuFrame::OnKeyUp(wxKeyEvent& event)
{
	int key = event.GetKeyCode();
	this->keyUp(key);
}

void WabbitemuFrame::OnLeftButtonDown(wxMouseEvent& event)
{
	event.Skip(true);
	static wxPoint pt;
	keypad_t *kp = lpCalc->cpu.pio.keypad;

	//CopySkinToButtons();
	//CaptureMouse();
	pt.x	= event.GetX();
	pt.y	= event.GetY();
	/*if (lpCalc->bCutout) {
		pt.y += GetSystemMetrics(SM_CYCAPTION);
		pt.x += GetSystemMetrics(SM_CXSIZEFRAME);
	}*/
	for(int group = 0; group < 7; group++) {
		for(int bit = 0; bit < 8; bit++) {
			kp->keys[group][bit] &= (~KEY_MOUSEPRESS);
		}
	}

	lpCalc->cpu.pio.keypad->on_pressed &= ~KEY_MOUSEPRESS;

	/*if (!event.LeftDown()) {
		//FinalizeButtons(lpCalc);
		return;
	}*/

	if (lpCalc->keymap.GetRed(pt.x, pt.y) == 0xFF) {
		//FinalizeButtons(lpCalc);
		return;
	}

	int green = lpCalc->keymap.GetGreen(pt.x, pt.y);
	int blue = lpCalc->keymap.GetBlue(pt.x, pt.y);
	if ((green >> 4) == 0x05 && (blue >> 4) == 0x00)
	{
		lpCalc->cpu.pio.keypad->on_pressed |= KEY_MOUSEPRESS;
	} else {
		kp->keys[green >> 4][blue >> 4] |= KEY_MOUSEPRESS;
		if ((kp->keys[green >> 4][blue >> 4] & KEY_STATEDOWN) == 0) {
			//DrawButtonState(lpCalc, lpCalc->hdcButtons, lpCalc->hdcKeymap, &pt, DBS_DOWN | DBS_PRESS);
			kp->keys[green >> 4][blue >> 4] |= KEY_STATEDOWN;
		}
	}
}

void WabbitemuFrame::OnLeftButtonUp(wxMouseEvent& event)
{
	int group, bit;
	event.Skip(true);
	static wxPoint pt;
	bool repostMessage = FALSE;
	keypad_t *kp = lpCalc->cpu.pio.keypad;

#define KEY_TIMER 1
	//KillTimer(hwnd, KEY_TIMER);

	for (group = 0; group < 7; group++) {
		for (bit = 0; bit < 8; bit++) {
#define MIN_KEY_DELAY 400
			if (kp->last_pressed[group][bit] - lpCalc->cpu.timer_c->tstates >= MIN_KEY_DELAY || !lpCalc->running) {
				kp->keys[group][bit] &= (~KEY_MOUSEPRESS);
			} else {
				repostMessage = TRUE;
			}
		}
	}

	if (kp->on_last_pressed - lpCalc->cpu.timer_c->tstates >= MIN_KEY_DELAY || !lpCalc->running) {
		lpCalc->cpu.pio.keypad->on_pressed &= ~KEY_MOUSEPRESS;
	} else {
		repostMessage = TRUE;
	}

	if (repostMessage) {
		//SetTimer(hwnd, KEY_TIMER, 50, NULL);
	}

	FinalizeButtons();
}


void WabbitemuFrame::OnQuit(wxCloseEvent& event)
{

	lpCalc->active = FALSE;
	event.Skip();
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
