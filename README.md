ViewImage or "View Image" is a powerful, yet lightweight image viewer designed and targeted for Windows  

<img width="1016" height="601" alt="demo" src="https://github.com/user-attachments/assets/0c9f1466-7a5a-4fb2-aeb9-81460a63ac77" />

# Features
* Snappy image controls + zoom
* Draw mode
  - Color, size, opacity (alpha blending), eyedropper, and softness
* Automatic Adjustment
* Brightness/Contrast
* Invert Colors
* Gaussian Blur
* Draw Text
* Crop Image
* Delete, print, copy, and paste
* Zoom auto / Zoom 1:1 scale
* Lightweight and performant
* File support
  - Uses stb_image formats: JPEG, PNG, TGA, BMP, PSD (composited only), static GIF, HDR (only sRGB), PIC, and PNM
  - Specialty supports include SVG, SFBB, M45, and IRBO

# Why
* Starts up fast
* User interface has **no** **popups, clutter, ads, or any annoyances or bloatware**
* Has lots of features

# Limitations
* Limited file support (sfb_image only supports so much)
* Standard dynamic range
* Can not scale, no hi-dpi support
* No linux support
* No hardware acceleration
I hope to solve these limitations in upcoming releases

# Compilation
## Requirements / Dependencies:
  - Win32 API (Windows XP or above supported)
  - LunaSVG for optional SVG support (_SVG compile flag in CMakeLists)
  - Freetype w/ TBB
## How to compile / How to use CMake
  ```
  mkdir build
  cd build
  cmake ..
  make
  ```
replace make with ninja if desired  
