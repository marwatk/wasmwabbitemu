#include "guilcd.h"
#include "gui.h"
#include "core.h"

BEGIN_EVENT_TABLE(WabbitemuLCD, wxWindow)
	EVT_PAINT(WabbitemuLCD::OnPaint)
	EVT_KEY_DOWN(WabbitemuLCD::OnKeyDown)
	EVT_KEY_UP(WabbitemuLCD::OnKeyUp)
	
	EVT_LEFT_DOWN(WabbitemuLCD::OnLeftButtonDown)
	EVT_LEFT_UP(WabbitemuLCD::OnLeftButtonUp)
END_EVENT_TABLE()

WabbitemuLCD::WabbitemuLCD(wxFrame *mainFrame, LPCALC lpCalc)
	: wxWindow(mainFrame, ID_LCD, wxPoint(0,0), lpCalc->LCDRect.GetSize()) {
	this->lpCalc = lpCalc;
	this->mainFrame = mainFrame;
#define LCD_HIGH	255
	for (int i = 0; i <= MAX_SHADES; i++) {
		redColors[i] = (0x9E*(256-(LCD_HIGH/MAX_SHADES)*i))/255;
		greenColors[i] = (0xAB*(256-(LCD_HIGH/MAX_SHADES)*i))/255;
		blueColors[i] = (0x88*(256-(LCD_HIGH/MAX_SHADES)*i))/255;
	}
	hasDrawnLCD = false;

}

void WabbitemuLCD::OnLeftButtonDown(wxMouseEvent& event)
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

void WabbitemuLCD::OnLeftButtonUp(wxMouseEvent& event)
{
	event.Skip(true);
	static wxPoint pt;
	keypad_t *kp = lpCalc->cpu.pio.keypad;

	//ReleaseMouse();

	for(int group = 0; group < 7; group++) {
		for(int bit = 0; bit < 8; bit++) {
			kp->keys[group][bit] &= ~(KEY_MOUSEPRESS | KEY_STATEDOWN);
		}
	}

	lpCalc->cpu.pio.keypad->on_pressed &= ~KEY_MOUSEPRESS;
}

void WabbitemuLCD::OnResize(wxSizeEvent& event)
{
	event.Skip(false);
}

//TODO: forward these events somehow
void WabbitemuLCD::OnKeyDown(wxKeyEvent& event)
{
	std::cout << "lcd KeyDown\n";
	int keycode = event.GetKeyCode();
	if (keycode == WXK_F8) {
		if (lpCalc->speed == 100)
			lpCalc->speed = 400;
		else
			lpCalc->speed = 100;
	}
	if (keycode == WXK_SHIFT) {
		wxUint32 raw = event.GetRawKeyCode();
		if (raw == 65505) {
			keycode = WXK_LSHIFT;
		} else {
			keycode = WXK_RSHIFT;
		}
	}

	keyprog_t *kp = keypad_key_press(&lpCalc->cpu, keycode);
	if (kp) {
		if ((lpCalc->cpu.pio.keypad->keys[kp->group][kp->bit] & KEY_STATEDOWN) == 0) {
			lpCalc->cpu.pio.keypad->keys[kp->group][kp->bit] |= KEY_STATEDOWN;
			this->Update();
			FinalizeButtons();
		}
	}
}

void WabbitemuLCD::OnKeyUp(wxKeyEvent& event)
{
	int key = event.GetKeyCode();
	if (key == WXK_SHIFT) {
		keypad_key_release(&lpCalc->cpu, WXK_LSHIFT);
		keypad_key_release(&lpCalc->cpu, WXK_RSHIFT);
	} else {
		keypad_key_release(&lpCalc->cpu, key);
	}
	FinalizeButtons();
}

void WabbitemuLCD::FinalizeButtons() {
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

void WabbitemuLCD::OnPaint(wxPaintEvent& event)
{
	if (this->mainFrame->IsIconized()) {
		return;
	}
	wxPaintDC dc(this);
	PaintLCD(this, &dc);
} 

void WabbitemuLCD::PaintLCD(wxWindow *window, wxPaintDC *wxDCDest)
{
	unsigned char *screen;
	LCD_t *lcd = lpCalc->cpu.pio.lcd;
	wxSize rc = lpCalc->LCDRect.GetSize();
	int scale = lpCalc->scale;
	int draw_width = lpCalc->LCDRect.GetWidth();
	int draw_height = lpCalc->LCDRect.GetHeight();
	wxPoint drawPoint(0, 0);
	wxMemoryDC wxMemDC;
	if (lcd->active == false) {
		unsigned char lcd_data[128*64];
		memset(lcd_data, 0, sizeof(lcd_data));
		unsigned char rgb_data[128*64*3];
		int i, j;
		for (i = j = 0; i < 128*64; i++, j+=3) {
			rgb_data[j] = redColors[lcd_data[i]];
			rgb_data[j+1] = greenColors[lcd_data[i]];
			rgb_data[j+2] = blueColors[lcd_data[i]];
		}
		wxImage screenImage(128, 64, rgb_data, true);
		wxBitmap bmpBuf(screenImage.Size(wxSize(lcd->width, 64), wxPoint(0, 0)).Scale(rc.GetWidth(), rc.GetHeight()));
		wxMemDC.SelectObject(bmpBuf);

		wxDCDest->Blit(drawPoint.x, drawPoint.y, draw_width, draw_height, &wxMemDC, 0, 0);
		wxMemDC.SelectObject(wxNullBitmap);

	} else {
		screen = LCD_image( lpCalc->cpu.pio.lcd ) ;
		unsigned char rgb_data[128*64*3];
		int i, j;
		for (i = j = 0; i < 128*64; i++, j+=3) {
			/*
			if ( i % 128 == 0 ) {
				std::cout << "\n";
			}
			if (screen[i] > 0) {
				std::cout << "*";
			}
			else {
				std::cout << " ";
			}
			*/
			rgb_data[j] = redColors[screen[i]];
			rgb_data[j+1] = greenColors[screen[i]];
			rgb_data[j+2] = blueColors[screen[i]];
		}
		
		wxImage screenImage(128, 64, rgb_data, true);
		wxBitmap bmpBuf(screenImage.Size(wxSize(lcd->width, 64), wxPoint(0, 0)).Scale(rc.GetWidth(), rc.GetHeight()));
		wxMemDC.SelectObject(bmpBuf);
		wxDCDest->Blit(drawPoint.x, drawPoint.y, draw_width, draw_height, &wxMemDC, 0, 0);
		//lets give it a texture to look nice
		wxImage skinTexture = lpCalc->calcSkin.GetSubImage(lpCalc->LCDRect);
		int textureSize = lpCalc->LCDRect.GetWidth() * lpCalc->LCDRect.GetHeight();
		unsigned char *alpha = (unsigned char *) malloc(textureSize);
		memset(alpha, 108, textureSize);
		skinTexture.SetAlpha(alpha);
		wxDCDest->DrawBitmap(skinTexture, drawPoint.x, drawPoint.y, true);
		wxMemDC.SelectObject(wxNullBitmap);

	}

}