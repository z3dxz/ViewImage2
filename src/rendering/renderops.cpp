#include "renderops.hpp"
#include <gl/gl.h>
#include <cmath>
#include <vector>
#include "rgblur.hpp"
#include "font.hpp"

int  cfont;

FT_Face current;

void SwitchFont(FT_Face font) {
    current = font;
}

void FilterByScale(float uiscale) {
    if(uiscale != 1.0f) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
}

void CircleGenerator(GlobalParams* m, int circleDiameter, int locX, int locY, uint32_t color, bool onlyUnderToolbar) {
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;
    glColor4f(r, g, b, 1.0f);

    const int seg = 32;
    GLfloat vertices[seg * 2];

    for (int i = 0; i < seg; i++) {
        float theta = 2.0f * 3.1415926f * float(i) / float(seg);
        float x = circleDiameter * cosf(theta) * 0.5f;
        float y = circleDiameter * sinf(theta) * 0.5f;

        vertices[i * 2]     = x + locX;
        vertices[i * 2 + 1] = y + locY;
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    
    glLineWidth(m->uiscale);
    glDrawArrays(GL_LINE_LOOP, 0, seg);

    glDisableClientState(GL_VERTEX_ARRAY);
}

int PlaceString(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color) {
    PlaceStringBuf(current, size, inputstr, locX, locY, color, m->uiscale, false);
    return 1;
}

int PlaceStringShadow(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color) {
    PlaceStringBuf(current, size, inputstr, locX, locY, color, m->uiscale, true);
    return 0;
}

void drawLine(GlobalParams* m, int startX, int startY, int len, bool horizontal, uint32_t color, float opacity) {
    startX *= m->uiscale;
    startY *= m->uiscale;
    len *= m->uiscale;

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

    GLfloat vertices[] = {
        x0, y0,
        x1, y0,
        x1, y1,
        x0, y1
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    
    FilterByScale(m->uiscale);
    glDrawArrays(GL_QUADS, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);
}

void dDrawFilledRectangle(GlobalParams* m, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
    xloc *= m->uiscale;
    yloc *= m->uiscale;
    width *= m->uiscale;
    height *= m->uiscale;

    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;

    glColor4f(r, g, b, opacity);

    GLfloat endX = (GLfloat)(xloc + width);
    GLfloat endY = (GLfloat)(yloc + height);
    GLfloat startX = (GLfloat)xloc;
    GLfloat startY = (GLfloat)yloc;

    GLfloat vertices[] = {
        startX, startY,
        endX,   startY,
        endX,   endY,
        startX, endY
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    
    glDrawArrays(GL_QUADS, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);
}

void PlaceFromAtlas(GlobalParams* m, GLAtlasTxt element, int sourceX, int sourceY, int destX, int destY, int width, int height, uint32_t color_tint, float opacity) {

    destX *= m->uiscale;
    destY *= m->uiscale;

    int adjwidth = width * m->uiscale;
    int adjheight = height * m->uiscale;

    if (!element.valid || !element.id) {
        std::cout << "Can't place: not valid\n";
        return;
    };

    uint8_t r = ((color_tint >> 16) & 0xFF);
    uint8_t g = ((color_tint >> 8)  & 0xFF);
    uint8_t b = (color_tint         & 0xFF);
    uint8_t a = (uint8_t)(opacity * 255.0f);
    glColor4ub(r,g,b,a);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, element.id);


    FilterByScale(m->uiscale);

    GLfloat vertices[] = {
        (GLfloat)destX,         (GLfloat)destY,
        (GLfloat)destX + adjwidth, (GLfloat)destY,
        (GLfloat)destX + adjwidth, (GLfloat)destY + adjheight,
        (GLfloat)destX,         (GLfloat)destY + adjheight
    };

    float texLeft   = (float)sourceX / (float)element.w;
    float texRight  = (float)(sourceX + width) / (float)element.w;
    float texTop    = (float)sourceY / (float)element.h;
    float texBottom = (float)(sourceY + height) / (float)element.h;

    GLfloat texCoords[] = {
        texLeft,  texTop,
        texRight, texTop,
        texRight, texBottom,
        texLeft,  texBottom
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

    glDrawArrays(GL_QUADS, 0, 4);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glDisable(GL_TEXTURE_2D);
}



void dDrawRectangle(GlobalParams* m, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
    xloc *= m->uiscale;
    yloc *= m->uiscale;
    width *= m->uiscale;
    height *= m->uiscale;
    
    if (width <= 0 || height <= 0) return;

    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;
    glColor4f(r, g, b, opacity);

    GLfloat x0 = (GLfloat)xloc;
    GLfloat y0 = (GLfloat)yloc;

    GLfloat x1 = (GLfloat)(xloc + width - 1);
    GLfloat y1 = (GLfloat)(yloc + height - 1);

    GLfloat vertices[] = {
        x0, y0,
        x1, y0,
        x1, y1,
        x0, y1
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glLineWidth(m->uiscale);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void dDrawRoundedRectangle(GlobalParams* m, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
    xloc *= m->uiscale;
    yloc *= m->uiscale;
    width *= m->uiscale;
    height *= m->uiscale;

    if (width <= 0 || height <= 0) return;

    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;
    glColor4f(r, g, b, opacity);

    GLfloat x0 = (GLfloat)xloc;
    GLfloat y0 = (GLfloat)yloc;

    GLfloat x1 = (GLfloat)(xloc + width - 1);
    GLfloat y1 = (GLfloat)(yloc + height - 1);

    GLfloat vertices[] = {
        x0 + 1.0f, y0,
        x1 - 1.0f, y0,
        x1,        y0 + 1.0f,
        x1,        y1 - 1.0f,
        x1 - 1.0f, y1,
        x0 + 1.0f, y1,
        x0,        y1 - 1.0f,
        x0,        y0 + 1.0f
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glLineWidth(m->uiscale);
    glDrawArrays(GL_LINE_LOOP, 0, 8);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void dDrawRoundedFilledRectangle(GlobalParams* m, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
    xloc *= m->uiscale;
    yloc *= m->uiscale;
    width *= m->uiscale;
    height *= m->uiscale;
    
    if (width <= 0 || height <= 0) return;

    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;
    glColor4f(r, g, b, opacity);

    GLfloat x0 = (GLfloat)xloc;
    GLfloat y0 = (GLfloat)yloc;
    GLfloat x1 = (GLfloat)(xloc + width);
    GLfloat y1 = (GLfloat)(yloc + height);

    GLfloat vertices[] = {
        x0 + 1.0f, y0,
        x1 - 1.0f, y0,
        x1 - 1.0f, y1,
        x0 + 1.0f, y1,
        x0,        y0 + 1.0f,
        x0 + 1.0f, y0 + 1.0f,
        x0 + 1.0f, y1 - 1.0f,
        x0,        y1 - 1.0f,
        x1 - 1.0f, y0 + 1.0f,
        x1,        y0 + 1.0f,
        x1,        y1 - 1.0f,
        x1 - 1.0f, y1 - 1.0f
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glDrawArrays(GL_QUADS, 0, 12);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void InitializeRenderOperations(GlobalParams* m) {
}
void DeInitializeRenderOperations(){
}

void blur_toolbar(GlobalParams* m) {
    gaussian_blur_render(m, m->width, m->toolheight, 4.0f, 0, 0);
}

void gaussian_blur(GlobalParams* m, int lW, int lH, double sigma, uint32_t offX, uint32_t offY) {
    gaussian_blur_render(m, lW, lH, sigma, offX, offY);

}

void boxBlur(GlobalParams* m, uint32_t kernelSize, int mode, int startOffset, int vsize) {

}