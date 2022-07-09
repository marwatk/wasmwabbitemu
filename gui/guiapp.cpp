#include "guiapp.h"
#include "gui.h"
#include "sendfile.h"

unsigned char redColors[MAX_SHADES+1];
unsigned char greenColors[MAX_SHADES+1];
unsigned char blueColors[MAX_SHADES+1];

int frameNum = 0;
LPCALC theCalc;
SDL_Renderer *renderer = NULL;      // Pointer for the renderer
SDL_Window *window = NULL;      // Pointer for the window

IMPLEMENT_APP(WabbitemuApp)

#define ROM_FILE L"z.rom"

bool WabbitemuApp::init() {
	LPCALC lpCalc = calc_slot_new();
	
	int result = rom_load(lpCalc, lpCalc->rom_path);
	if (result == TRUE) {
	} else {
		calc_slot_free(lpCalc);
		BOOL loadedRom = FALSE;
		
		if (rom_load(lpCalc, ROM_FILE)) {
			loadedRom = TRUE;
		}
		if (!loadedRom) {
			return FALSE;
		}
	}
	lpCalc->running = TRUE;
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

	theCalc = lpCalc;
	return TRUE;
}

bool WabbitemuApp::OnInit()
{
	return init();
}

int WabbitemuApp::exit() {
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

int WabbitemuApp::OnExit() {
	return exit();
}


unsigned WabbitemuApp::GetTickCount()
{
		struct timeval tv;
		if(gettimeofday(&tv, NULL) != 0)
			return 0;

		return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

void WabbitemuApp::OnTimer(wxTimerEvent& event) {
	tick();
}



void WabbitemuApp::tick() {
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

void LoadToLPCALC(INT_PTR lParam, LPTSTR filePath, SEND_FLAG sendLoc)
{
	LPCALC lpCalc = (LPCALC) lParam;
	SendFile(lpCalc, filePath, sendLoc);
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
