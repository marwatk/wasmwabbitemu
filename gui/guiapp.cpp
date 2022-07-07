#include "guiapp.h"
#include "gui.h"
#include "sendfile.h"

WabbitemuFrame *frames[MAX_CALCS];

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

	SDL_Init(SDL_INIT_VIDEO);       // Initializing SDL as Video
	SDL_CreateWindowAndRenderer(400, 400, 0, &window, &renderer);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);      // setting draw color
	SDL_RenderClear(renderer);      // Clear the newly created window
	SDL_RenderPresent(renderer);    // Reflects the changes done in the

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
				frames[0]->keyDown(WXK_UP);
				printf( "UP!\n" );
				break;

				case SDLK_DOWN:
				frames[0]->keyDown(WXK_DOWN);
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
				frames[0]->keyUp(WXK_UP);
				break;

				case SDLK_DOWN:
				frames[0]->keyUp(WXK_DOWN);
				printf( "DOWN!\n" );
				break;

				default:
				break;
			}
		}
    }
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