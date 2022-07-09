#include "guiapp.h"
#include "gui.h"
#include "sendfile.h"

WabbitemuFrame *frames[MAX_CALCS];

unsigned char redColors[MAX_SHADES+1];
unsigned char greenColors[MAX_SHADES+1];
unsigned char blueColors[MAX_SHADES+1];

int frameNum = 0;
LPCALC theCalc;
SDL_Renderer *renderer = NULL;      // Pointer for the renderer
SDL_Window *window = NULL;      // Pointer for the window

IMPLEMENT_APP(WabbitemuApp)

void WabbitemuApp::LoadSettings(LPCALC lpCalc)
{
	settingsConfig = new wxConfig(wxT("Wabbitemu"));
	wxString tempString;
	settingsConfig->Read(wxT("/rom_path"), &tempString, wxEmptyString);
	_tcscpy(lpCalc->rom_path, tempString.c_str());
	settingsConfig->Read(wxT("/SkinEnabled"), &lpCalc->SkinEnabled, FALSE);
}

bool WabbitemuApp::OnInit()
{
	wxImage::AddHandler(new wxPNGHandler);
	//stolen from the windows version
	ParseCommandLineArgs();

	memset(frames, 0, sizeof(frames));
	LPCALC lpCalc = calc_slot_new();
	LoadSettings(lpCalc);
	
	WabbitemuFrame *frame;
	int result = rom_load(lpCalc, lpCalc->rom_path);
	if (result == TRUE) {
		frame = gui_frame(lpCalc);
	} else {
		calc_slot_free(lpCalc);
		BOOL loadedRom = FALSE;
		if (parsedArgs.num_rom_files > 0) {
			for (int i = 0; i < parsedArgs.num_rom_files; i++) {
				if (rom_load(lpCalc, parsedArgs.rom_files[i])) {
					gui_frame(lpCalc);
					loadedRom = TRUE;
					break;
				}
			}
		}
		if (!loadedRom) {
			return FALSE;
		}
	}
	LoadCommandlineFiles((INT_PTR) lpCalc, LoadToLPCALC);
	timer = new wxTimer();
	timer->Connect(wxEVT_TIMER, (wxObjectEventFunction) &WabbitemuApp::OnTimer);
	timer->Start(TPF, false);

#define LCD_HIGH	255
	for (int i = 0; i <= MAX_SHADES; i++) {
		redColors[i] = (0x9E*(256-(LCD_HIGH/MAX_SHADES)*i))/255;
		greenColors[i] = (0xAB*(256-(LCD_HIGH/MAX_SHADES)*i))/255;
		blueColors[i] = (0x88*(256-(LCD_HIGH/MAX_SHADES)*i))/255;
	}

	SDL_Init(SDL_INIT_VIDEO);       // Initializing SDL as Video
	SDL_CreateWindowAndRenderer(128, 64, 0, &window, &renderer);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);      // setting draw color
	SDL_RenderClear(renderer);      // Clear the newly created window
	SDL_RenderPresent(renderer);    // Reflects the changes done in the

	/*
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	for( int x = 0; x < 128; x++ ) {
		for ( int y = 0; y < 64; y++ ) {
			SDL_RenderDrawPoint(renderer, x, y);
		}
	}
	SDL_RenderPresent(renderer);
	*/
	theCalc = lpCalc;
	return TRUE;
}

int WabbitemuApp::OnExit() {
	//load ROMs first
	for (int i = 0; i < parsedArgs.num_rom_files; i++) {
		free(parsedArgs.rom_files[i]);
		parsedArgs.rom_files[i] = NULL;
	}
	//then archived files
	for (int i = 0; i < parsedArgs.num_archive_files; i++) {
		free(parsedArgs.archive_files[i]);
		parsedArgs.archive_files[i] = NULL;
	}
	//then ram
	for (int i = 0; i < parsedArgs.num_ram_files; i++) {
		free(parsedArgs.ram_files[i]);
		parsedArgs.ram_files[i] = NULL;
	}
	//finally utility files (label, break, etc)
	for (int i = 0; i < parsedArgs.num_utility_files; i++) {
		free(parsedArgs.utility_files[i]);
		parsedArgs.utility_files[i] = NULL;
	}
	return 0;
}


unsigned WabbitemuApp::GetTickCount()
{
		struct timeval tv;
		if(gettimeofday(&tv, NULL) != 0)
			return 0;

		return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}


void WabbitemuApp::OnTimer(wxTimerEvent& event) {
	static int difference;
	static unsigned prevTimer;
	unsigned dwTimer = GetTickCount();
	
	// How different the timer is from where it should be
	// guard from erroneous timer calls with an upper bound
	// that's the limit of time it will take before the
	// calc gives up and claims it lost time
	difference += ((dwTimer - prevTimer) & 0x003F) - TPF;
	prevTimer = dwTimer;

	// Are we greater than Ticks Per Frame that would call for
	// a frame skip?
	if (difference > -TPF) {
		calc_run_all();
		while (difference >= TPF) {
			calc_run_all();
			difference -= TPF;
		}

		int i;
		for (i = 0; i < MAX_CALCS; i++) {
			if (calcs[i].active) {
				frames[i]->gui_draw();
				this->render();
			}
		}
	// Frame skip if we're too far ahead.
	} else {
		difference += TPF;
	}

	SDL_Event e;
    while (SDL_PollEvent(&e)) {  // poll until all events are handled!
        std::cout << "Got SDL event\n";
		if( e.type == SDL_KEYDOWN ) {
			switch( e.key.keysym.sym )
			{
				case SDLK_UP:
				keyDown(WXK_UP);
				printf( "UP!\n" );
				break;

				case SDLK_DOWN:
				keyDown(WXK_DOWN);
				printf( "DOWN!\n" );
				break;

				default:
				break;
			}
		}
		if( e.type == SDL_KEYUP ) {
			switch( e.key.keysym.sym )
			{
				case SDLK_UP:
				keyUp(WXK_UP);
				break;

				case SDLK_DOWN:
				keyUp(WXK_DOWN);
				printf( "DOWN!\n" );
				break;

				default:
				break;
			}
		}
    }
}

void WabbitemuApp::render() {
	//std::cout << "R Direct: " << static_cast<void*>(theCalc) << std::endl;
	//std::cout << "Renderer: " << static_cast<void*>(renderer) << "\n";
	if (theCalc == NULL) {
		return;
	}
	LCD_t *lcd = theCalc->cpu.pio.lcd;
	unsigned char *screen;
	screen = LCD_image( lcd ) ;

	unsigned char rgb_data[128*64*3];
	int i, j;
	int y = -1;
	int x = 0;
	for (i = j = 0; i < 128*64; i++, j+=3, x++) {
		if ( i % 128 == 0 ) {
			y++;
			x = 0;
		}
		int r = redColors[screen[i]];
		int g = greenColors[screen[i]];
		int b = blueColors[screen[i]];

		SDL_SetRenderDrawColor(renderer, r, g, b, 255);
		//std::cout << "(" << x << "," << y << ")[" << r << "," << g << "," << b << std::endl;
		SDL_RenderDrawPoint(renderer, x, y);
	}
	SDL_RenderPresent(renderer);
}

void WabbitemuApp::ParseCommandLineArgs()
{
	ZeroMemory(&parsedArgs, sizeof(ParsedCmdArgs));
	TCHAR tmpstring[512];
	SEND_FLAG ram = SEND_CUR;

	if (argv && argc > 1) {
		_tcscpy(tmpstring, argv[1]);
		for(int i = 1; i < argc; i++) {
			ZeroMemory(tmpstring, 512);
			_tcscpy(tmpstring, argv[i]);
			TCHAR secondChar = toupper(tmpstring[1]);
			if (*tmpstring != '-' && *tmpstring != '/') {
				TCHAR *temp = (TCHAR *) malloc(_tcslen(tmpstring) + 1);
				_tcscpy(temp, tmpstring);
				temp[_tcslen(tmpstring) + 1] = '\0';
				TCHAR extension[5] = _T("");
				const TCHAR *pext = _tcsrchr(tmpstring, _T('.'));
				if (pext != NULL) {
					_tcscpy(extension, pext);
				}
				if (!_tcsicmp(extension, _T(".rom")) || !_tcsicmp(extension, _T(".sav")) || !_tcsicmp(extension, _T(".clc"))) {
					parsedArgs.rom_files[parsedArgs.num_rom_files++] = temp;
				}
				else if (!_tcsicmp(extension, _T(".brk")) || !_tcsicmp(extension, _T(".lab")) 
					|| !_tcsicmp(extension, _T(".zip")) || !_tcsicmp(extension, _T(".tig"))) {
						parsedArgs.utility_files[parsedArgs.num_utility_files++] = temp;
				}
				else if (ram) {
					parsedArgs.ram_files[parsedArgs.num_ram_files++] = temp;
				} else {
					parsedArgs.archive_files[parsedArgs.num_archive_files++] = temp;
				}
			} else if (secondChar == 'R') {
				ram = SEND_RAM;
			} else if (secondChar == 'A') {
				ram = SEND_ARC;
			} else if (secondChar == 'S') {
				parsedArgs.silent_mode = TRUE;
			} else if (secondChar == 'F') {
				parsedArgs.force_focus = TRUE;
			} else if (secondChar == 'N') {
				parsedArgs.force_new_instance = TRUE;
			}
		}
	}
}

void LoadToLPCALC(INT_PTR lParam, LPTSTR filePath, SEND_FLAG sendLoc)
{
	LPCALC lpCalc = (LPCALC) lParam;
	SendFile(lpCalc, filePath, sendLoc);
}

void WabbitemuApp::LoadCommandlineFiles(INT_PTR lParam,  void (*load_callback)(INT_PTR, LPTSTR, SEND_FLAG))
{
	//load ROMs first
	for (int i = 0; i < parsedArgs.num_rom_files; i++) {
		load_callback(lParam, parsedArgs.rom_files[i], SEND_ARC);
	}
	//then archived files
	for (int i = 0; i < parsedArgs.num_archive_files; i++) {
		load_callback(lParam, parsedArgs.archive_files[i], SEND_ARC);
	}
	//then ram
	for (int i = 0; i < parsedArgs.num_ram_files; i++) {
		load_callback(lParam, parsedArgs.ram_files[i], SEND_RAM);
	}
	//finally utility files (label, break, etc)
	for (int i = 0; i < parsedArgs.num_utility_files; i++) {
		load_callback(lParam, parsedArgs.utility_files[i], SEND_ARC);
	}
}

void WabbitemuApp::keyDown(int keycode)
{
	keyprog_t *kp = keypad_key_press(&theCalc->cpu, keycode);
	if (kp) {
		if ((theCalc->cpu.pio.keypad->keys[kp->group][kp->bit] & KEY_STATEDOWN) == 0) {
			theCalc->cpu.pio.keypad->keys[kp->group][kp->bit] |= KEY_STATEDOWN;
			FinalizeButtons();
		}
	}
}

void WabbitemuApp::keyUp(int key)
{
	if (key == WXK_SHIFT) {
		keypad_key_release(&theCalc->cpu, WXK_LSHIFT);
		keypad_key_release(&theCalc->cpu, WXK_RSHIFT);
	} else {
		keypad_key_release(&theCalc->cpu, key);
	}
	FinalizeButtons();
}

void WabbitemuApp::FinalizeButtons() {
	int group, bit;
	keypad_t *kp = theCalc->cpu.pio.keypad;
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
