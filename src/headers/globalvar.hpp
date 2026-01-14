#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftlcdfil.h>
#include <functional>
#include <stdint.h>

struct ToolbarButtonItem {
	int indexX = 0;
	std::string name;
	bool isSeperator;
};

struct MenuItem {
	std::string name; // has seperator in it
	std::function<bool()> func;
	int atlasX;
	int atlasY;
	bool* enable_condition;
	bool close_menu;
};

struct UndoDataStruct {
	uint32_t imageID;
	uint32_t imageIDoriginal;
	int width;
	int height;
};

#define REAL_BIG_VERSION "2.6"

struct GlobalParams {
	std::string name_primary = "ViewImage";
	std::string name_full = "View Image";

	std::vector<ToolbarButtonItem> toolbartable;

	// deltatime
	float ms_time = 0;

	// the global window
	HWND hwnd;
	HDC hdc;

	// memory
	void* imgdata;
	void* imgoriginaldata;
	void* scrdata;
	void* toolbar_gaussian_data;

	void* imagepreview;
	bool isImagePreview = false;

	// images
	unsigned char* toolbarData;
	unsigned char* im;
	unsigned char* fullscreenIconData;
	unsigned char* dmguideIconData;
	unsigned char* cropImageData;
	unsigned char* menu_icon_atlas;
	unsigned char* menu_shadow;
	
	std::vector<UndoDataStruct> undoData;
	int undoStep = 0;
	int ProcessOfMakingUndoStep = 0;
	std::string undofolder;

	// strings
	std::string fpath;
	std::string renderfpath;
	std::string cd;

	// size integers
	int width = 1000;
	int height = 563;
	int imgwidth;
	int imgheight;

	int widthos; // for toolbar size
	int heightos;
	int channelos;

	// menu shadow size
	int menu_s_x;
	int menu_s_y;

	int dumpchannel; // UNUSED
	int menu_atlas_SizeX;
	int menu_atlas_SizeY;

	// settings
	const int iconSize = 30; // the real width is 31 because 1 px divider in memory
	int toolheight = 43;
	int starttoolbarloc = 3;
	// current state

	bool shouldSaveShutdown = false;

	bool loading = false;
	bool deletingtemporaryfiles = false;

	// left right logic
	bool halt = false;

	bool mouseDown = false;
	bool rightdown = false;
	bool toolmouseDown = false;
	bool drawmousedown = false;
	int drawtype = 1; // 1 for draw: 0 for erase: 3 for transparent

	int selectedbutton = -1;

	float iLocX = 0; // X position
	float iLocY = 0; // Y Position
	float mscaler = 1.0f; // Global zoom

	int CoordLeft = 0;
	int CoordTop = 0;
	int CoordRight = 0;
	int CoordBottom = 0;
	
	// mouse stuff
		bool lock = true;
		int lockimgoffx;
		int lockimgoffy;
		POINT LockmPos;
		bool isSize;
		//int lastMouseX;
		//int lastMouseY; // for drawing only
		// not used anymore due to WASD magic
		int lastK;
		int lastV; // use these instead
		


	bool fullscreen = false;
	WINDOWPLACEMENT wpPrev;

	// iterators
	std::vector<uint32_t> ith, itv;


	bool isMenuState = false;

	int menuY = toolheight+5;
	int menuX = 0;

	int actmenuY = 0;
	int actmenuX = 0;

	int menuSX = 0;
	int menuSY = 0;
	int mH = 25;

	// draw menu
	int drawMenuOffsetX = 0;
	int drawMenuOffsetY = 0;

	bool IsLastMouseDownWhenOverMenu = false;

	bool smoothing = true;

	std::string fontsfolder;

	std::vector<MenuItem> menuVector;

	// drawing/annotating

	
	bool drawmode = false;

	bool eyedroppermode = false;

	bool a_softmode = false;
	uint32_t a_drawColor = 0xFFCC0000;
	float drawSize = 20;
	float a_opacity = 1.0f;
	float a_resolution = 30.0f;

	// slider's

	int slider1begin = 113;
	int slider1end = 214;

	int slider2begin = 318;
	int slider2end = 405;

	bool slider1mousedown = false;
	bool slider2mousedown = false;
	bool slider3mousedown = false;
	bool slider4mousedown = false;

	float testfloat = 0.0f;

	bool debugmode = false;
	bool draw_updates_debug = false;

	bool sleepmode = false;

	int mouseRightInitialCheckX;
	int mouseRightInitialCheckY;

	bool tint = false;

	// preloaded fonts

	FT_Face SegoeUI;
	FT_Face Verdana;
	FT_Face OCRAExt;

	// crop mode
	bool isInCropMode = false;
	float leftP = 0.0f;
	float rightP = 1.0f;
	float topP = 0.0f;
	float bottomP = 1.0f;
	bool CropHandleSelectTL = false;
	bool CropHandleSelectTR = false;
	bool CropHandleSelectBL = false;
	bool CropHandleSelectBR = false;

	bool isMovingTL = false;
	bool isMovingTR = false;
	bool isMovingBL = false;
	bool isMovingBR = false;

	// For WASD Protection ONLY
	float wasdX = 0;
	float wasdY = 0;
	bool SetLastMouseForWASDInputCaptureProtectionLock = false;


	bool isJoystick = false;
	JOYINFOEX joyInfoEx;
	UINT joystickID = JOYSTICKID1;

	HWND drawtext_access_dialog_hwnd;
	int locationXtextvar = 0;
	int locationYtextvar = 0;

	int drawtext_guiLocX = 0;
	int drawtext_guiLocY = 0;
	int drawtext_ghostmode = false;

	int sizetextvar = 32;

	double def_txt_shadow_softness = 0.34f;

	std::string current_directory;

	int dmguide_x = 3;
	int dmguide_y = 45;
	int dmguide_sx = 45;
	int dmguide_sy = 168;

	bool lcd = true;

	bool item_enabled = true; // enable true placeholder
	bool item_disabled = false; // enable false placeholder

	bool undo_menucondition = false;
	bool redo_menucondition = false;
	bool isimage_menucondition = false;	
		
	LARGE_INTEGER previousTime;
	LARGE_INTEGER frequency; // for wasd magic

	bool full_redraw_surface_the_first_time = true; // fully redraw the surface the first time mouse enters the toolbar to clear any changes present inside the image AKA annotation circle

};	




// mod list
// drawing (does not change original)
// resizing (does change original)
// rotating (does change original)
// effects (does not change original)
	// brightness contrast
	// gaussian
	// text
	// invert
// crop (does change original)