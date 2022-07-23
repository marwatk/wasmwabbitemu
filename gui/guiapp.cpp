#include <iostream>
#include "guiapp.h"
#include "gui.h"
#include "sendfile.h"

#include <emscripten.h>
#include <emscripten/html5.h>

unsigned char redColors[MAX_SHADES+1];
unsigned char greenColors[MAX_SHADES+1];
unsigned char blueColors[MAX_SHADES+1];

int frameNum = 0;
LPCALC theCalc;
SDL_Renderer *renderer = NULL;      // Pointer for the renderer
SDL_Window *window = NULL;      // Pointer for the window

#define LCD_HIGH	255

#define MAX_PATH_LEN 2048

#define MAX_KEYPRESSES 10
int numKeypresses = 0;
int inkeyPressIdx = 0;
int outkeyPressIdx = 0;
js_key keyQueue[MAX_KEYPRESSES];

// This probably isn't unicode friendly?
char romPath[MAX_PATH_LEN];
bool initDone = false;

bool fetchCalcInput(js_key *key) {
	uint input = EM_ASM_INT(return fetchCalcInput());
	if (input == 0) {
		return false;
	}
	uint bit = input & 0xFF;
	uint group = (input & 0xFF00) >> 8;
	bool up = ((input & 0x10000) >> 16) != 0;
	printf("Input: %d, Bit: %d, Group: %d, Up: %d\n", input, bit, group, up);

	key->bit = bit;
	key->group = group;
	key->up = up;
	return true;
}

void jsKey(bool up, int group, int bit) {
	// I don't _think_ we need locking, but may need to investigate.
	if (numKeypresses >= MAX_KEYPRESSES) {
		puts("Discarding keys over threshhold\n");
		return;
	}

	keyQueue[inkeyPressIdx].up = up;
	keyQueue[inkeyPressIdx].group = group;
	keyQueue[inkeyPressIdx].bit = bit;

	inkeyPressIdx++;
	if (inkeyPressIdx >= MAX_KEYPRESSES) {
		inkeyPressIdx = 0;
	}
	numKeypresses++;
}

bool consumeJsKey(js_key *key) {
	if (numKeypresses <= 0) {
		return false;
	}
	key->up = keyQueue[outkeyPressIdx].up;
	key->group = keyQueue[outkeyPressIdx].group;
	key->bit = keyQueue[outkeyPressIdx].bit;
	outkeyPressIdx++;
	if (outkeyPressIdx >= MAX_KEYPRESSES) {
		outkeyPressIdx = 0;
	}
	numKeypresses--;
	return true;
}

bool WabbitemuApp::init() {
	LPCALC lpCalc = calc_slot_new();
	
	int result = rom_load(lpCalc, lpCalc->rom_path);
	if (result == TRUE) {
		puts("First load worked\n");
	} else {
		calc_slot_free(lpCalc);
		BOOL loadedRom = FALSE;
		
		if (rom_load(lpCalc, romPath)) {
			puts("Second load worked\n");
			loadedRom = TRUE;
		}
		if (!loadedRom) {
			puts("Load failed\n");
			return FALSE;
		}
	}
	lpCalc->running = TRUE;

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
    //std::cout << "Got SDL event\n";
		//std::cout << "WXK_UP: " << WXK_UP << "\n";
		//std::cout << "WXK_DOWN: " << WXK_DOWN << "\n";
		if( e.type == SDL_KEYDOWN ) {
			keyDown(e.key.keysym.sym);
		}
		if( e.type == SDL_KEYUP ) {
			keyUp(e.key.keysym.sym);
		}
  }

	js_key jsKey;
	if (fetchCalcInput(&jsKey)) {
		keypad_t *kp = theCalc->cpu.pio.keypad;
		int group = jsKey.group;
		int bit = jsKey.bit;
		if(jsKey.up) {
			kp->keys[group][bit] &= ~(KEY_MOUSEPRESS | KEY_STATEDOWN);
			if (group == 0x05 && bit == 0x00) {
				theCalc->cpu.pio.keypad->on_pressed &= ~KEY_MOUSEPRESS;
			}
		} else {
			if (group == 0x05 && bit == 0x00) {
				theCalc->cpu.pio.keypad->on_pressed |= KEY_MOUSEPRESS;
			} else {
				kp->keys[group][bit] |= KEY_MOUSEPRESS;
				if ((kp->keys[group][bit] & KEY_STATEDOWN) == 0) {
					kp->keys[group][bit] |= KEY_STATEDOWN;
				}
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
	keypad_key_release(&theCalc->cpu, key);
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

WabbitemuApp app;

EM_BOOL loop(double time, void* userData) {
  
  if (EM_ASM_INT( return loopJs(); ) == 0) {
    return EM_TRUE;
  }

  if (!initDone && romPath[0] != 0) {
		printf("Initializing with rom path %s\n", romPath);
		if (!app.init()) {
			printf("Init failed\n");
			romPath[0] = 0;
		}
		printf("Init succeeded\n");
    EM_ASM(calcLoaded());
		initDone = true;
	}
	if (initDone) {
		app.tick();
	}
  return EM_TRUE;
}

void voidLoop() {
	app.tick();
}

void testWrite() {
  FILE *file = fopen("/roms/test.txt", "wb");
  if (!file) {
    printf("cannot open file for writing\n");
    return;
  }
  fprintf(file, "Here's some file stuff\n");
	fclose(file);
}

void testRead() {
  FILE *file = fopen("/roms/test.txt", "rb");
  if (!file) {
    printf("cannot open file for reading\n");
    return;
  }
	while (!feof(file)) {
    char c = fgetc(file);
    if (c != EOF) {
      putchar(c);
    }
  }
  fclose (file);
}

EM_JS(void, sync, (), {
  sync();
});

int main(int argc, char * argv[]){
	romPath[0] = 0;

	EM_ASM(
		setRomPathPointer($0);
 		mainJs();
	, &romPath);

	//emscripten_set_main_loop(voidLoop, 0, false);
	emscripten_request_animation_frame_loop(loop, 0);
	//test();
}

