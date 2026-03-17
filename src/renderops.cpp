#include <windows.h>
#include <iostream>
#include <string>
#include "headers/renderops.hpp"

void CircleGenerator(GlobalParams* m, int circleDiameter, int locX, int locY, uint32_t color, bool onlyUnderToolbar) {
	float radius = circleDiameter/2.0f;
	float step = 1.0f/circleDiameter;
	for(float i=0; i<2.0f*3.141592f; i+= step) {
		float locationX = radius*cos(i)+locX;
		float locationY = radius*sin(i)+locY;

		int ptX = (int)locationX;
		int ptY = (int)locationY;

		if(ptX >= 0 && ptX < m->width && ptY >= 0 && ptY < m->height && ((ptY>m->toolheight) || !onlyUnderToolbar) && ptY >= 0) {
			*GetMemoryLocation(m->scrdata, ptX, ptY, m->width, m->height) = color;
		}
	}
}

int PlaceString(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color) {
	bool e = opsPlaceStringBuffer(m, size, inputstr, locX, locY, color, m->scrdata, m->width, m->height, m->scrdata);
	return e;
}

int PlaceStringLegacyShadow(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, int shadowY, int shadowX = 0) {
	bool b = opsPlaceStringBuffer(m, size, inputstr, locX+shadowX, locY + shadowY, 0x000000, mem, m->width, m->height, mem);
	bool e = opsPlaceStringBuffer(m, size, inputstr, locX, locY, color, mem, m->width, m->height, mem);

	return e && b;
}

int PlaceStringShadow(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, float sigma, int shadowOffsetX, int shadowOffsetY, int passes, uint32_t shadowColor) {
	bool b = opsPlaceStringShadowObject(m, size, inputstr, locX+shadowOffsetX, locY+shadowOffsetY, shadowColor, m->scrdata, sigma, passes);
	bool e = opsPlaceStringBuffer(m, size, inputstr, locX, locY, color, m->scrdata, m->width, m->height, m->scrdata);

	return e && b;
}

void drawLine(GlobalParams* m, int startX, int startY, int len, bool horizontal, uint32_t color, float opacity) {
    int endX = startX + (horizontal ? len - 1 : 0);
    int endY = startY + (horizontal ? 0 : len - 1);

    int clipX1 = (startX < 0) ? 0 : startX;
    int clipY1 = (startY < 0) ? 0 : startY;
    int clipX2 = (endX >= m->width) ? m->width - 1 : endX;
    int clipY2 = (endY >= m->height) ? m->height - 1 : endY;

    if (clipX1 > clipX2 || clipY1 > clipY2) return;

    int alpha = (int)(opacity * 255.0f);
    uint32_t* ma = GetMemoryLocation(m->scrdata, clipX1, clipY1, m->width, m->height);
    
    int count = horizontal ? (clipX2 - clipX1 + 1) : (clipY2 - clipY1 + 1);
    int step = horizontal ? 1 : m->width;

    if (opacity < 0.99f) {
        for (int i = 0; i < count; i++) {
            *ma = lerp_u32(*ma, color, alpha);
            ma += step;
        }
    } else {
        for (int i = 0; i < count; i++) {
            *ma = color;
            ma += step;
        }
    }
}

void dDrawFilledRectangle(GlobalParams* m, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
    int x1 = (xloc < 0) ? 0 : xloc;
    int y1 = (yloc < 0) ? 0 : yloc;
    int x2 = (xloc + width > m->width) ? m->width : xloc + width;
    int y2 = (yloc + height > m->height) ? m->height : yloc + height;

	// early exit
    if (x1 >= x2 || y1 >= y2) return;

    int alpha = (int)(opacity * 255.0f);
    int drawWidth = x2 - x1;

    if (opacity < 0.99f) {
        for (int y = y1; y < y2; y++) {
            uint32_t* ma = GetMemoryLocation(m->scrdata, x1, y, m->width, m->height);
            for (int x = 0; x < drawWidth; x++) {
                *ma = lerp_u32(*ma, color, alpha);
                ma++;
            }
        }
    } else {
        for (int y = y1; y < y2; y++) {
            uint32_t* ma = GetMemoryLocation(m->scrdata, x1, y, m->width, m->height);
            for (int x = 0; x < drawWidth; x++) {
                *ma = color;
                ma++;
            }
        }
    }
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

void PlaceFromAtlas(GlobalParams* m, void* source, int sourceWidth, int sourceHeight, int sourceX, int sourceY, int destX, int destY, int width, int height, uint32_t color_tint, float opacity) {
	// perform clipping
	if (sourceX < 0) { width += sourceX; destX -= sourceX; sourceX = 0; }
    if (sourceY < 0) { height += sourceY; destY -= sourceY; sourceY = 0; }
    if (sourceX + width > sourceWidth) width = sourceWidth - sourceX;
    if (sourceY + height > sourceHeight) height = sourceHeight - sourceY;

    if (destX < 0) { width += destX; sourceX -= destX; destX = 0; }
    if (destY < 0) { height += destY; sourceY -= destY; destY = 0; }
    if (destX + width > m->width) width = m->width - destX;
    if (destY + height > m->height) height = m->height - destY;

	for(int y=0; y<height; y++) {
		for(int x=0; x<width; x++) {
			uint32_t* psrc = GetMemoryLocation(source, sourceX+x, sourceY+y, sourceWidth, sourceHeight);
			uint32_t* pdest = GetMemoryLocation(m->scrdata, destX+x, destY+y, m->width, destHeight);

			uint32_t srcc = multiplyColors(*psrc, color_tint);

			*pdest = lerp_u32(*pdest, change_alpha(srcc, 255), (((*psrc) >> 24) & 0xFF)*opacity);
		}
	}
}

void dDrawGradient(GlobalParams* m, int x, int y, int w, int h, GradientDirection direction, uint32_t color1, uint32_t color2){
	// Get RGB of r1 and g2
    uint8_t r1 = (color1 >> 16) & 0xFF, g1 = (color1 >> 8) & 0xFF, b1 = color1 & 0xFF;
    uint8_t r2 = (color2 >> 16) & 0xFF, g2 = (color2 >> 8) & 0xFF, b2 = color2 & 0xFF;

	for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            uint32_t t = (direction == GradientLeftRight) ? (uint32_t)((i * 255) / (w - 1)) : (uint32_t)((j * 255) / (h - 1));
            uint32_t fcolor = lerp_u32(color1, color2, t);
            uint32_t alpha = (fcolor >> 24) & 0xFF;

            uint32_t* data = GetMemoryLocation(m->scrdata, x + i, y + j, m->width, m->height);
			
            *data = lerp_u32(*data, fcolor, alpha);
        }
    }
}


void PlaceImageNN(GlobalParams* m, int rendertoolbar, void* memory, bool invert, POINT p, bool clip, RECT region) {
    const int margin = m->fullscreen ? 0 : 2;
    const int32_t imgW = m->imgwidth;
    const int32_t imgH = m->imgheight;
    const int32_t dstW = m->width;
    const int32_t dstH = m->height;

    const float invScale = 1.0f / m->mscaler;
    const float offX = ((float)dstW - imgW * m->mscaler) / 2.0f + m->iLocX;
    const float offY = ((float)dstH - imgH * m->mscaler) / 2.0f + m->iLocY;

    uint32_t* src = static_cast<uint32_t*>(memory);
    uint32_t* dst = static_cast<uint32_t*>(m->scrdata);

    const uint32_t bkc1 = 0x121212;
    const uint32_t bkc2 = 0x181818;
    const uint32_t toolbarBkc = m->aeromode ? 0x00000000 : 0xFF151515;

    std::for_each(std::execution::par, m->itv.begin(), m->itv.end(), [&](uint32_t y) {
        uint32_t* dstRow = dst + y * dstW;
        float pty_f = ((float)y - offY) * invScale;
        int32_t iy = static_cast<int32_t>(std::floor(pty_f));

        bool y_toolbar = (y <= m->toolheight) && rendertoolbar;

        bool y_out_of_img = (iy < 0 || iy >= imgH) || (y < margin || y >= dstH - margin);

        for (uint32_t x : m->ith) {
            if (clip && (x < region.left || x > region.right || y < region.top || y > region.bottom))
                continue;

            if (x < margin || x >= dstW - margin || y_out_of_img) {
                dstRow[x] = y_toolbar ? toolbarBkc : (((x / 9 + y / 9) & 1) ? bkc1 : bkc2);
                continue;
            }

            float ptx_f = ((float)x - offX) * invScale;
            int32_t ix = static_cast<int32_t>(std::floor(ptx_f));

            if (ix < 0 || ix >= imgW) {
                dstRow[x] = y_toolbar ? toolbarBkc : (((x / 9 + y / 9) & 1) ? bkc1 : bkc2);
                continue;
            }
			uint32_t col = 0;
			if(memory) col = src[iy * imgW + ix];
            uint32_t alpha = col >> 24;

            uint32_t bkc = y_toolbar ? toolbarBkc : (((x / 9 + y / 9) & 1) ? bkc1 : bkc2);

            if (alpha == 255 && memory) {
                dstRow[x] = col;
            } else if (alpha == 0 || (!memory)) {
                dstRow[x] = bkc;
            } else {
                dstRow[x] = lerp_u32(bkc, col, static_cast<uint8_t>(alpha));
            }
        }
    });
}


bool followsPattern(int number) {
	double logBase2 = log2(number / 100.0);
	return logBase2 == floor(logBase2);
}

void PlaceImageBI(GlobalParams* m, int rendertoolbar, void* memory, bool invert, POINT p, bool clip, RECT region) {
    if (followsPattern(m->mscaler * 100)) {
        PlaceImageNN(m, rendertoolbar, memory, invert, p, clip, region);
        return;
    }

    const int margin = m->fullscreen ? 0 : 2;
    const int32_t imgW = m->imgwidth;
    const int32_t imgH = m->imgheight;
    const int32_t imgWm1 = imgW - 1;
    const int32_t imgHm1 = imgH - 1;
    const int32_t dstW = m->width;
    const int32_t dstH = m->height;

    const float invScale = 1.0f / m->mscaler;
    const float offX = ((float)dstW - imgW * m->mscaler) / 2.0f + m->iLocX;
    const float offY = ((float)dstH - imgH * m->mscaler) / 2.0f + m->iLocY;

    uint32_t* src = static_cast<uint32_t*>(memory);
    uint32_t* dst = static_cast<uint32_t*>(m->scrdata);
	
    const uint32_t bkc1 = 0x121212;
    const uint32_t bkc2 = 0x181818;
    const uint32_t toolbarBkc = m->aeromode ? 0x00000000 : 0xFF151515;

    std::for_each(std::execution::par, m->itv.begin(), m->itv.end(), [&](uint32_t y) {

        uint32_t* dstRow = dst + y * dstW;
        float pty_f = ((float)y - offY) * invScale - 0.5f;

        int32_t iy = static_cast<int32_t>(std::floor(pty_f));
        float fy = pty_f - iy;

        // Early row rejection
        if (iy < 0 || iy >= imgHm1 || y < margin || y >= dstH - margin) {
            for (uint32_t x : m->ith) {
                uint32_t bkc = (y > m->toolheight || !(rendertoolbar)) ? (((x / 9 + y / 9) & 1) ? bkc1 : bkc2) : toolbarBkc;
                dstRow[x] = bkc;
            }
            return;
        }

        uint32_t* srcRow0 = src + iy * imgW;
        uint32_t* srcRow1 = srcRow0 + imgW;

        for (uint32_t x : m->ith) {

            if (clip &&  (x < region.left || x > region.right || y < region.top || y > region.bottom))
                continue;

            if (x < margin || x >= dstW - margin) {
                uint32_t bkc = (y > m->toolheight || !(rendertoolbar)) ? (((x / 9 + y / 9) & 1) ? bkc1 : bkc2) : toolbarBkc;
                dstRow[x] = bkc;
                continue;
            }

            float ptx_f = ((float)x - offX) * invScale - 0.5f;
            int32_t ix = static_cast<int32_t>(std::floor(ptx_f));
            float fx = ptx_f - ix;

            if (ix < 0 || ix >= imgWm1) {
                uint32_t bkc = (y > m->toolheight || !(rendertoolbar)) ? (((x / 9 + y / 9) & 1) ? bkc1 : bkc2) : toolbarBkc;
                dstRow[x] = bkc;
                continue;
            }

            // Load 4 pixels for bilinear
            uint32_t c00 = srcRow0[ix];
            uint32_t c01 = srcRow0[ix + 1];
            uint32_t c10 = srcRow1[ix];
            uint32_t c11 = srcRow1[ix + 1];

            uint32_t maxAlpha = (c00 >> 24 | c01 >> 24 | c10 >> 24 | c11 >> 24);
            if (maxAlpha == 0) {
                uint32_t bkc = (y > m->toolheight || !(rendertoolbar)) ? (((x / 9 + y / 9) & 1) ? bkc1 : bkc2) : toolbarBkc;
                dstRow[x] = bkc;
                continue;
            }

            uint8_t fx8 = static_cast<uint8_t>(fx * 255.0f + 0.5f);
            uint8_t fy8 = static_cast<uint8_t>(fy * 255.0f + 0.5f);

            uint32_t colX0 = lerp_u32(c00, c01, fx8);
            uint32_t colX1 = lerp_u32(c10, c11, fx8);
            uint32_t col = lerp_u32(colX0, colX1, fy8);

            uint32_t alpha = (col >> 24) & 0xFF;
            uint32_t bkc = (y > m->toolheight || !(rendertoolbar)) ? (((x / 9 + y / 9) & 1) ? bkc1 : bkc2) : toolbarBkc;

            if (alpha == 255) {
                dstRow[x] = col;
            } else if (alpha == 0) {
                dstRow[x] = bkc;
            } else {
                dstRow[x] = lerp_u32(bkc, col, static_cast<uint8_t>(alpha));
            }
        }
    });
}

void blur_toolbar(GlobalParams* m) {
	boxBlur(m, 15, 1, 0, m->toolheight);
}

void gaussian_blur(GlobalParams* m, int lW, int lH, double sigma, uint32_t offX, uint32_t offY) {
	gaussian_blur_real((uint32_t*)m->scrdata, (uint32_t*)m->scrdata, lW, lH, sigma, m->width, m->height, offX, offY);
}

// box blur

uint32_t* eintegralR=0;
uint32_t* eintegralG=0;
uint32_t* eintegralB=0;

uint32_t eAssignedW = 0;
uint32_t eAssignedH = 0;
uint32_t* eAssignedMEM = 0;

uint32_t* fintegralR = 0;
uint32_t* fintegralG = 0;
uint32_t* fintegralB = 0;

uint32_t fAssignedW = 0;
uint32_t fAssignedH = 0;
uint32_t* fAssignedMEM = 0;

bool lastmode;

void boxBlur(GlobalParams* m, uint32_t kernelSize, int mode, int startOffset, int vsize) {

	int width = m->width;
	int height = vsize;
	uint32_t* memory = (uint32_t*)m->scrdata+(startOffset*width);

	uint32_t halfKernel = kernelSize / 2;

	// This entire thing is an optimization for keeping the two buffers on two different slots
	// Compute integral image
	uint32_t* integralR;
	uint32_t* integralG;
	uint32_t* integralB;
	if (mode == 1) {
		if (!eintegralR) {
			// init
			eintegralR = (uint32_t*)malloc(width * height * sizeof(uint32_t));
			eintegralG = (uint32_t*)malloc(width * height * sizeof(uint32_t));
			eintegralB = (uint32_t*)malloc(width * height * sizeof(uint32_t));
		}
		if (eAssignedH != height || eAssignedW != width || eAssignedMEM != memory) {
			// refresh
			FreeData(eintegralR); FreeData(eintegralG); FreeData(eintegralB);
			eintegralR = (uint32_t*)malloc(width * height * sizeof(uint32_t));
			eintegralG = (uint32_t*)malloc(width * height * sizeof(uint32_t));
			eintegralB = (uint32_t*)malloc(width * height * sizeof(uint32_t));
		}
		eAssignedW = width;
		eAssignedH = height;
		eAssignedMEM = (uint32_t*)memory;

		integralR = eintegralR;
		integralG = eintegralG;
		integralB = eintegralB;
	}
	else {
		if (!fintegralR) {
			// init
			fintegralR = (uint32_t*)malloc(width * height * sizeof(uint32_t));
			fintegralG = (uint32_t*)malloc(width * height * sizeof(uint32_t));
			fintegralB = (uint32_t*)malloc(width * height * sizeof(uint32_t));
		}
		if (fAssignedH != height || fAssignedW != width || fAssignedMEM != memory) {
			// refresh
			FreeData(fintegralR); FreeData(fintegralG); FreeData(fintegralB);
			fintegralR = (uint32_t*)malloc(width * height * sizeof(uint32_t));
			fintegralG = (uint32_t*)malloc(width * height * sizeof(uint32_t));
			fintegralB = (uint32_t*)malloc(width * height * sizeof(uint32_t));
		}
		fAssignedW = width;
		fAssignedH = height;
		fAssignedMEM = (uint32_t*)memory;

		integralR = fintegralR;
		integralG = fintegralG;
		integralB = fintegralB;
	}

	for (uint32_t y = 0; y < height; ++y) {
		for (uint32_t x = 0; x < width; ++x) {
			uint32_t pixel = ((uint32_t*)memory)[y * width + x];
			uint32_t r = (pixel >> 16) & 0xFF;
			uint32_t g = (pixel >> 8) & 0xFF;
			uint32_t b = pixel & 0xFF;

			uint32_t sumR = r;
			uint32_t sumG = g;
			uint32_t sumB = b;

			if (x > 0) {
				sumR += integralR[y * width + x - 1];
				sumG += integralG[y * width + x - 1];
				sumB += integralB[y * width + x - 1];
			}
			if (y > 0) {
				sumR += integralR[(y - 1) * width + x];
				sumG += integralG[(y - 1) * width + x];
				sumB += integralB[(y - 1) * width + x];
			}
			if (x > 0 && y > 0) {
				sumR -= integralR[(y - 1) * width + x - 1];
				sumG -= integralG[(y - 1) * width + x - 1];
				sumB -= integralB[(y - 1) * width + x - 1];
			}

			integralR[y * width + x] = sumR;
			integralG[y * width + x] = sumG;
			integralB[y * width + x] = sumB;
		}
	}

	for (uint32_t y = 0; y < height; ++y) {
		for (uint32_t x = 0; x < width; ++x) {
			uint32_t startX = x >= halfKernel ? x - halfKernel : 0;
			uint32_t startY = y >= halfKernel ? y - halfKernel : 0;
			uint32_t endX = x + halfKernel < width ? x + halfKernel : width - 1;
			uint32_t endY = y + halfKernel < height ? y + halfKernel : height - 1;

			uint32_t count = (endX - startX + 1) * (endY - startY + 1);
			uint32_t sumR = integralR[endY * width + endX];
			uint32_t sumG = integralG[endY * width + endX];
			uint32_t sumB = integralB[endY * width + endX];

			if (startX > 0) {
				sumR -= integralR[endY * width + startX - 1];
				sumG -= integralG[endY * width + startX - 1];
				sumB -= integralB[endY * width + startX - 1];
			}
            
			if (startY > 0) {
				sumR -= integralR[startY * width + endX];
				sumG -= integralG[startY * width + endX];
				sumB -= integralB[startY * width + endX];
			}
			if (startX > 0 && startY > 0) {
				sumR += integralR[startY * width + startX - 1];
				sumG += integralG[startY * width + startX - 1];
				sumB += integralB[startY * width + startX - 1];
			}

			uint32_t avgR = sumR / count;
			uint32_t avgG = sumG / count;
			uint32_t avgB = sumB / count;

			memory[y * width + x] = (avgR << 16) | (avgG << 8) | avgB;
		}
	}
}