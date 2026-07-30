
#include "../headers/renderops.hpp"
#include <gl/gl.h>

void InitializeRenderOperations(GlobalParams* m) {
}
void DeInitializeRenderOperations(){
}

void CircleGenerator(GlobalParams* m, int circleDiameter, int locX, int locY, uint32_t color, bool onlyUnderToolbar) {
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;
    glColor4f(r, g, b, 1.0f);

	glBegin(GL_LINE_LOOP);
	int seg = 32;
    for(int i = 0; i < seg; i++)
    {
        float theta = 2.0f * 3.1415926f * float(i) / float(seg);

        float x = circleDiameter * cosf(theta) * 0.5f;
        float y = circleDiameter * sinf(theta) * 0.5f;

        glVertex2f(x + locX, y + locY);//output vertex

    }
	glEnd();
}

int PlaceString(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color) {
    if (!inputstr || *inputstr == '\0') return 0;

    HDC hdc = wglGetCurrentDC();
    if (!hdc) return 0;

    GLuint base_list = glGenLists(96);
    if (base_list == 0) return 0;

    HFONT font = CreateFontA(
        -size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Verdana"
    );

    HGDIOBJ old_font = SelectObject(hdc, font);
    wglUseFontBitmapsA(hdc, 32, 96, base_list);
    SelectObject(hdc, old_font);
    DeleteObject(font);

    glPushAttrib(GL_CURRENT_BIT | GL_LIST_BIT | GL_TRANSFORM_BIT);

    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;
    glColor4f(r, g, b, 1.0f);

    glRasterPos2i(locX, locY + size);

    glListBase(base_list - 32);
    glCallLists((GLsizei)strlen(inputstr), GL_UNSIGNED_BYTE, inputstr);

    glPopAttrib();
    glDeleteLists(base_list, 96);

    return 1;
}

int PlaceStringShadow(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, float sigma, int shadowOffsetX, int shadowOffsetY, int passes, uint32_t shadowColor) {

	PlaceString(m, size, inputstr, locX, locY, color);
	
	//bool b = opsPlaceStringShadowObject(m, size, inputstr, locX+shadowOffsetX, locY+shadowOffsetY, shadowColor, m->scrdata, sigma, passes);
	//bool e = opsPlaceStringBuffer(m, size, inputstr, locX, locY, color, m->scrdata, m->width, m->height, m->scrdata);

	return 0;
}

void drawLine(GlobalParams* m, int startX, int startY, int len, bool horizontal, uint32_t color, float opacity) {
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;
    glColor4f(r, g, b, opacity);

    float x0 = (float)startX;
    float y0 = (float)startY;
    float x1, y1;

    if (horizontal) {
        x1 = startX + len;
        y1 = startY + 1.0f;
    } else {
        x1 = startX + 1.0f;
        y1 = startY + len;
    }

    glBegin(GL_QUADS);
    glVertex2f(x0, y0);
    glVertex2f(x1, y0);
    glVertex2f(x1, y1);
    glVertex2f(x0, y1);
    glEnd();
}

void dDrawFilledRectangle(GlobalParams* m, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
	float r = ((color >> 16) & 0xFF) / 255.0f;
	float g = ((color >> 8) & 0xFF) / 255.0f;
	float b = (color & 0xFF) / 255.0f;

	glColor4f(r, g, b, opacity);

	int endX = xloc + width;
	int endY = yloc + height;

	glBegin(GL_QUADS);
	glVertex2i(xloc, yloc);
	glVertex2i(endX, yloc);
	glVertex2i(endX, endY);
	glVertex2i(xloc, endY);
	glEnd();
}

void PlaceFromAtlas(GlobalParams* m, void* source, int sourceWidth, int sourceHeight, int sourceX, int sourceY, int destX, int destY, int width, int height, uint32_t color_tint, float opacity) {
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_PIXEL_MODE_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float r = ((color_tint >> 16) & 0xFF) / 255.0f;
    float g = ((color_tint >> 8) & 0xFF) / 255.0f;
    float b = (color_tint & 0xFF) / 255.0f;

    glPixelTransferf(GL_RED_SCALE, r); glPixelTransferf(GL_GREEN_SCALE, g); glPixelTransferf(GL_BLUE_SCALE, b); glPixelTransferf(GL_ALPHA_SCALE, opacity);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, sourceWidth);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, sourceX);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, sourceY);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glPixelZoom(1.0f, -1.0f);

    glRasterPos2i(destX, destY);

    glDrawPixels(width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, source);

    glPixelZoom(1.0f, 1.0f);

    glPixelTransferf(GL_RED_SCALE, 1.0f);
    glPixelTransferf(GL_GREEN_SCALE, 1.0f);
    glPixelTransferf(GL_BLUE_SCALE, 1.0f);
    glPixelTransferf(GL_ALPHA_SCALE, 1.0f);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    glPopAttrib();
}

void blur_toolbar(GlobalParams* m) {
	
}

void gaussian_blur(GlobalParams* m, int lW, int lH, double sigma, uint32_t offX, uint32_t offY) {

}

void boxBlur(GlobalParams* m, uint32_t kernelSize, int mode, int startOffset, int vsize) {

}