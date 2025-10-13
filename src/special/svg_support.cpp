#include "svg_support.hpp"

void* decodesvg(const char* filepath, int* imgwidth, int* imgheight) {

	const std::unique_ptr<lunasvg::Document> document = lunasvg::Document::loadFromFile(filepath);

	if (document == nullptr)
	{
		std::cout << "Unable to load SVG Document: " << filepath << "\n";
		return nullptr;
	}
	
	lunasvg::Bitmap bitmap = document->renderToBitmap();

	if (bitmap.isNull())
	{
		std::cout << "Unable to load SVG Bitmap: " << filepath << "\n";
		return nullptr;
	}

	*imgwidth = bitmap.width();
	*imgheight = bitmap.height();

	void* imgdata = malloc(4*bitmap.width()*bitmap.height());
	memcpy(imgdata, bitmap.data(), 4*bitmap.width()*bitmap.height());	
	
	return imgdata;
}