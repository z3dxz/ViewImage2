
#define _CRT_SECURE_NO_WARNINGS
#include "m45_support.hpp"

#define GetMemoryLocation(start, x, y, widthfactor) \
	((uint32_t*)(start) + ((y) * (widthfactor)) + (x))\
\

#define GetMemoryLocationByte(start, x, y, widthfactor) \
	((uint8_t*)(start) + ((y) * (widthfactor)) + (x))\
\

struct VCOLOR{
	uint8_t a;
	uint8_t b;
	uint8_t c;
};


float lerp(float a, float b, float t) {
	return (1.0f - t) * a + t * b;
}


VCOLOR rgbToHsv(uint32_t rgbColor) {
	// Extract the individual RGB components from the 32-bit color
	uint8_t r = (rgbColor >> 16) & 0xFF;
	uint8_t g = (rgbColor >> 8) & 0xFF;
	uint8_t b = rgbColor & 0xFF;

	double r_norm = r / 255.0;
	double g_norm = g / 255.0;
	double b_norm = b / 255.0;

	double cMax = std::max( std::max(r_norm, g_norm), b_norm);
	double cMin =  std::min( std::min(r_norm, g_norm), b_norm);
	double delta = cMax - cMin;

	// Calculate hue
	double h = 0;
	if (delta != 0) {
		if (cMax == r_norm) {
			h = 60 * fmod((g_norm - b_norm) / delta, 6);
		}
		else if (cMax == g_norm) {
			h = 60 * ((b_norm - r_norm) / delta + 2);
		}
		else {
			h = 60 * ((r_norm - g_norm) / delta + 4);
		}
	}
	if (h < 0) {
		h += 360;
	}

	// Calculate saturation
	double s = (cMax == 0) ? 0 : delta / cMax;

	// Calculate value
	double v = cMax;

	// Scale HSV components to 8-bit range and pack into a 32-bit color
	uint8_t h_byte = static_cast<uint8_t>(h / 360.0 * 255);
	uint8_t s_byte = static_cast<uint8_t>(s * 255);
	uint8_t v_byte = static_cast<uint8_t>(v * 255);

	//return (static_cast<uint32_t>(h_byte) << 16) | (static_cast<uint32_t>(s_byte) << 8) | static_cast<uint32_t>(v_byte);
	return { v_byte, s_byte, h_byte };
}

//2
VCOLOR hsvToRgb(uint8_t v_byte, uint8_t s_byte, uint8_t h_byte) {

	double h = static_cast<double>(h_byte) / 255.0 * 360.0;
	double s = static_cast<double>(s_byte) / 255.0;
	double v = static_cast<double>(v_byte) / 255.0;

	double c = v * s;
	double x = c * (1 - std::abs(fmod(h / 60, 2) - 1));
	double m = v - c;

	double r, g, b;
	if (h >= 0 && h < 60) {
		r = c;
		g = x;
		b = 0;
	}
	else if (h >= 60 && h < 120) {
		r = x;
		g = c;
		b = 0;
	}
	else if (h >= 120 && h < 180) {
		r = 0;
		g = c;
		b = x;
	}
	else if (h >= 180 && h < 240) {
		r = 0;
		g = x;
		b = c;
	}
	else if (h >= 240 && h < 300) {
		r = x;
		g = 0;
		b = c;
	}
	else {
		r = c;
		g = 0;
		b = x;
	}

	// Scale RGB components to 8-bit range and pack into a 32-bit color
	uint8_t r_byte = static_cast<uint8_t>((r + m) * 255);
	uint8_t g_byte = static_cast<uint8_t>((g + m) * 255);
	uint8_t b_byte = static_cast<uint8_t>((b + m) * 255);

	return {r_byte, g_byte, b_byte};
}


double randomRoundingDivision(double numerator, double denominator) {
	double result = numerator / denominator;
	double fractionalPart = result - static_cast<int>(result);

	// Generate a random number between 0 and 1
	double randomNum = static_cast<double>(rand()) / RAND_MAX;

	// Randomly round up or down
	if (randomNum < fractionalPart) {
		result = ceil(result); // Round up
	}
	else {
		result = floor(result); // Round down
	}

	return result;
}

int randomRoundingMultiplication(int num1, int num2) {
	int factor1 = num1 * num2;
	int factor2 = num1 * (num2-1);

	return lerp(factor2,factor1,(float)(rand()%100)/100.0f);
}


uint8_t pack(uint8_t value1, uint8_t value2) {
	// Extract the 4 least significant bits from each value

	uint8_t kval = randomRoundingDivision(value1, 17);
	uint8_t vval = randomRoundingDivision(value2, 17);
	
	//uint8_t kval = value1/ 17;
	//uint8_t vval = value2/ 17;

	uint8_t packed_value = (kval) | ((vval) << 4);
	return packed_value;
}


VCOLOR unpackNoRounding(int8_t packed) {

	uint8_t unpacked1 = (packed >> 4) & 0x0F;
	uint8_t unpacked2 = (packed) & 0x0F;

	uint8_t modulated1 = (unpacked1* 17);
	uint8_t modulated2 = (unpacked2* 17);

	//uint8_t modulated1 = unpacked1* 17;
	//uint8_t modulated2 = unpacked2* 17;

	return { modulated1, modulated2, 0 };
}

VCOLOR unpack(int8_t packed) {

	uint8_t unpacked1 = (packed >> 4) & 0x0F;
	uint8_t unpacked2 = (packed) & 0x0F;

	uint8_t modulated1 = randomRoundingMultiplication(unpacked1 , 17);
	uint8_t modulated2 = randomRoundingMultiplication(unpacked2 , 17);

	//uint8_t modulated1 = unpacked1* 17;
	//uint8_t modulated2 = unpacked2* 17;

	return { modulated1, modulated2, 0};
}

void extractRGB(uint32_t color, uint8_t& red, uint8_t& green, uint8_t& blue) {
	red = static_cast<uint8_t>((color >> 16) & 0xFF);
	green = static_cast<uint8_t>((color >> 8) & 0xFF);
	blue = static_cast<uint8_t>(color & 0xFF);
}

// Helper function to combine RGB components into a uint32_t color value
uint32_t combineRGB(uint8_t red, uint8_t green, uint8_t blue) {
	return (static_cast<uint32_t>(red) << 16) | (static_cast<uint32_t>(green) << 8) | static_cast<uint32_t>(blue);
}


// Function to find the mean color between 8 colors (uint32_t values)
uint32_t findMeanColor2(uint32_t color1, uint32_t color2) {
	uint8_t red1, green1, blue1;
	uint8_t red2, green2, blue2;

	// Extract RGB components from the two colors
	extractRGB(color1, red1, green1, blue1);
	extractRGB(color2, red2, green2, blue2);

	// Find the average RGB values
	uint8_t averageRed = static_cast<uint8_t>((red1 + red2) / 2);
	uint8_t averageGreen = static_cast<uint8_t>((green1 + green2) / 2);
	uint8_t averageBlue = static_cast<uint8_t>((blue1 + blue2) / 2);

	// Combine RGB components to get the mean color
	return combineRGB(averageRed, averageGreen, averageBlue);
}

// Function to find the mean color between 8 colors (uint32_t values)
uint32_t findMeanColor16(uint32_t colors[16]) {
	// Variables to store the sum of RGB components
	uint32_t sumRed = 0, sumGreen = 0, sumBlue = 0;

	// Calculate the sum of RGB components for all 8 colors
	for (int i = 0; i < 8; ++i) {
		uint8_t red, green, blue;
		extractRGB(colors[i], red, green, blue);
		sumRed += red;
		sumGreen += green;
		sumBlue += blue;
	}

	// Find the average RGB values
	uint8_t averageRed = static_cast<uint8_t>(sumRed / 16);
	uint8_t averageGreen = static_cast<uint8_t>(sumGreen / 16);
	uint8_t averageBlue = static_cast<uint8_t>(sumBlue / 16);

	// Combine RGB components to get the mean color
	return combineRGB(averageRed, averageGreen, averageBlue);
}

// encoding

bool encodefile(void* idd, uint32_t iw, uint32_t ih, const char* filepath, float aspect) {

	if (ih % 2 == 1) ih--;
	int imgByteSize = (((float)iw * (float)ih) + (((float)iw/8.0f)*((float)ih/2.0f))) + 5 + 2; // 2=intervals

	void* data = malloc(imgByteSize);

	if (!data) {
		//no img data
		return false;
	}

	if (iw > 65536 || ih > 65536) {
		// image width or height too big
		return false;
		//return "sorry, image width or height is too big";
	}


	float* ptr_ = (float*)data;

	*ptr_ = aspect;

	byte* ptr = (byte*)data;
	ptr += 4;
	*ptr = 45;
	ptr++;

	for (int y = 0; y < ih/2; y++) {
		for (int x = 0; x < iw; x++) {
			uint32_t cc = (*GetMemoryLocation(idd, x, y*2, iw));
			INT8 pix = rgbToHsv(cc).a;
			if (pix == 0) { pix = 1; }
			*ptr = pix;
			ptr++;
		}
	}

	*ptr = 0x00;
	ptr += 1;

	for (int y = 0; y < ih / 2; y++) {
		for (int x = 0; x < iw; x++) {
			uint32_t cc = (*GetMemoryLocation(idd, x, y * 2 + 1, iw));
			INT8 pix = rgbToHsv(cc).a;
			if (pix == 0) { pix = 1; }
			*ptr = pix;
			ptr++;
		}
	}

	*ptr = 0x00;
	ptr += 1;


	for (int y = 0; y < ih/2; y++) {
		for (int x = 0; x < iw/8; x++) {
			uint32_t c0 = (*GetMemoryLocation(idd, x * 8    , y * 2, iw));
			uint32_t c1 = (*GetMemoryLocation(idd, x * 8 + 1, y * 2, iw));
			uint32_t c2 = (*GetMemoryLocation(idd, x * 8 + 2, y * 2, iw));
			uint32_t c3 = (*GetMemoryLocation(idd, x * 8 + 3, y * 2, iw));
			uint32_t c4 = (*GetMemoryLocation(idd, x * 8 + 4, y * 2, iw));
			uint32_t c5 = (*GetMemoryLocation(idd, x * 8 + 5, y * 2, iw));
			uint32_t c6 = (*GetMemoryLocation(idd, x * 8 + 6, y * 2, iw));
			uint32_t c7 = (*GetMemoryLocation(idd, x * 8 + 7, y * 2, iw));

			uint32_t c0a = (*GetMemoryLocation(idd, x * 8, y * 2+1, iw));
			uint32_t c1a = (*GetMemoryLocation(idd, x * 8 + 1, y * 2 + 1, iw));
			uint32_t c2a = (*GetMemoryLocation(idd, x * 8 + 2, y * 2 + 1, iw));
			uint32_t c3a = (*GetMemoryLocation(idd, x * 8 + 3, y * 2 + 1, iw));
			uint32_t c4a = (*GetMemoryLocation(idd, x * 8 + 4, y * 2 + 1, iw));
			uint32_t c5a = (*GetMemoryLocation(idd, x * 8 + 5, y * 2 + 1, iw));
			uint32_t c6a = (*GetMemoryLocation(idd, x * 8 + 6, y * 2 + 1, iw));
			uint32_t c7a = (*GetMemoryLocation(idd, x * 8 + 7, y * 2 + 1, iw));

			uint32_t colors[16] = { c0,c1,c2,c3,c4,c5,c6,c7,c0a,c1a,c2a,c3a,c4a,c5a,c6a,c7a };
			uint32_t f = findMeanColor16(colors);
			INT8 pix = pack(rgbToHsv(f).b,rgbToHsv(f).c);
			if (pix == 0) { pix = 1; }
			*ptr = pix;
			ptr++;
		}
	}

	char str_path[256];
	strcpy(str_path, filepath);

	char* last_dot = strrchr(str_path, '.');
	if (last_dot != NULL) {
		*last_dot = '\0';
	}

	strcat(str_path, ".m45");

	// write to a file
	HANDLE hFile = CreateFile( str_path, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return "sorry, no file handling";
	}

	// Write data to the file
	DWORD bytesWritten;
	WriteFile( hFile, data, imgByteSize, &bytesWritten, 0);         // Overlapped

	// Close the handle once we don't need it.
	CloseHandle(hFile);

	free(data);

	return true;
}


bool encodedata(void* idd, uint32_t iw, uint32_t ih, const char* filepath) {

	if (!idd) {
		return false;
	}

	int desired_width = 320;
	int desired_height = 480;
	int output_channels = 4; // Keep the same number of channels

	// Allocate memory for the resized image
	void* resized_image_data = malloc(desired_width * desired_height * output_channels);

	stbir_resize_uint8_srgb((unsigned char*)idd, iw, ih, 0, (unsigned char*)resized_image_data, desired_width, desired_height, 0, STBIR_ARGB);

	bool l = encodefile(resized_image_data, desired_width, desired_height, filepath, (float)iw/ (float)ih);

	return l;
}

bool encodeimage(const char* filepath) {

	int imgwidth;
	int imgheight;
	int channels;
	void* imgdata2 = stbi_load(filepath, &imgwidth, &imgheight, &channels, 4);

	if (!imgdata2) {
		return false;
	}

	int desired_width = 320;
	int desired_height = 480;
	int output_channels = 4; // Keep the same number of channels

	// Allocate memory for the resized image
	void* resized_image_data = malloc(desired_width * desired_height * output_channels);

	stbir_resize_uint8_srgb((unsigned char*)imgdata2, imgwidth, imgheight, 0, (unsigned char*)resized_image_data, desired_width, desired_height, 0, STBIR_ARGB);

	bool l = encodefile(resized_image_data, desired_width, desired_height, filepath, (float)imgwidth/ (float)imgheight);

	free(imgdata2);

	return l;

}


float HueLerp(float startHue, float endHue, float t) {
	// Convert hues to RGB.
	float startR, startG, startB;
	VCOLOR k = hsvToRgb(255, 255, startHue);
	startR = k.a; startG = k.b; startB = k.c;

	float endR, endG, endB;
	VCOLOR k2 = hsvToRgb(255, 255, endHue);
	endR = k2.a; endG = k2.b; endB = k2.c;

	// Lerping RGB values.
	float lerpedR = startR + t * (endR - startR);
	float lerpedG = startG + t * (endG - startG);
	float lerpedB = startB + t * (endB - startB);

	// Convert back to hue.
	float lerpedHue = rgbToHsv(RGB(lerpedB, lerpedG, lerpedR)).c;
	return lerpedHue;
}

const int SEARCH_SIZE = 1;

const void* findZero(const byte* startAddress, size_t size) {

	for (size_t i = 0; i < size; i++) {
		if (startAddress[i] == 0) {
			return static_cast<const void*>(&startAddress[i - SEARCH_SIZE + 1]);
		}
	}

	return nullptr;
}

void* decode_m45(const char* filepath, int* imgwidth, int* imgheight) {

	// Opens file
	HANDLE hFile = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		return 0;
	}

	DWORD fsize = GetFileSize(hFile, 0);

	int sizeOfAllocation = fsize;
	void* data = malloc(sizeOfAllocation);
	DWORD dwBytesRead = 0;
	DWORD dwBytesWritten = 0;

	if (!ReadFile(hFile, data, sizeOfAllocation, &dwBytesRead, NULL)) {
		return 0;
	}

	float asp = *(float*)data;
	if (*((byte*)data + 4) != 45) {
		asp = 1.33333333;
	}

	int width = 320;
	int height = 480;

	int numOfPixels = width*height;

	if (width == 0) {
		return 0;
	}
	int bitmapDataSize = numOfPixels * 4;

	void* bitmapData = malloc(bitmapDataSize);

	int* bmp_ptr = (int*)bitmapData;

	byte* dataptr = (byte*)data;


	byte* field1start = (dataptr + 5);
	byte* field2start = (dataptr + (320 * 240) + 5+1);//(byte*)findZero(dataptr + 5, fsize - 5) + 1;

	byte* startc = (dataptr + (320 * 240)+ (320 * 240) + 5+2);//(byte*)findZero(field2start, fsize - (dataptr-field2start)) + 1;

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int x0 = x - 3;
				bool secondfield = false;

				byte* v_ptr = GetMemoryLocationByte(field1start, x, y / 2, width);

				if (y % 2 == 1) {
					v_ptr = GetMemoryLocationByte(field2start, x, y / 2, width);
				}


				byte* s_ptr = GetMemoryLocationByte(startc, ((x0) / 8), (y / 2), width / 8);
				byte* s_ptr2 = GetMemoryLocationByte(startc, ((x0) / 8) + 1, (y / 2), width / 8);

				byte* s_ptr3 = GetMemoryLocationByte(startc, ((x0) / 8), (y / 2) + 1, width / 8);
				byte* s_ptr4 = GetMemoryLocationByte(startc, ((x0) / 8) + 1, (y / 2) + 1, width / 8);

				byte* max_m = dataptr + fsize;

				uint8_t colorCarrier = 0;
				uint8_t colorCarrier2 = 0;
				uint8_t colorCarrier3 = 0;
				uint8_t colorCarrier4 = 0;

				if (s_ptr <= max_m);
				colorCarrier = (*s_ptr);

				if (s_ptr2 <= max_m);
				colorCarrier2 = (*s_ptr2);

				if (s_ptr3 <= max_m);
				colorCarrier3 = (*s_ptr3);

				if (s_ptr4 <= max_m);
				colorCarrier4 = (*s_ptr4);


				uint8_t lumCarrier = 0;
				if (v_ptr <= max_m)
					lumCarrier = (*v_ptr);

				uint8_t myMeanB = lerp(lerp((float)(unpack(colorCarrier).b), (float)(unpack(colorCarrier2).b), ((float)(x0 % 8)) / 8.0f), lerp((float)(unpack(colorCarrier3).b), (float)(unpack(colorCarrier4).b), ((float)(x0 % 8)) / 8.0f), ((float)(y % 2)) / 2.0f);
				uint8_t myMeanA = HueLerp(HueLerp((float)(unpack(colorCarrier).a), (float)(unpack(colorCarrier2).a), ((float)(x0 % 8)) / 8.0f), HueLerp((float)(unpack(colorCarrier3).a), (float)(unpack(colorCarrier4).a), ((float)(x0 % 8)) / 8.0f), ((float)(y % 2)) / 2.0f);

				uint32_t myMeanB2 = myMeanB * 1.1f;
				if (myMeanB2 > 255) { myMeanB2 = 255; }

				VCOLOR l = hsvToRgb(lumCarrier, myMeanB2, myMeanA);

				int r = l.a;
				int g = l.b;
				int b = l.c;
				int a = 255;

				int c = (a * 16777216) + (r * 65536) + (g * 256) + b;
				*bmp_ptr++ = c;
			}
		}
	


	int desired_width = 480*asp;
	int desired_height = 480;

	*imgwidth = desired_width;
	*imgheight = desired_height;

	void* resized_image_data = malloc(desired_width * desired_height * 4);
	srand(time(0));
	stbir_resize_uint8_srgb((unsigned char*)bitmapData, width, height, 0, (unsigned char*)resized_image_data, desired_width, desired_height, 0, STBIR_ARGB);

	char szFileName[200];

	SYSTEMTIME systemTime;
	GetLocalTime(&systemTime);

	sprintf(szFileName, "D:\\render\\future_retro\\New folder\\IMG_saved %04d-%02d-%02d %02d%02d%02d%02d.png",
		systemTime.wYear, systemTime.wMonth, systemTime.wDay,
		systemTime.wHour % 24, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);

	std::string path = "D:\\render\\future_retro\\New folder\\" + std::to_string(rand() % 400) + ".png";
	stbi_write_png(szFileName, desired_width, desired_height, 4, resized_image_data, 0);

	return resized_image_data;
}