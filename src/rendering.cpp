
#include "headers/rendering.hpp"
#include <vector>
#include <thread>
#include <time.h>
#include <shlwapi.h>
#include <wincodec.h>



#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

// Freetype Globals
FT_Library ft;
FT_Face* currentFace;


#define CanRenderToolbarMacro (((!m->fullscreen && m->height >= 250) || p.y < m->toolheight)&&!m->isInCropMode)

void CircleGenerator(int circleDiameter, int locX, int locY, uint32_t color, uint32_t* buffer, unsigned int width, unsigned int height) {
	float radius = circleDiameter/2.0f;
	float step = 1.0f/circleDiameter;
	for(float i=0; i<2.0f*3.141592f; i+= step) {
		float locationX = radius*cos(i)+locX;
		float locationY = radius*sin(i)+locY;

		int ptX = (int)locationX;
		int ptY = (int)locationY;

		if(ptX > 0 && ptX <= width && ptY > 0 && ptY <= height) {
			*GetMemoryLocation(buffer, ptX, ptY, width, height) = color;
		}
	}
}

std::string GetWindowsFontsFolder() {
	HKEY hKey;
	char fontPath[MAX_PATH];
	DWORD size = MAX_PATH;

	// Open the registry key where the fonts directory path is stored
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		// Query the "FontDir" value
		if (RegQueryValueEx(hKey, "FontDir", nullptr, nullptr, (LPBYTE)fontPath, &size) == ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return std::string(fontPath);
		}
		RegCloseKey(hKey);
	}

	// Fallback: Use the standard Windows directory + "Fonts" if registry lookup fails
	if (GetWindowsDirectory(fontPath, MAX_PATH)) {
		PathAppend(fontPath, "Fonts");
		return std::string(fontPath);
	}

	// If everything fails, return an empty string
	return "Fail";
}

void initfontfolder(GlobalParams* m) {
	m->fontsfolder = GetWindowsFontsFolder();
	if (m->fontsfolder == "Fail") {
		MessageBox(m->hwnd, "Fonts directory not found\n", "Error", MB_OK | MB_ICONERROR);
	}
}

std::string ExePath() {
    char buffer[MAX_PATH] = { 0 };
    GetModuleFileName( NULL, buffer, MAX_PATH );
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos);
}

bool alreadyInit = false;
FT_Face LoadFont(GlobalParams* m, std::string fontA) {
	if (!alreadyInit) {
		if (FT_Init_FreeType(&ft)) {
			MessageBox(m->hwnd, "Failed to initialize FreeType library\n", "Error", MB_OK | MB_ICONERROR);
			return 0;
		}
		FT_Library_SetLcdFilter(ft, FT_LCD_FILTER_MAX);
		alreadyInit = true;
	}
	
	if (m->fontsfolder == "") {
		initfontfolder(m);
	}
	std::string font_s = (m->fontsfolder + "\\" + fontA);
	FT_Face k;
	if (!(FT_New_Face(ft, font_s.c_str(), 0, &k))) {
		return k;
	}

	// try again
	std::string cdir = ExePath();
	font_s = (cdir + "\\" + fontA);
	if (!(FT_New_Face(ft, font_s.c_str(), 0, &k))) {
		return k;
	}

	std::string er = "Failed to load font: " + fontA + "\nYou may need to install this on your system";
	MessageBox(m->hwnd, er.c_str(), "Error Loading Font", MB_OK | MB_ICONERROR);
	return 0;
}


void SwitchFont(FT_Face& font) {
	currentFace = &font;
}


int PlaceStringGreyscale(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, int bufwidth, int bufheight, void* fromBuffer) {
	
	locY = bufheight - locY-size; // compensate for negative Y

	if (!*currentFace) {
		return 0;
	}

	FT_Set_Pixel_Sizes(*currentFace, 0, size);

    const FT_GlyphSlot glyph = (*currentFace)->glyph;
	
	for (const char* p = inputstr; *p; p++) {
		
		if (FT_Load_Char((*currentFace), *p, FT_LOAD_RENDER) != 0)
			continue;
		
		// width of char: glyph->bitmap.width
		// height of char: glyph->bitmap.rows

        const float vx = locX + glyph->bitmap_left * 1;
        const float vy = locY + glyph->bitmap_top * 1;
        const float w = glyph->bitmap.width * 1;
        const float h = glyph->bitmap.rows * 1;

		// render
		for (int y = 0; y < glyph->bitmap.rows; ++y) {
			for (int x = 0; x < glyph->bitmap.width; ++x) {
				unsigned char pixelValue = glyph->bitmap.buffer[y * glyph->bitmap.pitch + x];
				
				uint32_t ptx = vx + x;
				uint32_t pty = vy-y;

				pty = bufheight - pty; // compensate for negative Y

				uint32_t* memoryPath = GetMemoryLocation(mem, ptx, pty, bufwidth, bufheight);
				uint32_t* memoryPathFrom = GetMemoryLocation(fromBuffer, ptx, pty, bufwidth, bufheight);
				uint32_t existingColor = *memoryPathFrom;
				if (ptx >= 0 && pty >= 0 && ptx < bufwidth && pty < bufheight) {
					*GetMemoryLocation(mem, ptx, pty, bufwidth, bufheight) = lerp(existingColor, color, ((float)pixelValue / 255.0f));
				}
			}
		}

        locX += (glyph->advance.x >> 6) * 1;
        locY += (glyph->advance.y >> 6) * 1;
		
	}
	return 1;
}

int PlaceStringLCD(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, int bufwidth, int bufheight, void* fromBuffer) {
	
	locY = bufheight - locY - size; // compensate for negative Y

	if (!*currentFace) {
		return false;
	}

	FT_Set_Pixel_Sizes(*currentFace, 0, size);
	const FT_GlyphSlot glyph = (*currentFace)->glyph;

	for (const char* p = inputstr; *p; p++) {
		
		if (FT_Load_Char((*currentFace), *p, FT_LOAD_RENDER | FT_LOAD_TARGET_LCD) != 0)
			continue;

		const int vx = locX + glyph->bitmap_left;
		const int vy = locY + glyph->bitmap_top;

		for (int y = 0; y < glyph->bitmap.rows; y++) {
			for (int x = 0; x < glyph->bitmap.width / 3; x++) {

				int i = y * glyph->bitmap.pitch + x * 3;

				float cr = glyph->bitmap.buffer[i + 0] / 255.0f;
				float cg = glyph->bitmap.buffer[i + 1] / 255.0f;
				float cb = glyph->bitmap.buffer[i + 2] / 255.0f;

				int ptx = vx + x;
				int pty = bufheight - (vy - y);

				if (ptx < 0 || pty < 0 || ptx >= bufwidth || pty >= bufheight)
					continue;

				uint32_t* dstPtr  = GetMemoryLocation(mem,        ptx, pty, bufwidth, bufheight);
				uint32_t* srcPtr  = GetMemoryLocation(fromBuffer, ptx, pty, bufwidth, bufheight);
				uint32_t dstColor = *srcPtr;

				uint8_t dstR = (dstColor >> 16) & 0xFF;
				uint8_t dstG = (dstColor >> 8)  & 0xFF;
				uint8_t dstB =  dstColor        & 0xFF;
				uint8_t dstA = (dstColor >> 24) & 0xFF;

				uint8_t srcR = (color >> 16) & 0xFF;
				uint8_t srcG = (color >> 8)  & 0xFF;
				uint8_t srcB =  color        & 0xFF;
				uint8_t srcA = (color >> 24) & 0xFF;

				uint8_t outR = (uint8_t)(srcR * cr + dstR * (1.0f - cr));
				uint8_t outG = (uint8_t)(srcG * cg + dstG * (1.0f - cg));
				uint8_t outB = (uint8_t)(srcB * cb + dstB * (1.0f - cb));

				float cov = cr;
				if (cg > cov) cov = cg;
				if (cb > cov) cov = cb;

				uint8_t outA = dstA + (uint8_t)((255 - dstA) * cov * (srcA / 255.0f));

				*dstPtr = (outA << 24) | (outR << 16) | (outG << 8) | outB;
			}
		}

		locX += (glyph->advance.x >> 6);
		locY += (glyph->advance.y >> 6);
	}

	return true;
}


int PlaceStringBuffer(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, int bufwidth, int bufheight, void* fromBuffer) {
	bool state = true;
	if (m->lcd) {
		state = state && PlaceStringLCD(m, size, inputstr, locX, locY, color, mem, bufwidth, bufheight, fromBuffer);
	} else {
		state = state && PlaceStringGreyscale(m, size, inputstr, locX, locY, color, mem, bufwidth, bufheight, fromBuffer);
	}

	return state;
}



int PlaceStringShadowObject(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, double sigma, int passes) {
	int clearance = 10;
	unsigned int approx_textwidth = std::string(inputstr).length()*size;

	unsigned int tempbuffer_width = approx_textwidth+clearance*2;
	unsigned int tempbuffer_height = size+clearance*2;


	size_t memcount = tempbuffer_width*tempbuffer_height;
	size_t memsize = memcount*4;

	uint32_t* temp_buffer = (uint32_t*)malloc(memsize);
	memset(temp_buffer, 0xFF, memsize);

	int e = PlaceStringGreyscale(m, size, inputstr, clearance, clearance, 0x000000, temp_buffer, tempbuffer_width, tempbuffer_height, temp_buffer);

	// blur
	gaussian_blur_B(temp_buffer, temp_buffer, tempbuffer_width, tempbuffer_height, sigma, tempbuffer_width, tempbuffer_height, 0, 0);

	for(int i=0; i<passes; i++) {
		for(int y=0; y<tempbuffer_height; y++) {
			for(int x=0; x<tempbuffer_width; x++) {
				// render temp
				uint32_t putX = locX+x-clearance;
				uint32_t putY = locY+y-clearance;
				if(putX > 0 && putX <= m->width && putY > 0 && putY <= m->height) {
					uint32_t* scrbuf = GetMemoryLocation(mem, putX, putY, m->width, m->height);
					int from = (*GetMemoryLocation(temp_buffer, x, y, tempbuffer_width, tempbuffer_height) >> 8) & 0xFF;
					float factor = 1.0f-((float)from / 255.0f);
					*scrbuf = lerp(*scrbuf, color, factor);
				}
			}
		}
	}

	free(temp_buffer);

	return e;
}

int PlaceString(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem) {
	bool e = PlaceStringBuffer(m, size, inputstr, locX, locY, color, mem, m->width, m->height, mem);
	return e;
}

int PlaceString(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, int bufwidth, int bufheight, void* fromBuffer) {
	bool e = PlaceStringBuffer(m, size, inputstr, locX, locY, color, mem, bufwidth, bufheight, fromBuffer);
	return e;
}

int PlaceStringLegacyShadow(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, int shadowY, int shadowX = 0) {
	bool b = PlaceStringBuffer(m, size, inputstr, locX+shadowX, locY + shadowY, 0x000000, mem, m->width, m->height, mem);
	bool e = PlaceStringBuffer(m, size, inputstr, locX, locY, color, mem, m->width, m->height, mem);

	return e && b;
}

int PlaceStringShadow(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, float sigma, int shadowOffsetX, int shadowOffsetY, int passes, uint32_t shadowColor = 0x000000) {
	bool b = PlaceStringShadowObject(m, size, inputstr, locX+shadowOffsetX, locY+shadowOffsetY, shadowColor, mem, sigma, passes);
	bool e = PlaceStringBuffer(m, size, inputstr, locX, locY, color, mem, m->width, m->height, mem);

	return e && b;
}





void dDrawRectangle(void* mem, int kwidth, int kheight, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {

			if (x < 1 || x > width - 2 || y < 1 || y > height - 2) {
				int realX = xloc + x;
				int realY = yloc + y;

				if (realX >= 0 && realY >= 0 && realX < kwidth && realY < kheight) {
					uint32_t* ma = GetMemoryLocation(mem, realX, realY, kwidth, kheight);
					if (opacity < 0.99f) {
						*ma = lerp(*ma, color, opacity);
					}
					else {
						*ma = color;
					}
				}
			}
		}
	}
}

void dDrawRoundedFilledRectangle(void* mem, int kwidth, int kheight, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int realX = xloc + x;
			int realY = yloc + y;

			if (realX >= 0 && realY >= 0 && realX < kwidth && realY < kheight) {
				uint32_t* ma = GetMemoryLocation(mem, realX, realY, kwidth, kheight);
				//if (x < 1 || x > width - 2 || y < 1 || y > height - 2) {
				if (!(x == 0 && y == 0) && !(x == width - 1 && y == height - 1) && !(x == width - 1 && y == 0) && !(x == 0 && y == height - 1)) {
					if (opacity < 0.99f) {
						*ma = lerp(*ma, color, opacity);
					}
					else {
						*ma = color;
					}
				}
			}
			//}
		}
	}
}

void dDrawFilledRectangle(void* mem, int kwidth, int kheight, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if ((xloc + x) < 0 || (xloc + x) >= kwidth || (yloc + y) < 0 || (yloc + y) >= kheight) {
				continue;
			}
			uint32_t* ma = GetMemoryLocation(mem, xloc + x, yloc + y, kwidth, kheight);
			if (opacity < 0.99f) {
				*ma = lerp(*ma, color, opacity);
			}
			else {
				*ma = color;
			}
		}
	}
}

 
void dDrawRoundedRectangle(void* mem, int kwidth, int kheight, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if ((xloc + x) < 0 || (xloc + x) >= kwidth || (yloc + y) < 0 || (yloc + y) >= kheight) {
				continue;
			}
			uint32_t* ma = GetMemoryLocation(mem, xloc + x, yloc + y, kwidth, kheight);
			if (x < 1 || x > width - 2 || y < 1 || y > height - 2) {
				if (!(x == 0 && y == 0) && !(x == width - 1 && y == height - 1)&& !(x == width - 1 && y == 0) && !(x == 0 && y == height - 1)) {
					if (opacity < 0.99f) {
						*ma = lerp(*ma, color, opacity);
					}
					else {
						*ma = color;
					}
				}
			}
		}
	}
}

// MULTITHREADING!!!!!
// https://www.youtube.com/watch?v=46ddlUImiQA
//*********************************************


void PlaceImageNN(GlobalParams* m, void* memory, bool invert, POINT p) {

	const float inv_mscaler = 1.0f / m->mscaler;
	const int32_t imgwidth = m->imgwidth;
	const int32_t imgheight = m->imgheight;
	const int margin = 2;

	std::for_each(std::execution::par, m->itv.begin(), m->itv.end(), [&](uint32_t y) {
		for (uint32_t x : m->ith) {

			uint32_t bkc = 0x151515;
			// bkc
			if (y > m->toolheight || !(CanRenderToolbarMacro)) {

				if (((x / 9) + (y / 9)) % 2 == 0) {
					bkc = 0x181818;
				}
				else {
					bkc = 0x121212;
				}
			}

			int32_t offX = (((int32_t)m->width - (int32_t)(imgwidth * m->mscaler)) / 2) + m->iLocX;
			int32_t offY = (((int32_t)m->height - (int32_t)(imgheight * m->mscaler)) / 2) + m->iLocY;

			int32_t ptx = (x - offX) * inv_mscaler;
			int32_t pty = (y - offY) * inv_mscaler;

			uint32_t doColor = bkc;
			if (ptx < imgwidth && pty < imgheight && ptx >= 0 && pty >= 0 &&
				x >= margin && y >= margin && y < m->height - margin && x < m->width - margin) { // check if image is within it's coordinates
				uint32_t c = *GetMemoryLocation(memory, ptx, pty, imgwidth, imgheight);

				int alpha = (c >> 24) & 255;
				if (alpha == 255) {
					doColor = c;
				}
				else if (alpha == 0) {
					// still bkc
				}
				else {
					doColor = lerp(bkc, c, static_cast<float>(alpha) / 255.0f);
				}
			}
			*GetMemoryLocation(m->scrdata, x, y, m->width, m->height) = doColor;
		}
		});
}

bool followsPattern(int number) {
	double logBase2 = log2(number / 100.0);
	return logBase2 == floor(logBase2);
}

void PlaceImageBI(GlobalParams* m, void* memory, bool invert, POINT p) {


	//auto start = std::chrono::high_resolution_clock::now();

	if (followsPattern(m->mscaler * 100)) {
		PlaceImageNN(m, memory, invert, p);
		return;
	}

	const int margin = m->fullscreen ? 0 : 2;
	const float inv_mscaler = 1.0f / m->mscaler;
	const int32_t imgwidth_minus_1 = m->imgwidth - 1;
	const int32_t imgheight_minus_1 = m->imgheight - 1;

	std::for_each(std::execution::par, m->itv.begin(), m->itv.end(), [&](uint32_t y) {
		for (uint32_t x : m->ith) {

			uint32_t bkc = 0x151515;
			// bkc
			if (y > m->toolheight || !(CanRenderToolbarMacro)) {

				if (((x / 9) + (y / 9)) % 2 == 0) {
					bkc = 0x181818;
				}
				else {
					bkc = 0x121212;
				}
			}

			float offX = ((float)m->width  - ((float)m->imgwidth  * m->mscaler)) / 2.0f + m->iLocX;
			float offY = ((float)m->height - ((float)m->imgheight * m->mscaler)) / 2.0f + m->iLocY;

			float ptx = ((float)x - offX) * inv_mscaler - 0.5f;
			float pty = ((float)y - offY) * inv_mscaler - 0.5f;

			uint32_t doColor = bkc;

			if (ptx < imgwidth_minus_1 && pty < imgheight_minus_1 && ptx >= 0 && pty >= 0 && x >= margin && y >= margin && y < m->height - margin && x < m->width - margin) {
				int32_t ptx_int = static_cast<int32_t>(ptx);
				int32_t pty_int = static_cast<int32_t>(pty);
				float ptx_frac = ptx - ptx_int;
				float pty_frac = pty - pty_int;

				// Get the four nearest pixels
				
				uint32_t c00 = *GetMemoryLocation(memory, ptx_int, pty_int, m->imgwidth, m->imgheight);
				uint32_t c01 = *GetMemoryLocation(memory, ptx_int + 1, pty_int, m->imgwidth, m->imgheight);
				uint32_t c10 = *GetMemoryLocation(memory, ptx_int, pty_int + 1, m->imgwidth, m->imgheight);
				uint32_t c11 = *GetMemoryLocation(memory, ptx_int + 1, pty_int + 1, m->imgwidth, m->imgheight);
				
				if (((c00 >> 24) & 0xFF) != 0 || ((c01 >> 24) & 0xFF) != 0 || ((c10 >> 24) & 0xFF) != 0 || ((c11 >> 24) & 0xFF) != 0) {
					uint32_t placeColor = lerp(lerp(c00, c01, ptx_frac), lerp(c10, c11, ptx_frac), pty_frac);

					int alpha = (placeColor >> 24) & 255;
					if (alpha == 255) {
						doColor = placeColor;
					}
					else if (alpha == 0) {
						// still bkc
					}
					else {
						doColor = lerp(bkc, placeColor, static_cast<float>(alpha) / 255.0f);
					}
				}
			}
			*GetMemoryLocation(m->scrdata, x, y, m->width, m->height) = doColor;
		}
	});
}


void RenderToolbarIcon(GlobalParams* m, int index, int locationX, void* data, uint32_t color, uint32_t selectedColor) {
	ToolbarButtonItem* item = &m->toolbartable[index];
	for (uint32_t y = 0; y < m->iconSize; y++) {
		for (uint32_t x = 0; x < m->iconSize; x++) {

			int locationOnBitmap = item->indexX;

			uint32_t l = (*GetMemoryLocation(data, x+locationOnBitmap, y, m->widthos, m->heightos));
			float alphaM = (float)((l >> 24) & 0xFF) / 255.0f;
			float valueM = (float)((l >> 16) & 0xFF) / 255.0f;
			uint32_t* memoryPath = GetMemoryLocation(m->scrdata, locationX + x, 6 + y, m->width, m->height);

			uint32_t targetColor = color;
			if (index == m->selectedbutton) {
				targetColor = selectedColor;
			}

			*memoryPath = lerp(*memoryPath, multiplyColor(targetColor, valueM), alphaM);
		}
	}
	if (item->isSeperator) {
		int location = locationX + (m->iconSize)+4;
		for (int y = 0; y < m->toolheight-25; y++) {
			uint32_t* memoryPath = GetMemoryLocation(m->scrdata, location, y + 12, m->width, m->height);
			*memoryPath = lerp(*memoryPath, 0xFFFFFF, 0.3f);
		}
	}
}

void RenderToolbarButtons(GlobalParams* m, void* data, uint32_t color, uint32_t selectedColor) {
	// GetLocationFromButton, RenderToolbarButtons, getXbuttonID
	// Make sure they are all synced

	int p = m->starttoolbarloc;
	for (size_t i = 0; i < m->toolbartable.size(); i++) {
		RenderToolbarIcon(m, i, p+2, data, color, selectedColor);
		//p += m->iconSize + 5 + (m->toolbartable[i].isSeperator * 4);
		p += GetIndividualButtonPush(m, i);
		if (m->imgwidth < 1) {
			return;
		}
		if (m->drawmode && i == 11) {
			return;
		}
	}
}

void RenderToolbar(GlobalParams* m) {
		
		
		// Render the toolbar
		// 
		//BLUR FOR TOOLBAR
		
		if (m->CoordTop <= m->toolheight) {
			blur_toolbar(m, (uint32_t*)m->scrdata);
		}

		// TOOLBAR CONTAINER
		for (uint32_t y = 0; y < m->toolheight; y++) {
			for (uint32_t x = 0; x < m->width; x++) {
				uint32_t color = 0x0C0C0C;
				float opacity = 0.65f;
				//if (y == m->toolheight - 2) { color = 0xFFFFFF; opacity = 0.15f; }
				//if (y == m->toolheight - 1) { color = 0x000000; opacity = 1.0f; }

				if (y == 0) {color = 0x000000; opacity=1.0f;}
				//if (y == 1) color = 0x000000;
				uint32_t* memoryPath = GetMemoryLocation(m->scrdata, x, y, m->width, m->height);
				*memoryPath = lerp(*memoryPath, color, opacity); // transparency
			}
		}

		for (uint32_t x = 0; x < m->width; x++) { 
			uint32_t y = m->toolheight-2; uint32_t color = 0xFFFFFF; float opacity = 0.2f;
			uint32_t* memoryPath = GetMemoryLocation(m->scrdata, x, y, m->width, m->height);
			*memoryPath = lerp(*memoryPath, color, opacity); // transparency
		}
		for (uint32_t x = 0; x < m->width; x++) { 
			uint32_t y = m->toolheight-1; uint32_t color = 0x000000; float opacity = 1.0f;
			uint32_t* memoryPath = GetMemoryLocation(m->scrdata, x, y, m->width, m->height);
			*memoryPath = lerp(*memoryPath, color, opacity); // transparency
		}
		for (uint32_t x = 0; x < m->width; x++) { 
			uint32_t y = 0; uint32_t color = 0xFFFFFF; float opacity = 0.2f;
			uint32_t* memoryPath = GetMemoryLocation(m->scrdata, x, y, m->width, m->height);
			*memoryPath = lerp(*memoryPath, color, opacity); // transparency
		}


		
		

		// BUTTONS 
		if (!m->toolbarData) return;

		// render toolbar buttons shadow

		//int mybutton = m->maxButtons;
		//if (m->imgwidth < 1) mybutton = 1;

		//RenderToolbarButtonShadow(m, m->toolbarData_shadow, mybutton, 0x606060, 0.7f, 0x606060, 0.7f);
		//RenderToolbarButtonShadow(m, m->toolbarData_shadow, mybutton, 0x000000, 0.7f, 0, 0.7f);


		// version
		SwitchFont(m->OCRAExt);
		if (m->width > 540) {
			PlaceString(m, 13, REAL_BIG_VERSION, m->width - 71, 15, 0xB0B0B0, m->scrdata);
		}
		else {
			PlaceString(m, 13, REAL_BIG_VERSION, m->width - 71, m->toolheight + 4, 0xB0B0B0, m->scrdata);
		}

		// tooltips AND OUTLINE FOR THE BUTTONS (basically stuff when its selected)
		
		
		//-- The border when selecting annotate
		if (m->drawmode) {
			// 7 means draw
			dDrawRoundedRectangle(m->scrdata, m->width, m->height, GetLocationFromButton(m, 7), 4, m->iconSize+4, m->toolheight - 8, 0xFFFFFF, 0.2f);
			dDrawRoundedRectangle(m->scrdata, m->width, m->height, GetLocationFromButton(m, 7)-1, 3, m->iconSize+4 + 2, m->toolheight - 6, 0x000000, 1.0f);
		}
		
		RenderToolbarButtons(m, m->toolbarData, 0xFFFFFF, 0xFFE0E0);

		if (m->selectedbutton >= 0 && m->selectedbutton < m->toolbartable.size()) {
			// rounded corners: split hover thing into three things
			int in = m->selectedbutton;

			dDrawRoundedFilledRectangle(m->scrdata, m->width, m->height, GetLocationFromButton(m, in), 4, m->iconSize+4, m->toolheight - 8, 0xFF8080, 0.3f);
			//-- The fill when selecting the buttons
			
			//-- The outline wb border when selecting the buttons
			dDrawRoundedRectangle(m->scrdata, m->width, m->height, GetLocationFromButton(m, in), 4, m->iconSize+4, m->toolheight - 8, 0xFFFFFF, 0.3f);
			dDrawRoundedRectangle(m->scrdata, m->width, m->height, GetLocationFromButton(m, in)-1, 3, m->iconSize+4 +2, m->toolheight - 6, 0x000000, 1.0f);
			

			

			std::string txt = "Error";
			if (m->selectedbutton < m->toolbartable.size()) {
				txt = m->toolbartable[m->selectedbutton].name;
			}

			// tooltips
			if (!m->isMenuState) {
				int loc = 1 + (GetLocationFromButton(m, in));
				// draw guide
				if(in == 7 && m->drawmode) {
					int off = -20;
					uint32_t shiftx = loc+2;
					SwitchFont(m->Verdana);

					// fun sampling
					
					uint32_t color1 = *GetMemoryLocation(m->scrdata, 276, 95, m->width, m->height);
					uint32_t color2 = *GetMemoryLocation(m->scrdata, 379, 116, m->width, m->height);
					uint32_t color3 = *GetMemoryLocation(m->scrdata, 287, 148, m->width, m->height);
					int lum1 = (((color1>>16)&0xFF) + ((color1>>8)&0xFF) + (color1&0xFF))/3;
					int lum2 = (((color2>>16)&0xFF) + ((color2>>8)&0xFF) + (color2&0xFF))/3;
					int lum3 = (((color3>>16)&0xFF) + ((color3>>8)&0xFF) + (color3&0xFF))/3;

					int lum = (lum1+lum2+lum3)/3;

					float greyscale = (-2.0f/255.0f)*abs(lum-127)+1;

					uint32_t dcolor = lerp(0x808080, 0xFFFFFF, greyscale);
					PlaceString(m, 12, "Annotate Shortcuts", shiftx, m->toolheight + 28+off, dcolor, m->scrdata);
					SwitchFont(m->OCRAExt);

					PlaceString(m, 10, "Draw        Left Mouse", shiftx, m->toolheight + 43+off, dcolor, m->scrdata);
					PlaceString(m, 10, "Pan         Middle Mouse", shiftx, m->toolheight + 53+off, dcolor, m->scrdata);
					PlaceString(m, 10, "Erase       Ctrl", shiftx, m->toolheight + 63+off, dcolor, m->scrdata);
					PlaceString(m, 10, "Transparent Ctrl+Shift", shiftx, m->toolheight + 73+off, dcolor, m->scrdata);
					PlaceString(m, 10, "Eyedropper  Shift+Z", shiftx, m->toolheight + 83+off, dcolor, m->scrdata);
				} else {
					// actual tooltips
					gaussian_blur((uint32_t*)m->scrdata, (txt.length() * 8) + 12, 20, 4.0f, m->width, m->height, loc-1, m->toolheight+4);
					
					dDrawFilledRectangle(m->scrdata, m->width, m->height, loc-1, m->toolheight + 4, (txt.length() * 8) + 12, 20, 0x000000, 0.4f);
					dDrawRoundedRectangle(m->scrdata, m->width, m->height, loc - 1, m->toolheight + 4, (txt.length() * 8) + 12, 20, 0xFFFFFF, 0.3f);
					dDrawRoundedRectangle(m->scrdata, m->width, m->height, loc - 2, m->toolheight + 3, (txt.length() * 8) + 14, 22, 0x000000, 0.8f);

					SwitchFont(m->OCRAExt);
					PlaceStringShadow(m, 14, txt.c_str(), loc + 3, m->toolheight + 4, 0xFFFFFF, m->scrdata, 0.34f, 1, 1, 1);
				}


				
			}
			
		}
		

		// fullscreen icon
		if (m->width > 535) {
			POINT mp;
			GetCursorPos(&mp);
			ScreenToClient(m->hwnd, &mp);
			bool nearf = false;
			if ((mp.x > m->width - 36 && mp.x < m->width - 13) && (mp.y > 12 && mp.y < 33)) { //fullscreen icon location check coordinates (ALWAYS KEEP)
				nearf = true;
			}

			if(nearf) {
				//-- The outline for the fullscreen button
				dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->width-36, 12, 24, 23, 0xFFFFFF, 0.2f);
				dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->width-37, 11, 26, 25, 0x000000, 1.0f);

				SwitchFont(m->SegoeUI);
				//-- Fullscreen icon tooltip
				if (m->fullscreen) {

					PlaceStringShadow(m, 13, "Exit Fullscreen (F11)", m->width - 120, 48, 0xFFFFFF, m->scrdata, m->def_txt_shadow_softness, 1,1, 2);
				}
				else {

					PlaceStringShadow(m, 13, "Fullscreen (F11)", m->width - 100, 48, 0xFFFFFF, m->scrdata, m->def_txt_shadow_softness, 1,1, 2);
				}

			}
			for (int y = 0; y < 11; y++) {
				for (int x = 0; x < 12; x++) {
					uint32_t* oLoc = GetMemoryLocation(m->scrdata, m->width + x - 30, y + 18, m->width, m->height);
					float t = 0.5f;
					if (nearf) {
						t = 1.0f;
					}
					float transparency = ((float)((*GetMemoryLocation(m->fullscreenIconData, x, y, 12, 11)) & 0xFF) / 255.0f) * t;
					*oLoc = lerp(*oLoc, 0xFFFFFF, transparency);
				}
			}
		}
		

}

void RenderToolbarShadow(GlobalParams *m) {
	// drop shadow for the toolbar (not buttons)

	for (uint32_t y = 0; y < 20; y++) {
		for (uint32_t x = 0; x < m->width; x++) {
			uint32_t color = 0x000000;

			uint32_t* memoryPath = GetMemoryLocation(m->scrdata, x, y + m->toolheight, m->width, m->height);
			*memoryPath = lerp(*memoryPath, color, (1.0f - ((float)y / 20.0f)) * 0.3f); // transparency
		}
	}
}

void ResetCoordinates(GlobalParams* m) {

	if (m->imgwidth > 0) {

		m->CoordLeft = (m->width / 2) - (-m->iLocX) - (int)((((float)m->imgwidth / 2.0f)) * m->mscaler);
		m->CoordTop = (m->height / 2) - (-m->iLocY) - (int)((((float)m->imgheight / 2.0f)) * m->mscaler);

		m->CoordRight = (m->width / 2) - (-m->iLocX) + (int)((((float)m->imgwidth / 2.0f)) * m->mscaler);
		m->CoordBottom = (m->height / 2) - (-m->iLocY) + (int)((((float)m->imgheight / 2.0f)) * m->mscaler);

	}
	else {

		m->CoordLeft = 0;
		m->CoordTop = 0;

		m->CoordRight = 0;
		m->CoordBottom = 0;
	}
}

void DrawMenuIcon(GlobalParams* m, int locationX, int locationY, int atlasX, int atlasY, float opacity) {
	// 12x12
	int iconSize = 12;
	for (int y = 0; y < iconSize; y++) {
		for (int x = 0; x < iconSize; x++) {
			uint32_t* inAtlas = GetMemoryLocation(m->menu_icon_atlas, atlasX + x, atlasY + y, m->menu_atlas_SizeX, m->menu_atlas_SizeY);
			uint32_t* inMem = GetMemoryLocation(m->scrdata, locationX + x, locationY + y, m->width, m->height);
			float alphaM = ((float)((*inAtlas >> 24) & 0xFF) / 255.0f)*opacity;
			//float valueM = (float)((*inAtlas >> 16) & 0xFF) / 255.0f;
			*inMem = lerp(*inMem, 0xFF6060, alphaM);
		}
	}
}

void DrawMenu_Shadow(GlobalParams* m, uint32_t posx, uint32_t posy, uint32_t sizex, uint32_t sizey){
	uint32_t sx = sizex-2;
	uint32_t sy = sizey-2;
	uint32_t px = posx-17;
	uint32_t py = posy-17;
	// Top left
	for(int y=0; y<18; y++) {
		for(int x=0; x<18; x++) {
			uint32_t* memloc = GetMemoryLocation(m->scrdata, x+px, y+py, m->width, m->height);
			if(x+px >= m->width) {continue;}
			uint32_t color = *GetMemoryLocation(m->menu_shadow, x, y, m->menu_s_x, m->menu_s_y);
			*memloc = lerp(0x000000, *memloc, ((float)((color >> 16)&0xFF))/255.0f);
		}
	}

	// Top Right
	for(int y=0; y<18; y++) {
		for(int x=0; x<18; x++) {
			uint32_t* memloc = GetMemoryLocation(m->scrdata, x+sx+18+px, y+py, m->width, m->height);
			if(x+sx+18+px >= m->width) {continue;}
			uint32_t color = *GetMemoryLocation(m->menu_shadow, x+64, y, m->menu_s_x, m->menu_s_y);
			*memloc = lerp(0x000000, *memloc, ((float)((color >> 16)&0xFF))/255.0f);
		}
	}

	// Top
	for(int y=0; y<18; y++) {
		for(int x=0; x<sx; x++) {
			uint32_t* memloc = GetMemoryLocation(m->scrdata, x+18+px, y+py, m->width, m->height);
			if(x+18+px >= m->width) {continue;}
			uint32_t color = *GetMemoryLocation(m->menu_shadow, ((x*46)/sx)+18, y, m->menu_s_x, m->menu_s_y);
			*memloc = lerp(0x000000, *memloc, ((float)((color >> 16)&0xFF))/255.0f);
		}
	}

	// Left
	for(int y=0; y<sy; y++) {
		for(int x=0; x<18; x++) {
			uint32_t* memloc = GetMemoryLocation(m->scrdata, x+px, y+18+py, m->width, m->height);
			if(x+px >= m->width) {continue;}
			uint32_t color = *GetMemoryLocation(m->menu_shadow, x, ((y*53)/sy)+18, m->menu_s_x, m->menu_s_y);
			*memloc = lerp(0x000000, *memloc, ((float)((color >> 16)&0xFF))/255.0f);
		}
	}

	// Bottom Left
	for(int y=0; y<18; y++) {
		for(int x=0; x<18; x++) {
			uint32_t* memloc = GetMemoryLocation(m->scrdata, x+px, y+sy+18+py, m->width, m->height);
			if(x+px >= m->width) {continue;}
			uint32_t color = *GetMemoryLocation(m->menu_shadow, x, y+71, m->menu_s_x, m->menu_s_y);
			*memloc = lerp(0x000000, *memloc, ((float)((color >> 16)&0xFF))/255.0f);
		}
	}

	// Bottom
	for(int y=0; y<18; y++) {
		for(int x=0; x<sx; x++) {
			uint32_t* memloc = GetMemoryLocation(m->scrdata, x+18+px, y+sy+18+py, m->width, m->height);
			if(x+18+px >= m->width) {continue;}
			uint32_t color = *GetMemoryLocation(m->menu_shadow, ((x*46)/sx)+18, y+71, m->menu_s_x, m->menu_s_y);
			*memloc = lerp(0x000000, *memloc, ((float)((color >> 16)&0xFF))/255.0f);
		}
	}

	// Bottom Right
	for(int y=0; y<18; y++) {
		for(int x=0; x<18; x++) {
			uint32_t* memloc = GetMemoryLocation(m->scrdata, x+sx+18+px, y+sy+18+py, m->width, m->height);
			if(x+sx+18+px >= m->width) {continue;}
			uint32_t color = *GetMemoryLocation(m->menu_shadow, x+64, y+71, m->menu_s_x, m->menu_s_y);
			*memloc = lerp(0x000000, *memloc, ((float)((color >> 16)&0xFF))/255.0f);
		}
	}

	// Right
	for(int y=0; y<sy; y++) {
		for(int x=0; x<18; x++) {
			uint32_t* memloc = GetMemoryLocation(m->scrdata, x+sx+18+px, y+18+py, m->width, m->height);
			if(x+sx+18+px >= m->width) {continue;}
			uint32_t color = *GetMemoryLocation(m->menu_shadow, x+64, ((y*53)/sy)+18, m->menu_s_x, m->menu_s_y);
			*memloc = lerp(0x000000, *memloc, ((float)((color >> 16)&0xFF))/255.0f);
		}
	}

}

// rendermenu, placemenu
void DrawMenu(GlobalParams* m) { // render menu draw menu

	
	int mH = m->mH;
	int miX = 175;
	int miY = (m->menuVector.size() * mH) + m->menuVector.size();

	m->menuSX = miX;
	m->menuSY = miY - 2; // FIX-Crash when on border

	int posX = (m->menuX > (m->width - m->menuSX)) ? (m->width - m->menuSX) : m->menuX;
	int posY = (m->menuY > (m->height - m->menuSY)) ? (m->height - m->menuSY) : m->menuY;

	m->actmenuX = posX;
	m->actmenuY = posY;
	
	DrawMenu_Shadow(m, posX, posY, m->menuSX, m->menuSY);

	gaussian_blur((uint32_t*)m->scrdata, miX-4, miY-4, 4.0f, m->width, m->height, posX+2, posY+2);
	dDrawRoundedFilledRectangle(m->scrdata, m->width, m->height, posX, posY, miX, miY, 0x000000, 0.8f);
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, posX+1, posY+1, miX-2, miY-2, 0xFFFFFF, 0.2f);
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, posX, posY, miX, miY, 0x000000, 1.0f);

	POINT mp;
	GetCursorPos(&mp);
	ScreenToClient(m->hwnd, &mp);

	int selected = (mp.y-(posY+2))/mH;
	if (selected < m->menuVector.size() && IfInMenu(mp, m) && *(m->menuVector[selected].enable_condition) ) {
		int hoverLocX = posX + 4;
		int hoverLocY = posY + 4 + ((mH)*selected);
		int hoverSizeX = miX - 8;
		int hoverSizeY = mH - 3;
		// This is for hovering over your favorite menu button
		dDrawRoundedFilledRectangle(m->scrdata, m->width, m->height, hoverLocX, hoverLocY, hoverSizeX, hoverSizeY, 0xFF8080, 0.3f);   // FILL
		dDrawRoundedRectangle(m->scrdata, m->width, m->height, hoverLocX, hoverLocY, hoverSizeX, hoverSizeY, 0xFFFFFF, 0.3f);         // white
		dDrawRoundedRectangle(m->scrdata, m->width, m->height, hoverLocX - 1, hoverLocY - 1, hoverSizeX + 2, hoverSizeY + 2, 0x000000, 1.0f); // black BORDER!
	}
	

	SwitchFont(m->Verdana);
	for (int i = 0; i < m->menuVector.size(); i++) {
		bool enabled = *(m->menuVector[i].enable_condition);
		
		std::string mystr = m->menuVector[i].name;
		if (mystr.length() >= 3 && mystr.substr(mystr.length() - 3) == "{s}") {
			mystr = mystr.substr(0, mystr.length() - 3);
		}

		float opacity = 1.0f;
		uint32_t txtcolor = 0xF0F0F0;
		if(!enabled) {
			opacity = 0.5f;
			txtcolor = 0x808080;
		}

		DrawMenuIcon(m, posX + 10, (mH* i) + posY + 10, m->menuVector[i].atlasX, m->menuVector[i].atlasY, opacity);
												// changed when adding icon


		PlaceString(m, 12, mystr.c_str(), posX + 26, (mH * i) + posY + 9, txtcolor, m->scrdata);
		if (i == m->menuVector.size()-1) continue;

		if (strstr(m->menuVector[i].name.c_str(), "{s}")) { // seperator
			dDrawFilledRectangle(m->scrdata, m->width, m->height, posX + 8, posY + mH + (mH * i) + 2, miX - 16, 1, 0xFFFFFF, 0.25f);
		}
	}
}

void RenderBK(GlobalParams* m, const POINT& p) {
	for(int y=0; y<m->height; y++){
		for (int x = 0; x < m->width; x++) {
			// Don't use anymore due to transparency
				//if (x > (m->CoordRight-(m->mscaler/2)) || x < (m->CoordLeft+(m->mscaler / 2)) || y < (m->CoordTop+(m->mscaler / 2)) || y > (m->CoordBottom-(m->mscaler / 2))) {
					uint32_t bkc = 0x151515;
					// bkc
					if (y > m->toolheight || !(CanRenderToolbarMacro)) {

						if (((x / 9) + (y / 9)) % 2 == 0) {
							bkc = 0x181818;
						}
						else {
							bkc = 0x121212;
						}

					}


					*GetMemoryLocation(m->scrdata, x, y, m->width, m->height) = bkc;
				//}
			}
		}
	
}

void drawCircle(int x, int y, int radius, uint32_t* imageBuffer, int imageWidth) {
	// Ensure non-negative radius
	if (radius < 0) {
		return;
	}

	for (int i = 0; i <= 360; ++i) {
		double radians = i * 3.141592 / 180.0;

		int circleX = static_cast<int>(x + radius * std::cos(radians));
		int circleY = static_cast<int>(y + radius * std::sin(radians));

		// Check if the calculated coordinates are within the image boundaries
		if (circleX >= 0 && circleX < imageWidth && circleY >= 0) {
			// Assuming the image is a linear buffer with each pixel represented by a uint32_t
			imageBuffer[circleY * imageWidth + circleX] = 0xFFFFFFFF; // Set pixel color to white
		}
	}
}

void RenderSlider(GlobalParams* m, int offsetX, int offsetY, int sizex, float position, float highlightOpacity) {
	// draw slider
	
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, offsetX+ 1, offsetY +1, sizex, 5, 0xFFFFFF, highlightOpacity); // actual slider
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, offsetX+ 0, offsetY, sizex+2, 7, 0x000000, 0.9f); // outline slider
	int pos = ((sizex) * position) + offsetX-2;
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, pos+1, offsetY -4, 4, 15, 0xFFFFFF, highlightOpacity); // actual  "knob"
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, pos, offsetY-5, 6, 17, 0x000000, 0.9f); // outline "knob"
}

void DrawBottomFakeToolbar(GlobalParams* m){

	boxBlur(GetMemoryLocation(m->scrdata, 0, m->height - m->toolheight+1, m->width, m->height), m->width, m->toolheight-1, 15, 2);


	// TOOLBAR CONTAINER
	for (uint32_t y = 0; y < m->toolheight; y++) {
		for (uint32_t x = 0; x < m->width; x++) {
			uint32_t color = 0x050505;
			float opacity = 0.65f;

			if (y == 0) {color = 0x000000; opacity=1.0f;}
			//if (y == 1) color = 0x000000;
			uint32_t* memoryPath = GetMemoryLocation(m->scrdata, x, m->height-y, m->width, m->height);
			*memoryPath = lerp(*memoryPath, color, opacity); // transparency
		}
	}

	for (uint32_t x = 0; x < m->width; x++) { 
		uint32_t y = m->toolheight-2; uint32_t color = 0xFFFFFF; float opacity = 0.2f;
		uint32_t* memoryPath = GetMemoryLocation(m->scrdata, x, m->height-y, m->width, m->height);
		*memoryPath = lerp(*memoryPath, color, opacity); // transparency
	}
	for (uint32_t x = 0; x < m->width; x++) { 
		uint32_t y = m->toolheight-1; uint32_t color = 0x000000; float opacity = 1.0f;
		uint32_t* memoryPath = GetMemoryLocation(m->scrdata, x, m->height-y, m->width, m->height);
		*memoryPath = lerp(*memoryPath, color, opacity); // transparency
	}
	for (uint32_t x = 0; x < m->width; x++) { 
		uint32_t y = 0; uint32_t color = 0xFFFFFF; float opacity = 0.2f;
		uint32_t* memoryPath = GetMemoryLocation(m->scrdata, x, m->height-y, m->width, m->height);
		*memoryPath = lerp(*memoryPath, color, opacity); // transparency
	}
}

void DrawDrawModeMenu(GlobalParams* m){
	//if (m->width < 620) { return; }
	SwitchFont(m->SegoeUI);
	m->drawMenuOffsetX = 439; // CHANGED WHEN ADDING SEPERATORS  // -------changed when added copy----- // changed when padded icon less
	m->drawMenuOffsetY = 0;

	int sizeCx = m->width-m->drawMenuOffsetX - 80; // offset
	if (sizeCx < 490) { // changed when added copy +31 // changed when adding hard/soft
		m->drawMenuOffsetX = 0;
		m->drawMenuOffsetY = m->height - 41;
		sizeCx = m->width;
		
		// bottom toolbar
		DrawBottomFakeToolbar(m);
	}

	// draw outline
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX, m->drawMenuOffsetY+1, sizeCx, 40, 0x000000, 0.5f);
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX+1, m->drawMenuOffsetY+2, sizeCx-2, 38, 0xFFFFFF, 0.3f);

	// draw main
	dDrawFilledRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX+1, m->drawMenuOffsetY+2, sizeCx-2, 38, 0x0B0B0B, 0.5f);

	// draw seperator
	dDrawFilledRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX+75, m->drawMenuOffsetY + 15, 1, 17, 0xFFFFFF, 0.3f);
	dDrawFilledRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX + 264, m->drawMenuOffsetY + 15, 1, 17, 0xFFFFFF, 0.3f);

	// draw text
	PlaceString(m, 14, "Color", m->drawMenuOffsetX +11, m->drawMenuOffsetY + 13, 0xFFFFFF, m->scrdata);
	PlaceString(m, 14, "Size", m->drawMenuOffsetX +83, m->drawMenuOffsetY + 13, 0xFFFFFF, m->scrdata);
	PlaceString(m, 14, "Opacity", m->drawMenuOffsetX +270, m->drawMenuOffsetY + 13, 0xFFFFFF, m->scrdata);

	std::string drawstr = std::to_string((int)std::round(m->drawSize));

	int off = 235;
	if(m->drawSize>9.5f) {
		off = 230;
	}
	if(m->drawSize>99.5f) {
		off = 225;
	}
	
	PlaceString(m, 14, drawstr.c_str(), m->drawMenuOffsetX + off, m->drawMenuOffsetY + 13, 0xFFFFFF, m->scrdata);

	char str2[256];
	sprintf(str2, "%d%%", (int)round(m->a_opacity*100.0f));
	PlaceString(m, 14, str2, m->drawMenuOffsetX + 414, m->drawMenuOffsetY + 13, 0xFFFFFF, m->scrdata);

	// draw color square
	dDrawFilledRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX + 51, m->drawMenuOffsetY + 14, 18, 18, m->a_drawColor, true); // actual color
	dDrawRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX + 51, m->drawMenuOffsetY + 14, 18, 18, 0xFFFFFF, 0.3f); // outline (white)
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX +50, m->drawMenuOffsetY + 13, 20, 20, 0x000000, 0.9f); // outline (black)
	
	// draw hard/soft area
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX + 461, m->drawMenuOffsetY + 14, 18, 18, 0x808080, 1.0f); // outline (white)
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX + 460, m->drawMenuOffsetY + 13, 20, 20, 0x000000, 0.9f); // outline (black)

	std::string let = "H";
	if (m->a_softmode) {
		let = "S";
	}

	PlaceString(m, 14, let.c_str(), m->drawMenuOffsetX + 465, m->drawMenuOffsetY + 14, 0x808080, m->scrdata);


	float sizeLeveler = sqrt(m->drawSize-1) / 10.0f; // I used ALGEBRA!
	if (sizeLeveler > 1.0f) { sizeLeveler = 1.0f; }
	if (sizeLeveler < 0.0f) { sizeLeveler = 0.0f; }

	float opacitylever = m->a_opacity;

	POINT mp;
	GetCursorPos(&mp);
	ScreenToClient(m->hwnd, &mp);

	// Hover
	//------------------------------------------------------------------------------------------------------------------------------
	int slider1begin = m->drawMenuOffsetX + m->slider1begin;
	int slider1end = m->drawMenuOffsetX + m->slider1end;

	int slider2begin = m->drawMenuOffsetX + m->slider2begin;
	int slider2end = m->drawMenuOffsetX + m->slider2end;

	int sliderYb = m->drawMenuOffsetY;
	int sliderYe = m->drawMenuOffsetY + 40;

	float highlightOpacity1 = 0.3f;
	float highlightOpacity2 = 0.3f;

	if (CheckIfMouseInSlider1(mp, m, slider1begin, slider1end, sliderYb, sliderYe) || m->slider1mousedown) {
		
		highlightOpacity1 = 0.45f;
		if(m->toolmouseDown) {
			highlightOpacity1 = 0.75f;
		}
	}

	if (CheckIfMouseInSlider2(mp, m, slider2begin, slider2end, sliderYb, sliderYe) || m->slider2mousedown) {
		highlightOpacity2 = 0.45f;
		if(m->toolmouseDown) {
			highlightOpacity2 = 0.75f;
		}
	}
	//------------------------------------------------------------------------------------------------------------------------------


	RenderSlider(m, m->drawMenuOffsetX +m->slider1begin, m->drawMenuOffsetY + 19, m->slider1end-m->slider1begin, sizeLeveler, highlightOpacity1);
	RenderSlider(m, m->drawMenuOffsetX +m->slider2begin, m->drawMenuOffsetY + 19, m->slider2end-m->slider2begin, opacitylever, highlightOpacity2);

	//FT_Done_Face(face);
	//FT_Done_FreeType(ft);


}

BITMAPINFO bmi;
void UpdateBuffer(GlobalParams* m) {
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = m->width;
	bmi.bmiHeader.biHeight = -(int64_t)m->height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	// tell our lovely win32 api to update the window for us <3
	SetDIBitsToDevice(m->hdc, 0, 0, m->width, m->height, 0, 0, 0, m->height, m->scrdata, &bmi, DIB_RGB_COLORS);
}

void Tint(GlobalParams* m) {
	for(int y=0; y<m->height; y++) {
		for (int x = 0; x< m->width; x++) {
					if (((x) + (y)) % 2 == 0) {
						*GetMemoryLocation(m->scrdata, x, y, m->width, m->height) = 0x000000;
					}
				}
		}
}

void RenderCropButton(GlobalParams* m, int px, int py, bool invertX, bool invertY, bool selected) {
	if (px >= m->width - 16) { return; }
	if (py >= m->height - 16) { return; }
	if (px < 1) { return; }
	if (py < 1) { return; }

	uint32_t distLeft, distRight, distTop, distBottom;
	GetCropCoordinates(m, &distLeft, &distRight, &distTop, &distBottom);

	for (int y = 0; y < 16; y++) {
		for (int x = 0; x < 16; x++) {
			
			uint32_t* from = GetMemoryLocation(m->cropImageData, x, y, 16, 16);
			
			int xp = (-x + (2 * x * (1 - invertX))) + (invertX * 15); // math without logic
			int yp = (-y + (2 * y * (1 - invertY))) + (invertY * 15);
			int toLocX = xp + px;
			int toLocY = yp + py;

			// check within bounds

			uint32_t* to = GetMemoryLocation(m->scrdata, toLocX,toLocY, m->width, m->height);
			if (*from != 0xFFFF00FF) {
				if (selected) {
					*to = 0xFFFFFF;
				}
				else {
					*to = *from;
				}
			}
		}
	}
}

void DrawFrame(GlobalParams* m, uint32_t framecolor, uint32_t left, uint32_t right, uint32_t top, uint32_t bottom) {

	// sorry for the mess
	int wFrame = right - left;
	int hFrame = bottom - top;
	for (int y = 0; y < hFrame; y++) {
		if (((top + y) > 0 && (top + y) <= m->height) && ((left) > 0 && (left) <= m->width)) {
			*GetMemoryLocation(m->scrdata, left, top + y, m->width, m->height) = framecolor;
		}
	}
	for (int y = 0; y < hFrame; y++) {
		if (((top + y) > 0 && (top + y) <= m->height) && ((right) > 0 && (right) <= m->width)) {
			*GetMemoryLocation(m->scrdata, right, top + y, m->width, m->height) = framecolor;
		}
	}
	for (int x = 0; x < wFrame; x++) {
		if (((left + x) > 0 && (left + x) <= m->width) && ((top) > 0 && (top) <= m->height)) {
			*GetMemoryLocation(m->scrdata, left + x, top, m->width, m->height) = framecolor;
		}
	}
	for (int x = 0; x < wFrame; x++) {
		if (((left + x) > 0 && (left + x) <= m->width) && ((bottom) > 0 && (bottom) <= m->height)) {
			*GetMemoryLocation(m->scrdata, left + x, bottom, m->width, m->height) = framecolor;
		}
	}
}

void RenderCropGUI(GlobalParams* m) {
	// crop UI

	for (int y = 0; y < m->height; y++) {
		for (int x = 0; x < m->width; x++) {
			// tint screen
			uint32_t* data = GetMemoryLocation(m->scrdata, x, y, m->width, m->height);
			*data = lerp(0x000000, *data, 0.3f);
			
		}
	}

	SwitchFont(m->OCRAExt);
	PlaceString(m, 14, "Move handles with mouse then right click to confirm changes", 10, 10, 0xFFFFFF, m->scrdata);

	

	uint32_t distLeft, distRight, distTop, distBottom;
	GetCropCoordinates(m, &distLeft, &distRight, &distTop, &distBottom);

	// render crop thingy
	RenderCropButton(m, distLeft, distTop, false, false, m->CropHandleSelectTL);
	RenderCropButton(m, distRight-16, distTop, true, false, m->CropHandleSelectTR);
	RenderCropButton(m, distLeft, distBottom-16, false, true, m->CropHandleSelectBL);
	RenderCropButton(m, distRight-16, distBottom-16, true, true, m->CropHandleSelectBR);

	int widthOfImage = m->imgwidth * m->mscaler;
	int heightOfImage = m->imgheight * m->mscaler;

	DrawFrame(m, 0xFFFFFF, distLeft, distRight, distTop, distBottom);

	//dDrawRectangle(m->scrdata, m->width, m->height, m->CoordLeft, m->CoordTop, widthOfImage, heightOfImage, 0xFFFFFF, 0.5f);


}

void RenderDrawModeGuide(GlobalParams* m){
	SwitchFont(m->SegoeUI);

	uint32_t x = m->dmguide_x;
	uint32_t y = m->dmguide_y;
	uint32_t sx = m->dmguide_sx;
	uint32_t sy = m->dmguide_sy;

	gaussian_blur((uint32_t*)m->scrdata, sx-2, sy-2, 4.0f, m->width, m->height, x+1, y+1);
	
	dDrawFilledRectangle(m->scrdata, m->width, m->height, x + 1, y + 1, sx-2, sy-2, 0x000000, 0.4f);
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, x + 1, y + 1, sx-2, sy-2, 0xFFFFFF, 0.3f);
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, x, y, sx, sy, 0x000000, 0.8f);

	int ilocx = 5;
	int ilocy = 47;
	// icons
	for(int y=0; y<164; y++) {
		for(int x=0; x<41; x++) {
			uint32_t* ontxt = GetMemoryLocation(m->dmguideIconData, x, y, 41, 164);
			uint32_t* scr = GetMemoryLocation(m->scrdata, x+ilocx, y+ilocy, m->width, m->height);

			uint8_t alpha = (*ontxt >> 24) & 0xFF;
			
			float falpha = (float)alpha/255.0f;
			*scr = lerp(*scr, *ontxt, falpha);
		}
	}

	// not hover, but selected

	if ((GetKeyState(VK_CONTROL) & 0x8000 && GetKeyState(VK_SHIFT) & 0x8000) || m->drawtype == 3) {
		// transparent
		dDrawRoundedRectangle(m->scrdata, m->width, m->height, ilocx+2, ilocy+84, 37, 37, 0xFFFFFF, 0.4f);
	}
	else if (GetKeyState(VK_CONTROL) & 0x8000 || m->rightdown || (m->drawtype == 0)) {
		// erase
		dDrawRoundedRectangle(m->scrdata, m->width, m->height, ilocx+2, ilocy+43, 37, 37, 0xFFFFFF, 0.4f);
	} else {
		// pen
		dDrawRoundedRectangle(m->scrdata, m->width, m->height, ilocx+2, ilocy+2, 37, 37, 0xFFFFFF, 0.4f);
	}

	if(m->eyedroppermode) {
		dDrawRoundedRectangle(m->scrdata, m->width, m->height, ilocx+2, ilocy+125, 37, 37, 0xFFFFFF, 0.4f);
	}

	// hover
	POINT mPP;
	GetCursorPos(&mPP);
	ScreenToClient(m->hwnd, &mPP);

	// direct copy paste from events (toolbarmousedown)
	// each draw call is also copy pasted with lower transparency

	int button = -1;
	if ((mPP.x > (m->dmguide_x) && mPP.x <= (m->dmguide_x+m->dmguide_sx))) {
		if((mPP.y > (m->dmguide_y) && mPP.y <= (m->dmguide_y+43))) {
			// pen
			button = 0;

		}
		if((mPP.y > (m->dmguide_y+43) && mPP.y <= (m->dmguide_y+84))) {
			// erase
			button = 1;

		}
		if((mPP.y > (m->dmguide_y+84) && mPP.y <= (m->dmguide_y+125))) {
			// transparent
			button = 2;

		}
		if((mPP.y > (m->dmguide_y+125) && mPP.y <= (m->dmguide_y+168))) {
			// eyedropper
			button = 3;

		}
	}

	int offsets[] = {2, 43, 84, 125};
	std::string txts[] = {"Draw", "Erase", "Transparent", "Eyedropper"};

	if(button != -1) {
		dDrawRoundedRectangle(m->scrdata, m->width, m->height, ilocx+2, ilocy+offsets[button], 37, 37, 0xFFFFFF, 0.2f);

		// tooltip

		uint32_t loc = 60;
		uint32_t yloc = offsets[button]+8;
			
		std::string txt = txts[button];
		gaussian_blur((uint32_t*)m->scrdata, (txt.length() * 8) + 12, 20, 4.0f, m->width, m->height, loc-1, m->toolheight+4+yloc);
						
		dDrawFilledRectangle(m->scrdata, m->width, m->height, loc-1, m->toolheight + 4+yloc, (txt.length() * 8) + 12, 20, 0x000000, 0.4f);
		dDrawRoundedRectangle(m->scrdata, m->width, m->height, loc - 1, m->toolheight + 4+yloc, (txt.length() * 8) + 12, 20, 0xFFFFFF, 0.3f);
		dDrawRoundedRectangle(m->scrdata, m->width, m->height, loc - 2, m->toolheight + 3+yloc, (txt.length() * 8) + 14, 22, 0x000000, 0.8f);

		SwitchFont(m->OCRAExt);
		PlaceStringShadow(m, 14, txt.c_str(), loc + 3, m->toolheight + 4+yloc, 0xFFFFFF, m->scrdata, 0.34f, 1, 1, 1);
		
	}




}

void RedrawSurfaceTextDialog(GlobalParams* m) {
	RedrawSurface(m);
}

void RedrawSurface(GlobalParams* m, bool onlyImage, bool doesManualClip, bool bypassSleep) {

	if (!doesManualClip) {
		SelectClipRgn(m->hdc, NULL);
	}


	if(!bypassSleep) {
		if (m->sleepmode && (!m->isImagePreview)) {
			return;
		}
	}
	

	if (m->width < 20) { return; }

	POINT p;
	GetCursorPos(&p);
	ScreenToClient(m->hwnd, &p);

	// set the coordinates for the image
	if ((!onlyImage)) {
		// We aren't going to reset the coordinates due to the coordinates already being reset by 
		// the function calling redrawsurface. when passing in onlyimage we assume this is being done
		ResetCoordinates(m);
	}

	// update menu conditions
	m->undo_menucondition = m->undoStep > 0;
	m->redo_menucondition = (m->undoStep) < ((int)m->undoData.size() - 1); // got from RedoBus

	//int s = m->undoStep;
	//int step = m->undoData.size() - 1;
	//if (s < step) {

	m->isimage_menucondition = (m->imgwidth>0 && m->imgheight>0 && m->imgdata);

	void* drawingbuffer = m->imgdata;
	if (m->isImagePreview) {
		drawingbuffer = m->imagepreview;
	}

	if (drawingbuffer) {
		if ((m->smoothing && ((!m->mouseDown) || m->drawmode)) && !m->isInCropMode) {
			PlaceImageBI(m, drawingbuffer, true, p);
		}
		else {
			PlaceImageNN(m, drawingbuffer, true, p);
		}
	}
	else if (m->imgwidth > 1) {
		MessageBox(m->hwnd, "Failed to obtain drawing buffer", "Error", MB_OK | MB_ICONERROR);
		exit(0);
	} else {
		RenderBK(m, p);
	} 


	// draw 1px frame
	if (!m->isInCropMode) {
		DrawFrame(m, 0x000000, m->CoordLeft, m->CoordRight, m->CoordTop, m->CoordBottom);
	}

	//dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->CoordLeft, m->CoordTop, wFrame, hFrame, 0x808080, 1.0f);


	// debug circles
	//drawCircle(m->CoordLeft, m->CoordTop, 16, (uint32_t*)m->scrdata, m->width);
	//drawCircle(m->CoordRight, m->CoordTop, 16, (uint32_t*)m->scrdata, m->width);


	if (m->drawmode) {
		RenderDrawModeGuide(m);
	}


	if ((CanRenderToolbarMacro) && (!onlyImage)) {
		RenderToolbar(m);

	}
	if ((CanRenderToolbarMacro)) {
		RenderToolbarShadow(m);
	}

	bool shouldIDrawTheDrawingToolbar = !onlyImage;
	if (m->drawMenuOffsetY > m->toolheight) {
		shouldIDrawTheDrawingToolbar = true;
	}
	if (m->drawmode && (shouldIDrawTheDrawingToolbar) && (((!m->fullscreen && m->height >= 250) || p.y < m->toolheight) || m->drawMenuOffsetY > 1)) {
		DrawDrawModeMenu(m);
	}





	if (m->isInCropMode) {
		RenderCropGUI(m);
	}


	if (m->isMenuState) {
		DrawMenu(m);
	}


	// draw annotation circle

	// get nearest pixel coord and round
	
	// to image
    int k = (int)((float)(p.x - m->CoordLeft) * (1.0f / m->mscaler));
    int v = (int)((float)(p.y - m->CoordTop) * (1.0f / m->mscaler));

	int realx = (float)m->CoordLeft + (((float)k+0.5f) * m->mscaler+0.5f);
	int realy = (float)m->CoordTop + (((float)v+0.5f) * m->mscaler);

	float actdrawsize = m->drawSize;
	
	if(m->drawtype == 0 && m->drawSize > 1.5f) {
		actdrawsize *= 2.0f;
	}

	if (m->drawmode && actdrawsize > 0 && m->mscaler > 0 && !m->eyedroppermode) {
		if(IsInImage(p, m) && (!IfInMenu(p, m) || !m->isMenuState) ) {
			float dia = actdrawsize * m->mscaler-2;
			if(dia > 4.0f) {
				CircleGenerator(dia, realx, realy, 0x808080, (uint32_t*)m->scrdata,  m->width, m->height);
			}
		}
	}

	if (m->tint || m->deletingtemporaryfiles) {
		Tint(m);
	}

	if (m->loading) {
		SwitchFont(m->SegoeUI);
		PlaceStringShadow(m, 20, "Loading", 10, m->toolheight + 10, 0xFFFFFF, m->scrdata, m->def_txt_shadow_softness, 1,1, 2);
	}

	if (m->deletingtemporaryfiles) {
		SwitchFont(m->OCRAExt);
		PlaceString(m, 30, "-- Maintenance --", 33, 50, 0xFFFFFF, m->scrdata);
		PlaceString(m, 50, "Deleting temporary files", 33, 100, 0xFFFFFF, m->scrdata);
	}


	// debug mode

	SwitchFont(m->SegoeUI);
	if (m->debugmode) {
		char debug[256];
		sprintf(debug, "WASDX: %f: MS: %f: RES: %f: Undo Queue: %d:Undo Step: %d: Elapsed debug time: %f", m->wasdX, m->ms_time, m->a_resolution, m->ProcessOfMakingUndoStep, m->undoStep, m->etime);
		PlaceString(m, 16, debug, 12, m->toolheight + 8, 0x808080, m->scrdata);

	}

	if(m->draw_updates_debug) {
		uint32_t testc = 0xFF00FF;
		if(rand()%2==0) {
			testc = 0x00FF00;
		}
		dDrawFilledRectangle(m->scrdata, m->width, m->height, 0, 0, m->width, m->height, testc, 0.5f);
	}

	// update buffer
	UpdateBuffer(m);

	// Update window title
	
	
	std::string aststring = "";
	if (m->shouldSaveShutdown) {
		aststring = "*";
	}

	std::string titlebar_string = m->name_full;
	if(!m->fpath.empty()) {
		titlebar_string += " | " + m->fpath + aststring + " | " + std::to_string((int)(m->mscaler * 100.0f)) + "%";
	}
	if(m->drawmode) {
		titlebar_string += " (Annotate)";
	}
	SetWindowText(m->hwnd, titlebar_string.c_str());
}