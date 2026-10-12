#include <iostream>
#include <string>
#include <cmath>
#include <windows.h>
#include <gl/gl.h>
#include "../headers/ops.hpp"
#include "render_ui.hpp"
#define REDRAW_PADDING 4

unsigned int imgdatatxt = 0;
unsigned int checkertxt = 0;

// Extension constant
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

HGLRC hglrc;
uint32_t lastImgW = 0, lastImgH = 0;
uint32_t potImgW = 0, potImgH = 0;

uint32_t NextPowerOfTwo(uint32_t n) {
    if (n == 0) return 1; n--; n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; n |= n >> 16;
    return n + 1;
}

GLAtlasTxt LoadOpenGLImage(void* memory, int size) {
    int w, h, c;
    unsigned char* img = stbi_load_from_memory((stbi_uc*)memory, size, &w, &h, &c, 0);
    if (!img) {
        std::cerr << "Failed to load OpenGL Texture\n";
        return {0, 0, false};
    }
    
    if (c != 4) {
        std::cerr << "Only 4-channel textures are supported\n";
        free(img); 
        return {0, 0, false};
    }

    int new_w = NextPowerOfTwo(w);
    int new_h = NextPowerOfTwo(h);

    unsigned char* new_img = img;
    if (new_w != w || new_h != h) {
        new_img = new unsigned char[new_w * new_h * 4]();
        for (int y = 0; y < h; ++y) {
            memcpy(new_img + (y * new_w * 4), img + (y * w * 4), w * 4);
        }
    }

    GLAtlasTxt atl_ele;
    
    glGenTextures(1, &atl_ele.id);
    glBindTexture(GL_TEXTURE_2D, atl_ele.id);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, new_w, new_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, new_w, new_h, GL_RGBA, GL_UNSIGNED_BYTE, new_img);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    if (new_img != img) {
        delete[] new_img;
    }
    
    free(img);

    atl_ele.valid = true;
    atl_ele.w = new_w;
    atl_ele.h = new_h;
    return atl_ele;
}

void SetupPixelFormat(GlobalParams* m) {
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 24;
    if(m->aeromode) {pfd.cAlphaBits = 8;};
    pfd.iLayerType = PFD_MAIN_PLANE;
    int pixelFormat = ChoosePixelFormat(m->hdc, &pfd);
    if (pixelFormat == 0) {
        MessageBoxA(NULL, "ChoosePixelFormat Failed", "OpenGL Fatal Error", MB_OK | MB_ICONERROR);
        return;
    }
    if (!SetPixelFormat(m->hdc, pixelFormat, &pfd)) {
        MessageBoxA(NULL, "SetPixelFormat Failed", "OpenGL Fatal Error", MB_OK | MB_ICONERROR);
    }
}

void MakeCheckerboard() {
    uint32_t* checker = new uint32_t[4];
    uint32_t ch1 = 0xFF1A1A1A;
    uint32_t ch2 = 0xFF131313;
    checker[0] = ch1; checker[3] = ch1;
    checker[1] = ch2; checker[2] = ch2;
    if (checkertxt) glDeleteTextures(1, &checkertxt);
    glGenTextures(1, &checkertxt);
    glBindTexture(GL_TEXTURE_2D, checkertxt);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2, 2, GL_BGRA_EXT, GL_UNSIGNED_BYTE, checker);
    delete[] checker;
}

void DrawCheckerboard(GlobalParams* m) {
    UpdateMousePos(m);
    if (!checkertxt) return;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, checkertxt);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glColor3f(1.0f, 1.0f, 1.0f);

    float chk_size = 9.0f * m->uiscale;

    float tile2X = ((float)m->rlwidth / 2.0f / chk_size);
    float tile2Y = ((float)m->rlheight / 2.0f / chk_size);

    GLfloat vertices[] = {
        0.0f, 0.0f,
        (GLfloat)m->rlwidth, 0.0f,
        (GLfloat)m->rlwidth, (GLfloat)m->rlheight,
        0.0f, (GLfloat)m->rlheight
    };

    GLfloat texCoords[] = {
        0.0f, 0.0f,
        tile2X, 0.0f,
        tile2X, tile2Y,
        0.0f, tile2Y
    };

    // mpos: p

    if(m->aeromode && CanRenderToolbarMacro) {
        texCoords[1] = m->toolheight/2.0f/9.0f;
        texCoords[3] = m->toolheight/2.0f/9.0f;
        vertices[1] = m->toolheight;
        vertices[3] = m->toolheight;
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

    glDrawArrays(GL_QUADS, 0, 4);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glDisable(GL_TEXTURE_2D);
}

void InitializeOpenGL(GlobalParams* m) {
    SetupPixelFormat(m);
    hglrc = wglCreateContext(m->hdc);
    if (!hglrc) {
        MessageBoxA(NULL, "wglCreateContext Failed", "OpenGL Fatal Error", MB_OK | MB_ICONERROR);
        return;
    }
    if (!wglMakeCurrent(m->hdc, hglrc)) {
        MessageBoxA(NULL, "wglMakeCurrent Failed", "OpenGL Fatal Error", MB_OK | MB_ICONERROR);
        return;
    }
    int texSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
    printf("Max texture size: %d\n", texSize);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    MakeCheckerboard();
}

void NewImgDataTxt(GlobalParams* m) {
    if (imgdatatxt) glDeleteTextures(1, &imgdatatxt);
    potImgW = NextPowerOfTwo(m->imgwidth);
    potImgH = NextPowerOfTwo(m->imgheight);
    glGenTextures(1, &imgdatatxt);
    glBindTexture(GL_TEXTURE_2D, imgdatatxt);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, potImgW, potImgH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

void UpdateImage(GlobalParams* m, uint32_t* renderbuffer, ImageUpdateRgn* mregion) {
    if (!mregion->valid) return;
    if (!renderbuffer || !m->imgdata) return;

    if (lastImgW != m->imgwidth || lastImgH != m->imgheight || !imgdatatxt) {
        NewImgDataTxt(m);
        lastImgW = m->imgwidth;
        lastImgH = m->imgheight;
    }

    glBindTexture(GL_TEXTURE_2D, imgdatatxt);

    int xof = 0;
    int yof = 0;
    int xw = m->imgwidth;
    int yh = m->imgheight;

    if (!mregion->all) {
        xof = mregion->x; yof = mregion->y;
        xw = mregion->width; yh = mregion->height;
    }

    xof = std::clamp(xof, 0, (int)m->imgwidth);  yof = std::clamp(yof, 0, (int)m->imgheight);
    xw = std::clamp(xw, 0, (int)m->imgwidth - xof); yh = std::clamp(yh, 0, (int)m->imgheight - yof);

    if (xw <= 0 || yh <= 0) return;

    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)m->imgwidth);

    uint32_t* pixels = renderbuffer + (size_t)yof * m->imgwidth + xof;

    glTexSubImage2D(GL_TEXTURE_2D, 0, xof, yof, xw, yh, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

bool followsPattern(int number) {
    double logBase2 = log2((double)number / 100.0);
    return logBase2 == floor(logBase2);
}

void DrawImage(GlobalParams* m) {
    if (!imgdatatxt || !m->imgdata) return;
    const float offX = roundf(((float)m->rlwidth - m->imgwidth * m->mscaler) / 2.0f + m->iLocX);
    const float offY = roundf(((float)m->rlheight - m->imgheight * m->mscaler) / 2.0f + m->iLocY);
    const float distX = offX + roundf((float)m->imgwidth * m->mscaler);
    const float distY = offY + roundf((float)m->imgheight * m->mscaler);
    float maxImgU = (float)m->imgwidth / (float)potImgW;
    float maxImgV = (float)m->imgheight / (float)potImgH;

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, imgdatatxt);
    GLint filter = (m->smoothing) ? GL_LINEAR : GL_NEAREST;
    if (followsPattern((int)((m->mscaler) * 100))) {
        filter = GL_NEAREST;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    GLfloat vertices[] = {
        offX,  offY,
        offX,  distY,
        distX, distY,
        distX, offY
    };

    GLfloat texCoords[] = {
        0.0f,    0.0f,
        0.0f,    maxImgV,
        maxImgU, maxImgV,
        maxImgU, 0.0f
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

int redraw_timer = REDRAW_PADDING;

ImageUpdateRgn region = {false, false, 0,0,0,0};

void RedrawScene(GlobalParams* m);

void PerformRedraw(GlobalParams* m){
    if(redraw_timer>0) {
        redraw_timer--;
        RedrawScene(m);
    }
}

void RedrawSurface(GlobalParams* m, bool image_update, ImageUpdateRgn rgn) {
    redraw_timer = REDRAW_PADDING;
    if(image_update && !rgn.valid) {
        region.all = true;
        region.valid = true;
    }
    if(image_update && rgn.valid) {
        region = rgn;
    }
}

void RedrawForce(GlobalParams* m, bool image_update, ImageUpdateRgn rgn) {
    if(image_update && !rgn.valid) {
        region.all = true;
        region.valid = true;
    }
    if(image_update && rgn.valid) {
        region = rgn;
    }
    RedrawSurface(m);
    PerformRedraw(m);
}

#ifndef GL_FUNC_ADD
#define GL_FUNC_ADD 0x8006
#endif

static clock_t start, end;
void RedrawScene(GlobalParams* m) {
    start = clock();
    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_TRANSFORM_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if(m->aeromode) {
        if(m->glBlendFuncSeparate) {
            glEnable(GL_BLEND);
            m->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
    }

    glViewport(0, 0, m->rlwidth, m->rlheight);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, m->rlwidth, m->rlheight, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glTranslatef(0.375f, 0.375f, 0.0f);

    uint32_t* renderbuffer = (uint32_t*)m->imgdata;
    if (m->isImagePreview) renderbuffer = (uint32_t*)m->imagepreview;

    if (region.valid) {
        UpdateImage(m, renderbuffer, &region);
    }
    
    DrawCheckerboard(m);
    DrawImage(m);
    DrawGUI(m);
    SwapBuffers(m->hdc);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
    end = clock();
    region.valid = false;
}



void CleanupOpenGL(GlobalParams* m) {
    if (imgdatatxt) glDeleteTextures(1, &imgdatatxt);
    if (checkertxt) glDeleteTextures(1, &checkertxt);
	
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(m->hwnd, m->hdc);
}