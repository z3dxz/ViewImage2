#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include "irbo_support.hpp"
#include <string.h>
#include "../vendor/stb_image.h"
#include <vector>
#include <cstdlib>
#include <cstring>
#include <random>

#include <cstdint>


int clampv2(int value, int min, int max) {
	if(value < min) { return min;}
	if(value > max) {return max;}
	return value;
}

static uint32_t round_func(uint32_t x, uint32_t key) {
    x ^= key;
    x *= 0x9E3779B1;   // large odd constant
    x ^= x >> 16;
    return x;
}

uint32_t permute(uint32_t i, uint32_t seed) {
    uint16_t L = i >> 16;
    uint16_t R = i & 0xFFFF;

    for (int r = 0; r < 4; ++r)
    {
        uint16_t F = round_func(R, seed + r) & 0xFFFF;
        uint16_t newL = R;
        uint16_t newR = L ^ F;
        L = newL;
        R = newR;
    }

    return (uint32_t(L) << 16) | R;
}

std::vector<uint32_t> build_permutation(uint32_t pixelCount, uint32_t seed)
{
    std::vector<uint32_t> p(pixelCount);
    for (uint32_t i = 0; i < pixelCount; ++i)
        p[i] = i;

    std::mt19937 rng(seed);
    std::shuffle(p.begin(), p.end(), rng);

    return p;
}

#define GetMemoryLocationK(start, x, y, widthfactor) \
	((uint32_t*)start + (y * widthfactor) + x)\
\

template <typename T>
T* GetMemoryLocationAny(T* start, uint32_t x, uint32_t y, uint32_t widthfactor) {
	return ((T*)start + (y * widthfactor) + x);
}

uint32_t twobytesize = 1024*512;
	uint32_t seed = 0;


const char* encodeirbo(const char* out_path, void* imgdata, int imgwidth, int imgheight, int channels) {
	uint32_t pixelcount = 512*512;

	std::vector<uint32_t> perm = build_permutation(pixelcount, seed);

	float aspect = (float)imgwidth/(float)imgheight;
	
	std::cout << "Aspect: " << aspect << "\n";

	void* temp_img = malloc(pixelcount*4);

	stbir_resize_uint8_linear((unsigned char*)imgdata, imgwidth, imgheight, 0, (unsigned char*)temp_img, 512, 512, 0, STBIR_RGBA);

	void* shuffled = malloc(pixelcount*4);

	for (size_t i = 0; i < pixelcount; ++i)
    {
		size_t j = perm[i];

        memcpy(
            (uint8_t*)shuffled + j * 4,
            (uint8_t*) temp_img + i * 4,
            4
        );
    }

	void* twobyte = malloc(twobytesize);

	for(int y=0; y<512; y++) {
		for(int x=0; x<512; x++) {

			uint32_t done = *GetMemoryLocationAny<uint32_t>((uint32_t*)shuffled, x, y, 512);

			uint8_t R = (done>>16)&0xFF;
			uint8_t G = (done>>8)&0xFF;
			uint8_t B = (done)&0xFF;

			uint8_t Y  = (  77 * R + 150 * G +  29 * B) >> 8;
			uint8_t Cb = ((-43 * R -  85 * G + 128 * B) >> 8) + 128;
			uint8_t Cr = ((128 * R - 107 * G -  21 * B) >> 8) + 128;
			

			int approx_cb = Cb/16;
			int approx_cr = Cr/16;

			int remainCB = ((float)(Cb%16)/16.0f)*100.0f;
			int remainCR = ((float)(Cr%16)/16.0f)*100.0f;
			

			approx_cb = lerp(approx_cb, approx_cb+1, rand()%100<remainCB);
			approx_cr = lerp(approx_cr, approx_cr+1, rand()%100<remainCR);

			if(approx_cb > 15) {approx_cb = 15;}
			if(approx_cr > 15) {approx_cr = 15;}
			if(approx_cb <= 0) {approx_cb = 0;}
			if(approx_cr <= 0) {approx_cr = 0;}

			//int approx_cb = (Cb >> 4) & 0xF;
			//int approx_cr  =  (Cr >> 4) & 0xF;
				
			uint8_t chroma = (approx_cb << 4) | approx_cr;

			*GetMemoryLocationAny<uint8_t>((uint8_t*)twobyte, x*2, y, 1024) = Y;
			*GetMemoryLocationAny<uint8_t>((uint8_t*)twobyte, x*2+1, y, 1024) = chroma;
		}
	}

	free(temp_img);

	// copy paste

	char str_path[256];
    strcpy(str_path, out_path);

    char* last_dot = strrchr(str_path, '.');
    if (last_dot != NULL) {
        *last_dot = '\0';
    }

    strcat(str_path, ".irbo");

    // write to a file
    HANDLE hFile = CreateFile(  str_path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return "sorry, no file handling";
    }
	
	void* data = malloc(twobytesize+4);
	memcpy((uint8_t*)data, &aspect, 4);
	memcpy((uint8_t*)data+4, twobyte, twobytesize);

	free(twobyte);

	size_t imgByteSize = twobytesize+4;

    // Write data to the file
    DWORD bytesWritten;
    WriteFile(
        hFile,            // Handle to the file
        data,  // Buffer to write
        imgByteSize,   // Buffer size
        &bytesWritten,    // Bytes written
        0);         // Overlapped

    // Close the handle once we don't need it.
    CloseHandle(hFile);

    FreeData(data);

    return "success";
}


void* decodeirbo(const char* filepath, int* imgwidth, int* imgheight){
	// copy paste
	printf("\n -- Reading File -- \n");
    HANDLE hFile = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("sorry, the handle value was invalid");
        return 0;
    }

    DWORD fsize = GetFileSize(hFile, 0);
    printf("File Size: %i\n", (int)fsize);
    
    int sizeOfAllocation = fsize;
    void* data = malloc(sizeOfAllocation);
    DWORD dwBytesRead = 0;
    DWORD dwBytesWritten = 0;

    if (!ReadFile(hFile, data, sizeOfAllocation, &dwBytesRead, NULL)) {
        return 0;
    }
	if(sizeOfAllocation<512*512*2+4){
		std::cout << "not enough\n";
		return 0;
	}

	// decode
	uint32_t pixelcount = 512*512;
	float aspect = *((float*)data);
	void* initimage = malloc(pixelcount*2);
	memcpy(initimage, (uint8_t*)data+4, pixelcount*2);
	free(data);

	std::vector<uint32_t> perm = build_permutation(pixelcount, seed);


	void* together = malloc(pixelcount*4);

	for(int y=0; y<512; y++) {
		for(int x=0; x<512; x++) {

			uint32_t* location = GetMemoryLocationAny<uint32_t>((uint32_t*)together, x, y, 512);

			uint8_t Y = *GetMemoryLocationAny<uint8_t>((uint8_t*)initimage, x*2, y, 1024);
			uint8_t CBCR = *GetMemoryLocationAny<uint8_t>((uint8_t*)initimage, x*2+1, y, 1024);

			int preCb = (CBCR >> 4) & 0xF;
			int preCr = (CBCR) & 0xF;

			int Cb = (preCb*16);
			int Cr = (preCr*16);
				
			int y  = Y;
			int cb = Cb - 128;
			int cr = Cr - 128;

			cb = clampv2(cb, -128, 127);
			cr = clampv2(cr, -128, 127);

			float myCB = (float)cb/255.0f;
			float myCR = (float)cr/255.0f;

			 int r = (int) (Y + 1.40200 * (Cr - 0x80));
			 int g = (int) (Y - 0.34414 * (Cb - 0x80) - 0.71414 * (Cr - 0x80));
			int b = (int) (Y + 1.77200 * (Cb - 0x80));

			
			uint8_t R = clampv2(r, 0, 255);
			uint8_t G = clampv2(g, 0, 255);
			uint8_t B = clampv2(b, 0, 255);

			*location = (255 << 24) | (R << 16) | (G << 8) | B;
		}
	}

	free(initimage);


	void* unshuffled = malloc(pixelcount*4);

    for (size_t i = 0; i < pixelcount; ++i)
    {
        size_t j = perm[i];

        memcpy(
            (uint8_t*)unshuffled + i * 4,
            (uint8_t*)together + j * 4,
            4
        );
    }
	free(together);

	uint32_t new_width = 512*aspect;


	std::cout << "Aspect: " << aspect << "\n";
	std::cout << "New Width: " << new_width << "\n";
	void* final = malloc(new_width*512*4);
	stbir_resize_uint8_linear((unsigned char*)unshuffled, 512, 512, 0, (unsigned char*)final, new_width, 512, 0, STBIR_RGBA);
	free(unshuffled);

	if(new_width > 10000 || new_width <= 0) {
		std::cout << "Width bad\n";
		return 0;
	}

	*imgwidth = new_width;
	*imgheight = 512;

	return final;
}