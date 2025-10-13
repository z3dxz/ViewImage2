include(FetchContent)

cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(viewimage VERSION 2.6)


set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

# --------------------------- LunaSVG ---------------------------
set(LUNASVG_VERSION 3.4.0)
FetchContent_Declare(
	lunasvg
	URL https://github.com/sammycage/lunasvg/archive/refs/tags/v${LUNASVG_VERSION}.zip
	FIND_PACKAGE_ARGS ${LUNASVG_VERSION}
)
option(LUNASVG_BUILD_EXAMPLES "Build examples" OFF)
FetchContent_MakeAvailable(lunasvg)
#----------------------------------------------------------------

file(GLOB_RECURSE SRC_FILES src/*.cpp "res/View Image.rc")

find_library(COMCTL32_LIB comctl32)
find_library(COMCTL32_LIB comctl32)
find_library(UXTHEME_LIB uxtheme)
find_library(SHLWAPI_LIB shlwapi)
find_library(WINMM winmm)

find_package(Freetype REQUIRED)


find_package(TBB REQUIRED)

add_executable(viewimage WIN32 ${SRC_FILES} )
set_target_properties(viewimage PROPERTIES OUTPUT_NAME "viewimage-2.5-x64-cm")

# Link the library to the target
target_link_libraries(viewimage PRIVATE ${COMCTL32_LIB} ${UXTHEME_LIB} ${SHLWAPI_LIB} ${WINMM} TBB::tbb Freetype::Freetype  lunasvg)