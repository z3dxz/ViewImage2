#include <windows.h>
#include <vector>
#include <cstdint>
#include "vendor/stb_image_write.h"
#include "headers/ops.hpp"

static void PNGWriteCallback(void* context, void* data, int size)
{
    auto* buf = reinterpret_cast<std::vector<uint8_t>*>(context);
    buf->insert(buf->end(), (uint8_t*)data, (uint8_t*)data + size);
}

bool CopyImageToClipboard(GlobalParams* m, void* imageData, int width, int height)
{
    Beep(3000, 40);

    if (!imageData || width <= 0 || height <= 0)
        return false;

    uint8_t* src = (uint8_t*)imageData;
    const int pixelCount = width * height;

    // Build CF_DIBV5 (BGRA, bottom-up)
    const size_t dibSize =  sizeof(BITMAPV5HEADER) + pixelCount * 4;

    HGLOBAL hDIB = GlobalAlloc(GMEM_MOVEABLE, dibSize);
    if (!hDIB)
        return false;

    BITMAPV5HEADER* hdr = (BITMAPV5HEADER*)GlobalLock(hDIB);
    ZeroMemory(hdr, sizeof(*hdr));

    hdr->bV5Size = sizeof(BITMAPV5HEADER);
    hdr->bV5Width = width;
    hdr->bV5Height = height; // bottom-up REQUIRED
    hdr->bV5Planes = 1;
    hdr->bV5BitCount = 32;
    hdr->bV5Compression = BI_BITFIELDS;
    hdr->bV5RedMask   = 0x00FF0000;
    hdr->bV5GreenMask = 0x0000FF00;
    hdr->bV5BlueMask  = 0x000000FF;
    hdr->bV5AlphaMask = 0xFF000000;

    uint8_t* dst = (uint8_t*)(hdr + 1);

    for (int y = 0; y < height; ++y)
    {
        uint8_t* rowDst = dst + (height - 1 - y) * width * 4;
        uint8_t* rowSrc = src + y * width * 4;
        memcpy(rowDst, rowSrc, width * 4);
    }

    GlobalUnlock(hDIB);

    std::vector<uint8_t> rgba(pixelCount * 4);
    for (int i = 0; i < pixelCount; ++i)
    {
        rgba[i * 4 + 0] = src[i * 4 + 2]; // R
        rgba[i * 4 + 1] = src[i * 4 + 1]; // G
        rgba[i * 4 + 2] = src[i * 4 + 0]; // B
        rgba[i * 4 + 3] = src[i * 4 + 3]; // A
    }

    std::vector<uint8_t> pngData;
    if (!stbi_write_png_to_func(
            PNGWriteCallback, &pngData,
            width, height, 4,
            rgba.data(), width * 4))
    {
        GlobalFree(hDIB);
        return false;
    }

    HGLOBAL hPNG = GlobalAlloc(GMEM_MOVEABLE, pngData.size());
    void* pngDst = GlobalLock(hPNG);
    memcpy(pngDst, pngData.data(), pngData.size());
    GlobalUnlock(hPNG);

    if (!OpenClipboard(nullptr))
    {
        GlobalFree(hDIB);
        GlobalFree(hPNG);
        return false;
    }

    EmptyClipboard();

    SetClipboardData(CF_DIBV5, hDIB);
    UINT cfPNG = RegisterClipboardFormatW(L"PNG");
    SetClipboardData(cfPNG, hPNG);

    CloseClipboard();
    return true;
}

#include <optional>

std::optional<uint32_t*> GetImageFromClipboard(GlobalParams* m, int& width, int& height)
{
    width = height = 0;
    if (!OpenClipboard(nullptr))
        return nullptr;

    uint32_t* pixels = nullptr;

    // PNG (modern)
    UINT cfPNG = RegisterClipboardFormatW(L"PNG");
    if (IsClipboardFormatAvailable(cfPNG))
    {
        HGLOBAL hPNG = GetClipboardData(cfPNG);
        if (hPNG)
        {
            void* data = GlobalLock(hPNG);
            size_t size = GlobalSize(hPNG);

            int w, h, comp;
            uint8_t* rgba = stbi_load_from_memory((uint8_t*)data, (int)size, &w, &h, &comp, 4);
            GlobalUnlock(hPNG);

            if (rgba)
            {
                pixels = (uint32_t*)malloc(w * h * 4);
                for (int i = 0; i < w*h; ++i)
                {
                    uint8_t r = rgba[i*4+0];
                    uint8_t g = rgba[i*4+1];
                    uint8_t b = rgba[i*4+2];
                    uint8_t a = rgba[i*4+3];
                    pixels[i] = (a << 24) | (r << 16) | (g << 8) | b;
                }
                stbi_image_free(rgba);
                width = w;
                height = h;
                CloseClipboard();
                return pixels;
            }
        }
    }

    // CF_DIBV5 (modern with alpha)
    if (IsClipboardFormatAvailable(CF_DIBV5))
    {
        HGLOBAL hDIB = GetClipboardData(CF_DIBV5);
        if (hDIB)
        {
            BITMAPV5HEADER* hdr = (BITMAPV5HEADER*)GlobalLock(hDIB);
            if (hdr && hdr->bV5BitCount == 32)
            {
                int w = hdr->bV5Width;
                int h = abs(hdr->bV5Height);
                bool bottomUp = hdr->bV5Height > 0;
                uint8_t* src = (uint8_t*)(hdr + 1);

                pixels = (uint32_t*)malloc(w * h * 4);
                for (int y = 0; y < h; ++y)
                {
                    int sy = bottomUp ? (h-1-y) : y;
                    memcpy(pixels + y*w, src + sy*w*4, w*4);
                }

                GlobalUnlock(hDIB);
                width = w;
                height = h;
                CloseClipboard();
                return pixels;
            }
            if (hdr) GlobalUnlock(hDIB);
        }
    }

    // CF_DIB (legacy, 24-bit RGB)
    if (IsClipboardFormatAvailable(CF_DIB))
    {
        HGLOBAL hDIB = GetClipboardData(CF_DIB);
        if (hDIB)
        {
            BITMAPINFOHEADER* hdr = (BITMAPINFOHEADER*)GlobalLock(hDIB);
            if (hdr && hdr->biBitCount == 24)
            {
                int w = hdr->biWidth;
                int h = abs(hdr->biHeight);
                bool bottomUp = hdr->biHeight > 0;
                uint8_t* src = (uint8_t*)(hdr + 1);

                pixels = (uint32_t*)malloc(w * h * 4);
                for (int y = 0; y < h; ++y)
                {
                    int sy = bottomUp ? (h-1-y) : y;
                    for (int x = 0; x < w; ++x)
                    {
                        uint8_t b = src[(sy*w+x)*3+0];
                        uint8_t g = src[(sy*w+x)*3+1];
                        uint8_t r = src[(sy*w+x)*3+2];
                        pixels[y*w+x] = 0xFF000000 | (r<<16) | (g<<8) | b; // opaque alpha
                    }
                }

                GlobalUnlock(hDIB);
                width = w;
                height = h;
                CloseClipboard();
                return pixels;
            }
            if (hdr) GlobalUnlock(hDIB);
        }
    }

	// file paths
	if (IsClipboardFormatAvailable(CF_HDROP))
	{
		HDROP hDrop = (HDROP)GetClipboardData(CF_HDROP);
		if (hDrop)
		{
			UINT count = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
			if (count > 0)
			{
				// Only take the first file
				// this will load the image completely, not just the data
				char path[MAX_PATH];
				if (DragQueryFileA(hDrop, 0, path, MAX_PATH))
				{
					m->loading = true;
					RedrawSurface(m);
					OpenImageFromPath(m, path, false);
					m->loading = false;
					RedrawSurface(m);
					m->shouldSaveShutdown = false;
					CloseClipboard();
					return {};
				}
			}
		}
	}

    CloseClipboard();
    return nullptr;
}

// paste
bool PasteImageFromClipboard(GlobalParams* m) {

	if(!m->imgdata) {
		AllocateBlankImage(m, 0x00000000);
	} else {
		createUndoStep(m, false);
	}


	int w, h;
	std::optional<uint32_t*> d = GetImageFromClipboard(m, w, h);
	
	if(d.has_value()) {
		if (d.value()) {
			if (m->imgdata) {
				FreeData(m->imgdata);
			}
			Beep(4000, 40);

			if(m->imgoriginaldata) {
				FreeData(m->imgoriginaldata);
			}

			m->imgdata = d.value();
			m->imgwidth = w;
			m->imgheight = h;

			void* l = malloc(m->imgwidth*m->imgheight*4);
			memcpy(l, m->imgdata, m->imgwidth*m->imgheight*4);
			m->imgoriginaldata = l;

			autozoom(m);
		}
		else {
			Beep(300, 90);
		}
	} else {
		// already handled
		Beep(4000, 40);
	}
	

	return true;
}

