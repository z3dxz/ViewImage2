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