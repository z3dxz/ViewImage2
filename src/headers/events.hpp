#pragma once
#include "globalvar.hpp"
#include "ops.hpp"
#include "rendering.hpp"
#include "imgload.hpp"
#include "leftrightlogic.hpp"
#include <string>
#include "../effectdialog/headers/resizedialog.hpp"

#include "../effectdialog/headers/brightnesscontrast.h"
#include "../effectdialog/headers/gaussian.h"
#include "../effectdialog/headers/drawtext.h"

uint32_t PickColorFromDialog(GlobalParams* m, uint32_t def, bool* success);

void ResetCoordinates(GlobalParams* m);

bool Initialization(GlobalParams* m, int argc, LPWSTR* argv);
bool MouseDown(GlobalParams* m);
void MouseMove(GlobalParams* m, bool isCalledWhenMouseAcuallyMoved = true);
void KeyDown(GlobalParams* m, WPARAM wparam, LPARAM lparam);
void MouseUp(GlobalParams* m);
void RightDown(GlobalParams* m);
void RightUp(GlobalParams* m);
void MiddleDown(GlobalParams* m);
void MiddleUp(GlobalParams* m);
void Size(GlobalParams* m);
void MouseWheel(GlobalParams* m, WPARAM wparam, LPARAM lparam);

void ToggleFullscreen(GlobalParams* m);

void PerformWASDMagic(GlobalParams* m);

void createUndoStep(GlobalParams* m, bool async);
void ShowMyInformation(GlobalParams* m);


void TurnOnDraw(GlobalParams* m);
void TurnOffDraw(GlobalParams* m);