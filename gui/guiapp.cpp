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

#define MAX_KEYPRESSES 10
int numKeypresses = 0;
int inkeyPressIdx = 0;
int outkeyPressIdx = 0;
js_key keyQueue[MAX_KEYPRESSES];

#define ROM_FILE "z.rom"

int charToInt(char c) {
	return c - 48;
}

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
			printf("Keydown: %d: [%c]\n", e.key.keysym.sym, e.key.keysym.sym);
      keyDown(e.key.keysym.sym);
		}
		if( e.type == SDL_KEYUP ) {
			keyUp(e.key.keysym.sym);
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

void WabbitemuApp::keyDown(int group, int bit) {
		if ((theCalc->cpu.pio.keypad->keys[group][bit] & KEY_STATEDOWN) == 0) {
			theCalc->cpu.pio.keypad->keys[group][bit] |= KEY_STATEDOWN;
			FinalizeButtons();
		}
}

void WabbitemuApp::keyDown(int keycode)
{
	keyprog_t *kp = keypad_key_press(&theCalc->cpu, keycode);
	if (kp) {
		keyDown(kp->group, kp->bit);
	}
}

void WabbitemuApp::keyUp(int group, int bit)
{
	keypad_release(&theCalc->cpu, group, bit);
	FinalizeButtons();
}

void WabbitemuApp::keyUp(int keycode)
{
	keyprog_t *kp = keypad_key_press(&theCalc->cpu, keycode);
	if (kp) {
		keyUp(kp->group, kp->bit);
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

WabbitemuApp app;

EM_BOOL loop(double time, void* userData) {
  app.tick();
  //puts("Looping\n");
  // Return true to keep the loop running.
  return EM_TRUE;
}

void voidLoop() {
	app.tick();
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

int main(int argc, char * argv[]){
	printf("Pre app\n");
	if (!app.init()) {
		puts("Init failed\n");
		return 1;
	}
	printf("Post init\n");
	//emscripten_set_main_loop(voidLoop, 0, false);
	emscripten_request_animation_frame_loop(loop, 0);
	//test();
	
	
	//while(true){
    //    SDL_Delay(10);  // setting some Delay
	//	app.tick();
	//}
}


