#pragma once

void RedrawSurface(GlobalParams* m, bool image_update = false);
void PerformRedraw(GlobalParams* m);

// Function prototypes
void SetupPixelFormat(GlobalParams* m);
void InitializeOpenGL(GlobalParams* m);
void CleanupOpenGL(GlobalParams* m);