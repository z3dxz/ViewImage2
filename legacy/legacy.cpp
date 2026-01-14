/*

int ActuallyPlaceString_Old(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, int bufwidth, int bufheight, void* fromBuffer) {
	if (!*currentFace) {
		return 0;
	}

	FT_Set_Pixel_Sizes(*currentFace, 0, size);
	
	if (bufwidth <= 0) return 0;
	if(bufheight <= 0) return 0;

	const char* text = inputstr;

	FT_GlyphSlot g = (*currentFace)->glyph;
	int penX = 0;
	int spacing = 0; // Variable to track spacing between characters

	// Calculate the base Y-position using ascender value
	int baseY = locY + ((*currentFace)->size->metrics.ascender >> 6);

	for (const char* p = text; *p; ++p) {
		if (FT_Load_Char((*currentFace), *p, FT_LOAD_RENDER))
			continue;

		FT_Bitmap* bitmap = &g->bitmap;

		int maxGlyphHeight = 0;
		int maxDescender = 0;
		
		int yOffset = (g->metrics.horiBearingY - g->bitmap_top) >> 6;

		if (g->bitmap.rows > maxGlyphHeight)
			maxGlyphHeight = g->bitmap.rows;

		int descender = (g->metrics.height >> 6) - yOffset;
		if (descender > maxDescender)
			maxDescender = descender;

		for (int y = 0; y < bitmap->rows; ++y) {
			for (int x = 0; x < bitmap->width; ++x) {
				int pixelIndex = (y)*bufwidth + (penX + x);
				unsigned char pixelValue = bitmap->buffer[y * bitmap->width + x];
				uint32_t ptx = locX + penX + x;
				uint32_t pty = baseY - (bitmap->rows - y) + maxDescender; // Adjusted Y-position calculation

				uint32_t* memoryPath = GetMemoryLocation(mem, ptx, pty, bufwidth, bufheight);
				uint32_t* memoryPathFrom = GetMemoryLocation(fromBuffer, ptx, pty, bufwidth, bufheight);
				uint32_t existingColor = *memoryPathFrom;
				if (ptx >= 0 && pty >= 0 && ptx < bufwidth && pty < bufheight) {
					*GetMemoryLocation(mem, ptx, pty, bufwidth, bufheight) = lerp(existingColor, color, ((float)pixelValue / 255.0f));
				}
			}
		}

		// Calculate actual character width
		int characterWidth = (g->metrics.horiAdvance >> 6) - (g->metrics.horiBearingX >> 6);

		// Update penX position with character width and spacing
		penX += characterWidth + spacing;
	}

	return 1;
}



POINT* sampleLine_old(double x1, double y1, double x2, double y2, int numSamples) {


	POINT* samples = (POINT*)malloc(sizeof(POINT) * numSamples);

	for (int i = 0; i < numSamples; i++) {
		double t = (double)i / (numSamples - 1);
		samples[i].x = x1 + t * (x2 - x1);
		samples[i].y = y1 + t * (y2 - y1);
	}

	// remember to deallocate
	return samples;
}

const char* strtable[] {
	"Open Image (F)",
	"Save as PNG (CTRL+S)",
	"Zoom In",
	"Zoom Out",
	"Zoom Auto",
	"Zoom 1:1 (100%)",
	"Rotate",
	"Annotate (G)",
	"DELETE image",
	"Print",
	"Copy Image",
	"Information"
};


		//CircleGenerator(m->drawSize * m->mscaler-2, p.x, p.y, 5, 4, (uint32_t*)m->scrdata, 0xFFFFFF, 0.5f, m->width, m->height);
	
	//InitFont(m->hwnd, "segoeui.TTF", 14); // why did I even put this here? it just causes a memory leak and does nothing
	
	
	uint32_t randc = ((uint32_t)rand() << 16) | (uint32_t)rand();;
		for (int y = 0; y < m->height; y++) {
			for (int x = 0; x < m->width; x++) {
				uint32_t* memloc = GetMemoryLocation(m->scrdata, x, y, m->width, m->height);
				*memloc = lerp(*memloc, randc, 0.5f);
			}
		}

*/


	// END OF TEMP FILES


	// load pathway image
	
		// turn arguments into path
		/*
		
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, NULL, 0, NULL, NULL);

		
		char* path = (char*)malloc(size_needed);
		
		WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, path, size_needed, NULL, NULL);
		
		if (argc >= 2) {
			if (!OpenImageFromPath(m, path, false)) {
				MessageBox(m->hwnd, "Unable to open image", "Error", MB_OK | MB_ICONERROR);
				exit(0);
				return false;
			}
		}
		else {
			RedrawSurface(m);
		}
		*/


		/*
		// fun but impractical
		
					// sample points for contrast, something fun
					uint32_t color1 = *GetMemoryLocation(m->scrdata, 276, 95, m->width, m->height);
					uint32_t color2 = *GetMemoryLocation(m->scrdata, 379, 116, m->width, m->height);
					uint32_t color3 = *GetMemoryLocation(m->scrdata, 287, 148, m->width, m->height);
					int lum1 = (((color1>>16)&0xFF) + ((color1>>8)&0xFF) + (color1&0xFF))/3;
					int lum2 = (((color2>>16)&0xFF) + ((color2>>8)&0xFF) + (color2&0xFF))/3;
					int lum3 = (((color3>>16)&0xFF) + ((color3>>8)&0xFF) + (color3&0xFF))/3;

					int lumc1 = lum1; int lumc2 = lum2; int lumc3 = lum3;

					int max = max(max(lum1, lum2), lum3);
					int min = min(min(lum1, lum2), lum3);
					int adv = (max+min)/2;
					if(adv<127) {
						int diff = 255-max;
						lumc1+=diff;
						lumc2+=diff;
						lumc3+=diff;
					} else {
						int diff = min;
						lumc1-=diff;
						lumc2-=diff;
						lumc3-=diff;
					}

					int lum0 = (lumc1+lumc2+lumc3)/3;

					// curved
					int lum = round((float)(2.0f*(float)lum0-127.5f-(2.0f/255.0f)*((float)lum0-127.5f)*abs((float)lum0-127.5f)));

					// absolutely
					//int lum = -4.0f*(float)abs((float)lum0-63.5)+255.0f;
					//if((float)lum0 > 127.5f) {
					//	lum = 4.0f*(float)abs((float)lum0-191);
					//}

					uint32_t dcolor = change_alpha(RGB(lum, lum, lum), 255);
					uint32_t shadow = change_alpha(RGB(255-lum, 255-lum, 255-lum), 255);
		*/

		/*

		
/*

void parseCommandLine(const std::string& cmdLine, std::string& firstArg, std::string& secondArg, std::string& thirdArg) {
	std::istringstream stream(cmdLine);
	std::string token;
	std::vector<std::string> args;
	bool inQuotes = false;

	while (stream >> std::ws) {
		if (stream.peek() == '"') {
			stream.ignore();
			std::getline(stream, token, '"');
		}
		else {
			stream >> token;
		}
		args.push_back(token);
	}

	// Skip the application name
	if (args.size() > 1) firstArg = args[1];
	if (args.size() > 2) secondArg = args[2];
	if (args.size() > 3) thirdArg = args[3];

	if (args.size() <= 1) firstArg = "";
	if (args.size() <= 2) secondArg = "";
	if (args.size() <= 3) thirdArg = "";
}
*/


/*

bool CopyImageToClipboard(GlobalParams* m, void* imageData, int width, int height){
	Beep(3000, 40);

	// Initialize COM for clipboard operations
	if (FAILED(OleInitialize(NULL)))
		return false;

	// Create a device context for the screen
	HDC screenDC = GetDC(NULL);
	HDC memDC = CreateCompatibleDC(screenDC);
	ReleaseDC(NULL, screenDC);

	// Create a bitmap and select it into the device context
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height; // negative height for top-down DIB
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0; // Set to 0 for BI_RGB
	HBITMAP hBitmap = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);

	if (hBitmap == NULL) {
		DeleteDC(memDC);
		OleUninitialize();
		return false;
	}

	// Copy the image data to the bitmap
	SetDIBits(memDC, hBitmap, 0, height, imageData, &bmi, DIB_RGB_COLORS);

	// Select the bitmap into the device context
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);

	// Open the clipboard
	if (!OpenClipboard(NULL)) {
		DeleteObject(hBitmap);
		DeleteDC(memDC);
		OleUninitialize();
		return false;
	}

	// Empty the clipboard
	EmptyClipboard();
	
	// Set PNG format
	HANDLE hDIB = NULL;
	{
		DWORD dwBmpSize = ((width * 32 + 31) / 32) * 4 * height; // Calculate size of image buffer (DWORD aligned)
		hDIB = GlobalAlloc(GHND, sizeof(BITMAPINFOHEADER) + dwBmpSize);
		if (hDIB != NULL) {
			LPVOID pv = GlobalLock(hDIB);
			if (pv != NULL) {
				BITMAPINFOHEADER* pbmi = (BITMAPINFOHEADER*)pv;
				pbmi->biSize = sizeof(BITMAPINFOHEADER);
				pbmi->biWidth = width;
				pbmi->biHeight = -height; // Corrected height for bottom-up DIB
				pbmi->biPlanes = 1;
				pbmi->biBitCount = 32;
				pbmi->biCompression = BI_RGB;
				pbmi->biSizeImage = dwBmpSize;

				BYTE* pData = (BYTE*)pbmi + sizeof(BITMAPINFOHEADER);
				memcpy(pData, imageData, dwBmpSize);
				for (int y = 0; y < height; y++) {
					for (int x = 0; x < width; x++) {
						*GetMemoryLocation(pData, x, y, width, height) = *GetMemoryLocation(pData, x, y, width, height);
					}
				}
				ConvertToPremultipliedAlpha((uint32_t*)pData, width, height);
				GlobalUnlock(hDIB);
			}
		}
		
	}
	
	SetClipboardData(CF_DIB, hDIB); // CF_DIBV5 may not be supported on all systems

	// Clean up
	CloseClipboard();
	SelectObject(memDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(memDC);
	OleUninitialize();

	return true;
}

*/
		
// for windows xp, not used anymore due to layered window discovery
/*
void RedrawSurfaceTextDialog(GlobalParams* m) {
	RedrawSurface(m);
	return;
	// windows xp / non-dwm clipping to compensate for draw text menu
	if(m->drawtext_access_dialog_hwnd) {
		// hardcoded for now
		 int dtsx = 417;
		 int dtsy = 67;

		int X0 = 0;
		int X1 = m->drawtext_guiLocX;
		int X2 = m->drawtext_guiLocX+dtsx;
		int X3 = m->width;

		int Y0 = 0;
		int Y1 = m->drawtext_guiLocY;
		int Y2 = m->drawtext_guiLocY+dtsy;
		int Y3 = m->height;

		HRGN regionL = CreateRectRgn(X0, Y0, X1, Y3);
		HRGN regionR = CreateRectRgn(X2, Y0, X3, Y3);

		HRGN regionT = CreateRectRgn(X1, Y0, X2, Y1);
		HRGN regionB = CreateRectRgn(X1, Y2, X2, Y3);

		SelectClipRgn(m->hdc, regionT); RedrawSurface(m, false, true);
		SelectClipRgn(m->hdc, regionB); RedrawSurface(m, false, true);
		SelectClipRgn(m->hdc, regionL); RedrawSurface(m, false, true);
		SelectClipRgn(m->hdc, regionR); RedrawSurface(m, false, true);

	}

}
*/

/*
// Old Image Image BI
void PlaceImageBI(GlobalParams* m, void* memory, bool invert, POINT p, bool clip, RECT region) {

	if (followsPattern(m->mscaler * 100)) {
		PlaceImageNN(m, memory, invert, p, clip, region);
		return;
	}

	const int margin = m->fullscreen ? 0 : 2;
	const float inv_mscaler = 1.0f / m->mscaler;
	const int32_t imgwidth_minus_1 = m->imgwidth - 1;
	const int32_t imgheight_minus_1 = m->imgheight - 1;

	float offX = ((float)m->width  - ((float)m->imgwidth  * m->mscaler)) / 2.0f + m->iLocX;
	float offY = ((float)m->height - ((float)m->imgheight * m->mscaler)) / 2.0f + m->iLocY;

	std::for_each(std::execution::par, m->itv.begin(), m->itv.end(), [&](uint32_t y) {
		
		float pty = ((float)y - offY) * inv_mscaler - 0.5f;
		
		for (uint32_t x : m->ith) {

			if((!(x >= region.left && x <= region.right && y >= region.top && y <= region.bottom)) && clip) {
				continue;
			}

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

			float ptx = ((float)x - offX) * inv_mscaler - 0.5f;

			uint32_t doColor = bkc;

			if (ptx < imgwidth_minus_1 && pty < imgheight_minus_1 && ptx >= 0 && pty >= 0 && x >= margin && y >= margin && y < m->height - margin && x < m->width - margin) {
				int32_t ptx_int = static_cast<int32_t>(ptx);
				int32_t pty_int = static_cast<int32_t>(pty);
				int ptx_frac = (int)((ptx - ptx_int)*255.0f);
				int pty_frac = (int)((pty - pty_int)*255.0f);

				// Get the four nearest pixels
				
				uint32_t c00 = *GetMemoryLocationRaw(memory, ptx_int, pty_int, m->imgwidth, m->imgheight);
				uint32_t c01 = *GetMemoryLocationRaw(memory, ptx_int + 1, pty_int, m->imgwidth, m->imgheight);
				uint32_t c10 = *GetMemoryLocationRaw(memory, ptx_int, pty_int + 1, m->imgwidth, m->imgheight);
				uint32_t c11 = *GetMemoryLocationRaw(memory, ptx_int + 1, pty_int + 1, m->imgwidth, m->imgheight);
				
				if (((c00 >> 24) & 0xFF) != 0 || ((c01 >> 24) & 0xFF) != 0 || ((c10 >> 24) & 0xFF) != 0 || ((c11 >> 24) & 0xFF) != 0) {
					uint32_t placeColor = lerp_u32(lerp_u32(c00, c01, ptx_frac), lerp_u32(c10, c11, ptx_frac), pty_frac);

					int alpha = (placeColor >> 24) & 255;
					if (alpha == 255) {
						doColor = placeColor;
					}
					else if (alpha == 0) {
						// still bkc
					}
					else {
						doColor = lerp_u32(bkc, placeColor, (alpha));
					}
				}
			}
			*GetMemoryLocationRaw(m->scrdata, x, y, m->width, m->height) = doColor;
		}
	});
}

void PlaceImageNN(GlobalParams* m, void* memory, bool invert, POINT p, bool clip, RECT region) {

	const float inv_mscaler = 1.0f / m->mscaler;
	const int32_t imgwidth = m->imgwidth;
	const int32_t imgheight = m->imgheight;
	constexpr int margin = 2;

	std::for_each(std::execution::par, m->itv.begin(), m->itv.end(), [&](uint32_t y) {

		for (uint32_t x : m->ith) {

			if((!(x >= region.left && x <= region.right && y >= region.top && y <= region.bottom)) && clip) {
				continue;
			}

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
				uint32_t c = *GetMemoryLocationRaw(memory, ptx, pty, imgwidth, imgheight);

				int alpha = (c >> 24) & 255;
				if (alpha == 255) {
					doColor = c;
				}
				else if (alpha == 0) {
					// still bkc
				}
				else {
					doColor = lerp_u32(bkc, c, static_cast<float>(alpha));
				}
			}
			*GetMemoryLocationRaw(m->scrdata, x, y, m->width, m->height) = doColor;
		}
		});
}
*/