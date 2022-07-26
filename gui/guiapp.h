#include "stdafx.h"
#include "gui.h"
#include "calc.h"

#include <SDL.h>

typedef struct js_key {
	bool up;
	int group;
	int bit;
} js_key;

void jsKey(bool up, int group, int bit);

#define MAX_FILES 255
typedef struct ParsedCmdArgs
{
	LPTSTR rom_files[MAX_FILES];
	LPTSTR utility_files[MAX_FILES];
	LPTSTR archive_files[MAX_FILES];
	LPTSTR ram_files[MAX_FILES];
	int num_rom_files;
	int num_utility_files;
	int num_archive_files;
	int num_ram_files;
	BOOL silent_mode;
	BOOL force_new_instance;
	BOOL force_focus;
} ParsedCmdArgs_t;

class WabbitemuApp
{
public:
 	void keyDown(int keycode);
	void keyUp(int keycode);
 	void keyDown(int group, int bit);
	void keyUp(int group, int bit);
	void FinalizeButtons();
	
	bool init();
	int exit();
	void tick();
	void handleEvents();

	virtual bool OnInit();
	virtual int OnExit();
	void getTimer(int slot);
	void render();
	unsigned GetTickCount();
};

void LoadToLPCALC(INT_PTR lParam, LPTSTR filePath, SEND_FLAG sendLoc);