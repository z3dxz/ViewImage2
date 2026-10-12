#pragma once

struct ImageUpdateRgn {
    bool valid;
    bool all;
    int x;
    int y;
    int width;
    int height;
};

void RedrawSurface(GlobalParams* m, bool image_update = false, ImageUpdateRgn rgn = {0, 0, 0, 0, 0, 0});
void PerformRedraw(GlobalParams* m);

void RedrawForce(GlobalParams* m, bool image_update = false, ImageUpdateRgn rgn = {0, 0, 0, 0, 0, 0});


GLAtlasTxt LoadOpenGLImage(void* memory, int size);

// Function prototypes
void SetupPixelFormat(GlobalParams* m);
void InitializeOpenGL(GlobalParams* m);
void CleanupOpenGL(GlobalParams* m);