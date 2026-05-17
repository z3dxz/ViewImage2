
#include "headers/rendering.hpp"
#include <vector>
#include <thread>
#include <time.h>
#include <wincodec.h>
#include "headers/renderops.hpp"

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

#define CanRenderToolbarMacro (((!m->fullscreen && m->height >= 250) || p.y < m->toolheight || m->isMenuState)&&!m->isInCropMode)

void dDrawRectangle(GlobalParams* m, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {

    // Draw the 4 edges
    drawLine(m, xloc, yloc, width, true, color, opacity);
    drawLine(m, xloc, yloc + height - 1, width, true, color, opacity);
    drawLine(m, xloc, yloc + 1, height - 2, false, color, opacity);
    drawLine(m, xloc + width - 1, yloc + 1, height - 2, false, color, opacity);
}

void dDrawRoundedRectangle(GlobalParams* m, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
    drawLine(m, xloc+1, yloc, width-2, true, color, opacity);
    drawLine(m, xloc+1, yloc + height - 1, width-2, true, color, opacity);
    drawLine(m, xloc, yloc + 1, height - 2, false, color, opacity);
    drawLine(m, xloc + width - 1, yloc + 1, height - 2, false, color, opacity);
}

void dDrawRoundedFilledRectangle(GlobalParams* m, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
    if (width <= 0 || height <= 0) return;

    drawLine(m, xloc + 1, yloc, width - 2, true, color, opacity);
    drawLine(m, xloc + 1, yloc + height - 1, width - 2, true, color, opacity);

    if (height > 2) {
        dDrawFilledRectangle(m, xloc, yloc + 1, width, height - 2, color, opacity);
    }
}

void RenderToolbarIcon(GlobalParams* m, int index, int locationX, uint32_t color, uint32_t selectedColor) {

	ToolbarButtonItem* item = &m->toolbartable[index];

	int offsetX = item->indexX;
	
	uint32_t tint = color;
	if (index == m->selectedbutton) {
		tint = selectedColor;
	}

	PlaceFromAtlas(m, m->toolbarData, m->widthos, m->heightos, offsetX, 0, locationX, 6, m->iconSize, m->iconSize, tint, 1.0f);

	if (item->isSeperator) {
		int location = locationX + (m->iconSize)+4;
		dDrawRectangle(m, location, 12, 1, m->toolheight-25, 0x4DFFFFFF, 0.3);
	}
}

void RenderFullscreenIcon(GlobalParams* m, bool aeromode){

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
			// what happens when you hover over the screen button
			//-- The outline
			dDrawRoundedRectangle(m, m->width-36, 12, 24, 23, 0xFFFFFF, 0.2f);
			dDrawRoundedRectangle(m, m->width-37, 11, 26, 25, 0x000000, 1.0f);


			//-- Fullscreen icon tooltip
			SwitchFont(m->SegoeUI);
			if (m->fullscreen) {
				PlaceStringShadow(m, 13, "Exit Fullscreen (F11)", m->width - 120, 48, 0xFFFFFF, m->def_txt_shadow_softness, 1,1, 2);
			}
			else {
				PlaceStringShadow(m, 13, "Fullscreen (F11)", m->width - 100, 48, 0xFFFFFF, m->def_txt_shadow_softness, 1,1, 2);
			}
		}

		// outline for aero
		if(aeromode) {
			dDrawRoundedRectangle(m, m->width-31, 17, 14, 13, 0x80000000, 0.5f);
			dDrawRoundedRectangle(m, m->width-29, 21, 10, 7, 0x80000000, 0.5f);
		}

		// actually draw the fullscreen icon
		float t = 0.5f;
		if (nearf) {
			t = 1.0f;
		}
		PlaceFromAtlas(m, m->fullscreenIconData, 12, 11, 0, 0, m->width-30, 18, 12, 11, 0xFFFFFFFF, t);
	}
	
}

void RenderToolbarContainer(GlobalParams* m) {

	// First the blur
	if (m->CoordTop <= m->toolheight) {
		blur_toolbar(m);
	}
	// the background
	dDrawFilledRectangle(m, 0, 1, m->width, m->toolheight-1, 0x0C0C0C, 0.65f);

	// The three border lines
	drawLine(m, 0, 0, m->width, true, 0x333333, 1.0f);
	drawLine(m, 0, m->toolheight-2, m->width, true, 0xFFFFFF, 0.2f);
	drawLine(m, 0, m->toolheight-1, m->width, true, 0x000000, 1.0f);
}

void RenderToolbarContainerAero(GlobalParams* m) {
	// make clear
	dDrawFilledRectangle(m, 0, 0, m->width, m->toolheight, 0x00000000, 1.0f);
}

void RenderToolbarIcons(GlobalParams* m, uint32_t color, uint32_t selectedColor) {
	// GetLocationFromButton, RenderToolbarButtons, getXbuttonID
	// Make sure they are all synced

	int p = m->starttoolbarloc;
	for (size_t i = 0; i < m->toolbartable.size(); i++) {
		RenderToolbarIcon(m, i, p+2, color, selectedColor);
		
		p += GetIndividualButtonPush(m, i);
		if (m->imgwidth < 1) {
			return;
		}
		if (m->drawmode && i == 11) {
			return;
		}
	}
}

void RenderToolbarButtons(GlobalParams* m, bool aeromode){
	// BUTTONS 
	if (!m->toolbarData) return;
	
	//-- The border when selecting annotate
	if (m->drawmode) {
		// 7 means draw
		dDrawRoundedRectangle(m, GetLocationFromButton(m, 7), 4, m->iconSize+4, m->toolheight - 8, 0xFFFFFFFF, 0.2f);
		dDrawRoundedRectangle(m, GetLocationFromButton(m, 7)-1, 3, m->iconSize+4 + 2, m->toolheight - 6, 0xFF000000, 1.0f);
	}
	
	RenderToolbarIcons(m, 0xC0FFFFFF, 0xC0FFE0E0);

	if (m->selectedbutton >= 0 && m->selectedbutton < m->toolbartable.size()) {
		int in = m->selectedbutton;

		// hover fill
		dDrawRoundedFilledRectangle(m, GetLocationFromButton(m, in), 4, m->iconSize+4, m->toolheight - 8, 0xFFFF8080, 0.3f);
		
		// hover border
		dDrawRoundedRectangle(m, GetLocationFromButton(m, in), 4, m->iconSize+4, m->toolheight - 8, 0xFFFFFFFF, 0.3f);
		dDrawRoundedRectangle(m, GetLocationFromButton(m, in)-1, 3, m->iconSize+4 +2, m->toolheight - 6, 0xFF000000, 1.0f);
	}
}

void DrawVersion(GlobalParams* m, bool aeromode) {
	// version
	SwitchFont(m->OCRAExt);
	int yloc = 15;
	if(m->width <= 540) {
		yloc = m->toolheight + 4;
	}
	std::string str = REAL_BIG_VERSION;
	uint32_t vcolor = m->aeromode ? 0xFFFFFFFF : 0xFFb0b0b0;
	if(str.find('*') != std::string::npos) {
		vcolor = 0xFFFF8080;
	}
	
	if(aeromode) {
		PlaceStringShadow(m, 13, str.c_str(), m->width - 71, yloc, 0xFF000000, 1.34f, 0,0,4, vcolor);
	} else {
		PlaceString(m, 13, str.c_str(), m->width - 71, yloc, vcolor);
	}	
}

void RenderToolbar(GlobalParams* m, bool aeromode) {

	int lcd = m->lcd;
	if(aeromode) m->lcd = false;

	if(aeromode) {
		RenderToolbarContainerAero(m);
	} else {
		RenderToolbarContainer(m);
	}

	DrawVersion(m, aeromode);
	RenderFullscreenIcon(m, aeromode);

	RenderToolbarButtons(m, aeromode);

	if(aeromode) m->lcd = lcd;
}

void RenderToolbarTooltips(GlobalParams* m) {
	// tooltips
	if (m->selectedbutton < 0 || m->selectedbutton >= m->toolbartable.size() || m->isMenuState) {
		return;
	}

	int in = m->selectedbutton;
	std::string txt = "Error";
	if (m->selectedbutton < m->toolbartable.size()) {
		txt = m->toolbartable[m->selectedbutton].name;
	}

	int loc = 1 + (GetLocationFromButton(m, in));

	// draw guide
	if(in != 7 || !m->drawmode) {
		// actual tooltips
		gaussian_blur(m, (txt.length() * 8) + 12, 20, 4.0f, loc-1, m->toolheight+4);

		dDrawFilledRectangle(m, loc-1, m->toolheight + 4, (txt.length() * 8) + 12, 20, 0x000000, 0.4f);
		dDrawRoundedRectangle(m, loc - 1, m->toolheight + 4, (txt.length() * 8) + 12, 20, 0xFFFFFF, 0.3f);
		dDrawRoundedRectangle(m, loc - 2, m->toolheight + 3, (txt.length() * 8) + 14, 22, 0x000000, 0.8f);

		SwitchFont(m->OCRAExt);
		PlaceStringShadow(m, 14, txt.c_str(), loc + 3, m->toolheight + 4, 0xFFFFFF, 0.34f, 1, 1, 1);
	} else {
		int off = -20;
		uint32_t shiftx = loc+2;
		SwitchFont(m->Verdana);

		uint32_t dcolor = 0xFFC0C0C0;
		PlaceString(m, 12, "Annotate Shortcuts", shiftx, m->toolheight + 28+off, dcolor);
		SwitchFont(m->OCRAExt);

		PlaceString(m, 10, "Draw        Left Mouse", shiftx, m->toolheight + 43+off, dcolor);
		PlaceString(m, 10, "Pan         Middle Mouse", shiftx, m->toolheight + 53+off, dcolor);
		PlaceString(m, 10, "Erase       Ctrl", shiftx, m->toolheight + 63+off, dcolor);
		PlaceString(m, 10, "Transparent Ctrl+Shift", shiftx, m->toolheight + 73+off, dcolor);
		PlaceString(m, 10, "Eyedropper  Shift+Z", shiftx, m->toolheight + 83+off, dcolor);
	}
}

void DrawMenuIcon(GlobalParams* m, int locationX, int locationY, int atlasX, int atlasY, int opacity2) {
	PlaceFromAtlas(m, m->menu_icon_atlas, m->menu_atlas_SizeX, m->menu_atlas_SizeY, atlasX, atlasY, locationX, locationY, 12, 12, 0xFF6060, 1.0f);
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

	gaussian_blur(m, miX-4, miY-4, 4.0f, posX+2, posY+2);
	dDrawRoundedFilledRectangle(m, posX, posY, miX, miY, 0x000000, 0.65f);
	dDrawRoundedRectangle(m, posX+1, posY+1, miX-2, miY-2, 0xFFFFFF, 0.2f);
	dDrawRoundedRectangle(m, posX, posY, miX, miY, 0x000000, 1.0f);

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
		dDrawRoundedFilledRectangle(m, hoverLocX, hoverLocY, hoverSizeX, hoverSizeY, 0xFF8080, 0.3f);   // FILL
		dDrawRoundedRectangle(m, hoverLocX, hoverLocY, hoverSizeX, hoverSizeY, 0xFFFFFF, 0.3f);         // white
		dDrawRoundedRectangle(m, hoverLocX - 1, hoverLocY - 1, hoverSizeX + 2, hoverSizeY + 2, 0x000000, 1.0f); // black BORDER!
	}
	

	SwitchFont(m->Verdana);
	for (int i = 0; i < m->menuVector.size(); i++) {
		bool enabled = *(m->menuVector[i].enable_condition);
		
		std::string mystr = m->menuVector[i].name;
		if (mystr.length() >= 3 && mystr.substr(mystr.length() - 3) == "{s}") {
			mystr = mystr.substr(0, mystr.length() - 3);
		}


		int opacity = 255;
		if(!enabled) {
			opacity = 128;
			PlaceString(m, 12, mystr.c_str(), posX + 26, (mH * i) + posY + 9, 0x808080);
		} else {
			PlaceStringShadow(m, 12, mystr.c_str(), posX + 26, (mH * i) + posY + 9, 0xF0F0F0, 0.5, 1, 1, 1, 0xFF000000);
		}
		
		DrawMenuIcon(m, posX + 10, (mH* i) + posY + 10, m->menuVector[i].atlasX, m->menuVector[i].atlasY, opacity);
												// changed when adding icon
		
		if (i == m->menuVector.size()-1) continue;

		if (strstr(m->menuVector[i].name.c_str(), "{s}")) { // seperator
			dDrawFilledRectangle(m, posX + 8, posY + mH + (mH * i) + 2, miX - 16, 1, 0xFFFFFF, 0.25f);
		}
	}
}

void RenderSlider(GlobalParams* m, Slider slider, POINT mPP, float position) {
	float highlightOpacity = 0.3f;

	if (IsInSlider(slider)) {
		
		highlightOpacity = 0.45f;
		if(m->Leftdown) {
			highlightOpacity = 0.75f;
		}
	}

	int offsetX = (*slider.parentX)+slider.x;
	int offsetY = (*slider.parentY)+slider.y;
	int sizex = slider.endX - slider.x;
	int sizey = slider.endY - slider.y;

	// draw slider
	uint32_t colorh = ((int)(highlightOpacity*255.0f) >> 24) | (255 << 16) | (255 << 8) | 255;
	
	dDrawRoundedRectangle(m, offsetX+1, offsetY+sizey/2-2, sizex, 5, colorh, highlightOpacity); // actual slider
	dDrawRoundedRectangle(m, offsetX+ 0, offsetY+sizey/2-3, sizex+2, 7, 0xE6000000, 0.9f); // outline slider
	int pos = ((sizex) * position) + offsetX-2;
	dDrawRoundedRectangle(m, pos+1, offsetY+1, 4, sizey-2, colorh, highlightOpacity); // actual  "knob"
	dDrawRoundedRectangle(m, pos, offsetY, 6, sizey, 0xE6000000, 0.9f); // outline "knob"
}

void DrawBottomFakeToolbar(GlobalParams* m) {

	boxBlur(m, 15, 2, m->height - m->toolheight+1, m->toolheight-1);

	dDrawFilledRectangle(m, 0, m->height-m->toolheight+2, m->width, m->toolheight-2, 0x050505, 0.65f);
	dDrawFilledRectangle(m, 0, m->height-m->toolheight+2, m->width, 1, 0xFFFFFF, 0.2f);
	dDrawFilledRectangle(m, 0, m->height-m->toolheight+1, m->width, 1, 0x000000, 1.0f);

}

void DrawDrawModeMenu(GlobalParams* m){
	bool aeromode = m->aeromode && (!m->fullscreen);
	int lcd = m->lcd;
	if(aeromode) m->lcd = false;

	uint32_t textc = 0xFFFFFF;
	//if (m->width < 620) { return; }
	SwitchFont(m->SegoeUI);
	m->drawMenuOffsetX = 434; // CHANGED WHEN ADDING SEPERATORS  // -------changed when added copy----- // changed when padded icon less // changed when squishing a little
	m->drawMenuOffsetY = 0;

	int sizeCx = m->width-m->drawMenuOffsetX - 77; // offset
	if (sizeCx < 480) { // changed when added copy +31 // changed when adding hard/soft // changed when squishing a little
		m->drawMenuOffsetX = 0;
		m->drawMenuOffsetY = m->height - 41;
		sizeCx = m->width;
		
		// bottom toolbar
		DrawBottomFakeToolbar(m);
	}
	
	// draw outline
	dDrawRoundedRectangle(m, m->drawMenuOffsetX, m->drawMenuOffsetY+1, sizeCx, 40, 0x80000000, 0.5f);
	dDrawRoundedRectangle(m, m->drawMenuOffsetX+1, m->drawMenuOffsetY+2, sizeCx-2, 38, 0x4DFFFFFF, 0.3f);

	// draw main
	dDrawFilledRectangle(m, m->drawMenuOffsetX+1, m->drawMenuOffsetY+2, sizeCx-2, 38, 0x800B0B0B, 0.5f);

	// draw seperator
	dDrawFilledRectangle(m, m->drawMenuOffsetX+75, m->drawMenuOffsetY + 15, 1, 17, 0x4DFFFFFF, 0.3f);
	dDrawFilledRectangle(m, m->drawMenuOffsetX + 254, m->drawMenuOffsetY + 15, 1, 17, 0x4DFFFFFF, 0.3f);

	if(aeromode) {
		PlaceStringShadow(m, 14, "Color", m->drawMenuOffsetX +11, m->drawMenuOffsetY + 13, textc, 0.34f, 0, 0, 1, 0xFF000000);
		PlaceStringShadow(m, 14, "Size", m->drawMenuOffsetX +83, m->drawMenuOffsetY + 13, textc, 0.34f, 0, 0, 1, 0xFF000000);
		PlaceStringShadow(m, 14, "Opacity", m->drawMenuOffsetX +264, m->drawMenuOffsetY + 13, textc, 0.34f, 0, 0, 1, 0xFF000000);
	} else {
		PlaceString(m, 14, "Color", m->drawMenuOffsetX +11, m->drawMenuOffsetY + 13, textc);
		PlaceString(m, 14, "Size", m->drawMenuOffsetX +83, m->drawMenuOffsetY + 13, textc);
		PlaceString(m, 14, "Opacity", m->drawMenuOffsetX +264, m->drawMenuOffsetY + 13, textc);
	}

	std::string drawstr = std::to_string((int)std::round(m->drawSize));

	int off = 235;
	if(m->drawSize>9.5f) {
		off = 230;
	}
	if(m->drawSize>99.5f) {
		off = 225;
	}

	if(aeromode) {
		PlaceStringShadow(m, 14, drawstr.c_str(), m->drawMenuOffsetX + off, m->drawMenuOffsetY + 13, textc, 0.34f, 0, 0, 1, 0xFF000000);
	} else {
		PlaceString(m, 14, drawstr.c_str(), m->drawMenuOffsetX + off, m->drawMenuOffsetY + 13, textc);
	}

	char str2[256];
	sprintf(str2, "%d%%", (int)round(m->a_opacity*100.0f));
	if(aeromode) {
		PlaceStringShadow(m, 14, str2, m->drawMenuOffsetX + 414, m->drawMenuOffsetY + 13, textc,  0.34f, 0, 0, 1, 0xFF000000);
	} else {
		PlaceString(m, 14, str2, m->drawMenuOffsetX + 414, m->drawMenuOffsetY + 13, textc);
	}
	PlaceString(m, 14, str2, m->drawMenuOffsetX + 414, m->drawMenuOffsetY + 13, textc);

	// draw color square
	dDrawFilledRectangle(m, m->drawMenuOffsetX + 51, m->drawMenuOffsetY + 14, 18, 18, m->a_drawColor, true); // actual color
	dDrawRectangle(m, m->drawMenuOffsetX + 51, m->drawMenuOffsetY + 14, 18, 18, 0x4DFFFFFF, 0.3f); // outline (white)
	dDrawRoundedRectangle(m, m->drawMenuOffsetX +50, m->drawMenuOffsetY + 13, 20, 20, 0xE6000000, 0.9f); // outline (black)
	
	// draw hard/soft area h/s
	dDrawRoundedRectangle(m, m->drawMenuOffsetX + 461, m->drawMenuOffsetY + 14, 18, 18, 0x80FFFFFF, .5f); // outline (white)
	dDrawRoundedRectangle(m, m->drawMenuOffsetX + 460, m->drawMenuOffsetY + 13, 20, 20, 0xE6000000, 0.9f); // outline (black)

	std::string let = "H";
	if (m->a_softmode) {
		let = "S";
	}
	
	if(aeromode) {
		PlaceString(m, 14, let.c_str(), m->drawMenuOffsetX + 465, m->drawMenuOffsetY + 14, 0xFFC0C0C0);
	} else {
		PlaceString(m, 14, let.c_str(), m->drawMenuOffsetX + 465, m->drawMenuOffsetY + 14, 0xFF808080);
	}


	float sizeLeveler = sqrt(m->drawSize-1) / sqrt(m->imgheight-1); // I used ALGEBRA!
	if (sizeLeveler > 1.0f) { sizeLeveler = 1.0f; }
	if (sizeLeveler < 0.0f) { sizeLeveler = 0.0f; }

	float opacitylever = m->a_opacity;

	POINT mp;
	GetCursorPos(&mp);
	ScreenToClient(m->hwnd, &mp);

	RenderSlider(m, m->brush_size_slider, mp, sizeLeveler);
	RenderSlider(m, m->brush_opacity_slider, mp, opacitylever);

	if(aeromode) m->lcd = lcd;
}


void UpdateBuffer(GlobalParams* m)
{
    BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = m->width;
	bmi.bmiHeader.biHeight = -m->height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	// fun
	/*
	for(int i=0; i<100; i++) {
		int x = rand()%(m->width-40); int y = rand()%(m->width-40);
		StretchDIBits(m->hdc, x,m->height-y-40, 40, 40, x,y, 40, 40, m->scrdata, &bmi, DIB_RGB_COLORS, SRCCOPY);
	}
	*/

	StretchDIBits(m->hdc, 0, 0, m->width, m->height, 0, 0, m->width, m->height, m->scrdata, &bmi, DIB_RGB_COLORS, SRCCOPY);
	
}

void RenderCropGUI(GlobalParams* m) {
	// crop UI

	dDrawFilledRectangle(m, 0, 0, m->width, m->height, 0x000000, 0.7f);

	SwitchFont(m->OCRAExt);
	PlaceString(m, 14, "Move handles with mouse then right click to confirm changes", 10, 10, 0xFFFFFF);

	uint32_t distLeft, distRight, distTop, distBottom;
	GetCropCoordinates(m, &distLeft, &distRight, &distTop, &distBottom);

	uint32_t c = 0xFFFFFFFF; uint32_t sc = 0xFFFF8080;

	PlaceFromAtlas(m, m->cropImageData, 32, 32, 0, 0, distLeft, distTop, 16, 16, m->CropHandleSelectTL ? sc : c, 1.0f);
	PlaceFromAtlas(m, m->cropImageData, 32, 32, 16, 0, distRight-16, distTop, 16, 16, m->CropHandleSelectTR ? sc : c, 1.0f);
	PlaceFromAtlas(m, m->cropImageData, 32, 32, 0, 16, distLeft, distBottom-16, 16, 16, m->CropHandleSelectBL ? sc : c, 1.0f);
	PlaceFromAtlas(m, m->cropImageData, 32, 32, 16, 16, distRight-16, distBottom-16, 16, 16, m->CropHandleSelectBR ? sc : c, 1.0f);

	int widthOfImage = m->imgwidth * m->mscaler;
	int heightOfImage = m->imgheight * m->mscaler;

	dDrawRectangle(m, distLeft, distTop, distRight-distLeft, distBottom-distTop, 0xFFFFFFFF, 1.0f);
}

void DrawAnnotationCircle(GlobalParams* m, POINT p){
	float actdrawsize = m->drawSize;
	if(m->drawtype == 0 && m->drawSize > 1.5f)
		actdrawsize *= 2.0f;
	if(actdrawsize <= 0) {
		return;
	}
	if(IsInImage(p, m) && (!IfInMenu(p, m) || !m->isMenuState) ) {
			int k = (int)((float)(p.x - m->CoordLeft) * (1.0f / m->mscaler));
			int v = (int)((float)(p.y - m->CoordTop) * (1.0f / m->mscaler));

			int realx = (float)m->CoordLeft + (((float)k+0.5f) * m->mscaler+0.5f);
			int realy = (float)m->CoordTop + (((float)v+0.5f) * m->mscaler);

			float dia = actdrawsize * m->mscaler-2;
			if(dia > 4.0f) {
				CircleGenerator(m, dia, realx, realy, 0x808080, CanRenderToolbarMacro);
		}
	}
}

void RenderDrawModeGuide(GlobalParams* m){
	SwitchFont(m->SegoeUI);

	uint32_t x = m->dmguide_x;
	uint32_t y = m->dmguide_y;
	uint32_t sx = m->dmguide_sx;
	uint32_t sy = m->dmguide_sy;

	gaussian_blur(m, sx-2, sy-2, 4.0f, x+1, y+1);
	
	dDrawFilledRectangle(m, x + 1, y + 1, sx-2, sy-2, 0x000000, 0.4f);
	dDrawRoundedRectangle(m, x + 1, y + 1, sx-2, sy-2, 0xFFFFFF, 0.3f);
	dDrawRoundedRectangle(m, x, y, sx, sy, 0x000000, 0.8f);

	int ilocx = 5;
	int ilocy = 47;

	PlaceFromAtlas(m, m->dmguideIconData, 41, 164, 0, 0, ilocx, ilocy, 41, 164, 0xFFFFFFFF, 1.0f);

	// not hover, but selected

	if ((GetKeyState(VK_CONTROL) & 0x8000 && GetKeyState(VK_SHIFT) & 0x8000) || m->drawtype == 3) {
		// transparent
		dDrawRoundedRectangle(m, ilocx+2, ilocy+84, 37, 37, 0xFFFFFF, 0.4f);
	}
	else if (GetKeyState(VK_CONTROL) & 0x8000 || m->Rightdown || (m->drawtype == 0)) {
		// erase
		dDrawRoundedRectangle(m, ilocx+2, ilocy+43, 37, 37, 0xFFFFFF, 0.4f);
	} else {
		// pen
		dDrawRoundedRectangle(m, ilocx+2, ilocy+2, 37, 37, 0xFFFFFF, 0.4f);
	}

	if(m->eyedroppermode) {
		dDrawRoundedRectangle(m, ilocx+2, ilocy+125, 37, 37, 0xFFFFFF, 0.4f);
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
		dDrawRoundedRectangle(m, ilocx+2, ilocy+offsets[button], 37, 37, 0xFFFFFF, 0.2f);

		// tooltip

		uint32_t loc = 60;
		uint32_t yloc = offsets[button]+8;
			
		std::string txt = txts[button];
		gaussian_blur(m, (txt.length() * 8) + 12, 20, 4.0f, loc-1, m->toolheight+4+yloc);
		
		dDrawFilledRectangle(m, loc-1, m->toolheight + 4+yloc, (txt.length() * 8) + 12, 20, 0x000000, 0.4f);
		dDrawRoundedRectangle(m, loc - 1, m->toolheight + 4+yloc, (txt.length() * 8) + 12, 20, 0xFFFFFF, 0.3f);
		dDrawRoundedRectangle(m, loc - 2, m->toolheight + 3+yloc, (txt.length() * 8) + 14, 22, 0x000000, 0.8f);

		SwitchFont(m->OCRAExt);
		PlaceStringShadow(m, 14, txt.c_str(), loc + 3, m->toolheight + 4+yloc, 0xFFFFFF, 0.34f, 1, 1, 1);
		
	}
}

void RedrawSurfaceTextDialog(GlobalParams* m) {
	RedrawSurface(m);
}

bool last_render_toolbar = false;

void RedrawSurface(GlobalParams* m, bool onlyImage, bool doesManualClip, bool bypassSleep, bool clip, RECT region) {

	if (!doesManualClip) {
		SelectClipRgn(m->hdc, nullptr);
		m->full_redraw_surface_the_first_time = true;
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
		ResetCoordinates(m);
	}

	// update menu conditions
	m->undo_menucondition = m->undoStep > 0;
	m->redo_menucondition = (m->undoStep) < ((int)m->undoData.size() - 1); // got from RedoBus

	m->isimage_menucondition = (m->imgwidth>0 && m->imgheight>0 && m->imgdata);

	void* drawingbuffer = m->imgdata;
	if (m->isImagePreview) {
		drawingbuffer = m->imagepreview;
	}
	
	if (drawingbuffer) {
		if ((m->smoothing && ((!m->movemousedown) || m->drawmode)) && !m->isInCropMode) {
			PlaceImageBI(m, CanRenderToolbarMacro, drawingbuffer, true, p, clip, region);
		}
		else {
			PlaceImageNN(m, CanRenderToolbarMacro, drawingbuffer, true, p, clip, region);
		}
	} else if (m->imgwidth > 1) {
		MessageBox(m->hwnd, "Failed to obtain drawing buffer", "Error", MB_OK | MB_ICONERROR);
		exit(0);
	} else {
		PlaceImageNN(m, CanRenderToolbarMacro, nullptr, true, p, clip, region);
	} 

	// draw 1px frame
	if (!m->isInCropMode) {
		dDrawRectangle(m, m->CoordLeft, m->CoordTop, m->CoordRight-m->CoordLeft, m->CoordBottom-m->CoordTop, 0xFF000000, 1.0f);
	}

	if (m->drawmode) {
		RenderDrawModeGuide(m);
	}

	// toolbar rendering code

	bool toolbar = CanRenderToolbarMacro;

	if((toolbar != last_render_toolbar) && m->aeromode) {
		// changed
		if(CanRenderToolbarMacro && (!m->fullscreen)) {
			DwmExtend(m->hwnd, m->toolheight);
		} else {
			DwmExtend(m->hwnd, 0);
		}
	}

	last_render_toolbar = toolbar && (!m->fullscreen);
	
	if ((toolbar) && (!onlyImage)) {
		RenderToolbar(m, m->aeromode && (!m->fullscreen));
	}

	if ((toolbar)) {
		RenderToolbarTooltips(m);
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
	
	if (m->drawmode && m->mscaler > 0 && !m->eyedroppermode) {
		DrawAnnotationCircle(m, p);
	}

	if (m->tint || m->deletingtemporaryfiles) {
		Tint(m);
	}

	if (m->loading) {
		SwitchFont(m->SegoeUI);
		PlaceStringShadow(m, 20, "Loading", 10, m->toolheight + 10, 0xFFFFFF, m->def_txt_shadow_softness, 1,1, 2);
	}

	if (m->deletingtemporaryfiles) {
		SwitchFont(m->OCRAExt);
		PlaceString(m, 30, "-- Maintenance --", 33, 50, 0xFFFFFF);
		PlaceString(m, 50, "Deleting temporary files", 33, 100, 0xFFFFFF);
	}

	// move indicator for draw text
	if(m->drawtext_access_dialog_hwnd) {
		// from image to screen
		int k =  m->mscaler*std::clamp(m->locationXtextvar, 0, m->imgwidth)+m->CoordLeft;
		int v = m->mscaler*std::clamp(m->locationYtextvar, 0, m->imgheight)+m->CoordTop;
		DrawMenuIcon(m, k-5, v-5, 0, 26, 255);
	}

	// debug mode
	SwitchFont(m->SegoeUI);
	if (m->debugmode) {
		char debug[256];
		sprintf(debug, "WASDX: %f: MS: %f: RES: %f: Undo Queue: %d:Undo Step: %d: LD: %d | RD: %d | ", m->wasdX, m->ms_time, m->a_resolution, m->ProcessOfMakingUndoStep, m->undoStep, m->Leftdown, m->Rightdown);
		PlaceString(m, 16, debug, 12, m->toolheight + 8, 0x808080);
	}

	if(m->draw_updates_debug) {
		uint32_t testc = 0xFF00FF;
		if(rand()%2==0) {
			testc = 0x00FF00;
		}
		dDrawFilledRectangle(m, 0, 0, m->width, m->height, testc, 0.5f);
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