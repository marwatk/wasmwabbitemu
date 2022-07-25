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

#define MAX_PATH_LEN 2048
#define ROM_FILE "z.rom"

char romPath[MAX_PATH_LEN];
bool initDone = false;
int tickNum = 0;

bool mapKey(int keycode, js_key *key) {
	uint input = EM_ASM_INT(return mapKey($0), keycode);
	if (input == 0) {
		return false;
	}

	uint bit = input & 0xFF;
	uint group = (input & 0xFF00) >> 8;
	printf("Input: %d, Bit: %d, Group: %d\n", input, bit, group);

	key->bit = bit;
	key->group = group;
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
		
		if (rom_load(lpCalc, ROM_FILE)) {
			puts("Second load worked\n");
			loadedRom = TRUE;
		}
		if (!loadedRom) {
			puts("Load failed\n");
			return FALSE;
		}
	}
	lpCalc->running = TRUE;

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

void WabbitemuApp::handleEvents() {
	js_key jsKey;
	SDL_Event e;
	while (SDL_PollEvent(&e)) {  // poll until all events are handled!
    if( e.type == SDL_KEYDOWN || e.type == SDL_KEYUP ) {
      printf("Tick %d: Key event: %d: [%c]\n", tickNum, e.key.keysym.sym, e.key.keysym.sym);
      if( !mapKey(e.key.keysym.sym, &jsKey) ) {
        continue;
      }
      if( e.type == SDL_KEYDOWN ) {
        printf("Keydown: %d: [%c], %d,%d\n",
          e.key.keysym.sym, e.key.keysym.sym,
          jsKey.group, jsKey.bit);
        keyDown(jsKey.group, jsKey.bit);
      }
      if( e.type == SDL_KEYUP ) {
        printf("Keyup: %d: [%c], %d,%d\n",
          e.key.keysym.sym, e.key.keysym.sym,
          jsKey.group, jsKey.bit);
        keyUp(jsKey.group, jsKey.bit);
      }
			// Only allow one key event per cpu tick
			return;
    }
  }
	if ( EM_ASM_INT(return calcInputs.length) > 0 ) {
		int input = EM_ASM_INT(return calcInputs.shift());
		int bit = input & 0xFF;
		int group = (input & 0xFF00) >> 8;
		bool up = (input & 0x10000) != 0;
		if ( up ) {
        keyUp(group, bit);
		}
		else {
			keyDown(group, bit);
		}
	}	
}

void WabbitemuApp::tick() {
	tickNum++;
//	if (tickNum % 100 == 0) {
//		printf("Tick %d\n", tickNum);
//	}
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
	handleEvents();

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

void WabbitemuApp::keyDown(int group, int bit) {
		keypad_press(&theCalc->cpu, group, bit);
		if ((theCalc->cpu.pio.keypad->keys[group][bit] & KEY_STATEDOWN) == 0) {
			theCalc->cpu.pio.keypad->keys[group][bit] |= KEY_STATEDOWN;
			FinalizeButtons();
		}
}

void WabbitemuApp::keyDown(int keycode)
{
	keyprog_t *key = keypad_map_key(&theCalc->cpu, keycode);
	if ( key != NULL ) {
		keyDown(key->group, key->bit);
	}
}

void WabbitemuApp::keyUp(int group, int bit)
{
	keypad_release(&theCalc->cpu, group, bit);
	FinalizeButtons();
}

void WabbitemuApp::keyUp(int keycode)
{
	keyprog_t *key = keypad_map_key(&theCalc->cpu, keycode);
	if ( key != NULL ) {
		keyUp(key->group, key->bit);
	}
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

void test() {
  FILE *file = fopen("test.txt", "rb");
  if (!file) {
    printf("cannot open file\n");
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

int main(int argc, char * argv[]){
	romPath[0] = 0;
	//strcpy(romPath, ROM_FILE);
	EM_ASM(
		setRomPathPointer($0);
		mainJs();
	, &romPath);

	emscripten_request_animation_frame_loop(loop, 0);
}
