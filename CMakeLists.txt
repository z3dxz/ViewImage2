include(FetchContent)

add_compile_definitions(_SVG)

cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(viewimage VERSION 2.6)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

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

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.c "res/View Image.rc")

find_library(SHLWAPI_LIB shlwapi)
find_package(OpenGL REQUIRED)

configure_file(res/OCRAEXT.ttf OCRAEXT.ttf COPYONLY)
configure_file(res/segoeui.ttf segoeui.ttf COPYONLY)
configure_file(res/verdana.ttf verdana.ttf COPYONLY)

if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    add_compile_options(-ffunction-sections -fdata-sections)
    add_link_options(-Wl,--gc-sections)
    add_link_options(-Wl,--as-needed)
    add_link_options(-s)
endif()

find_package(Freetype REQUIRED)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
add_executable(viewimage ${SRC_FILES} )
else ()
set(CMAKE_CXX_FLAGS_DEBUG "-g")
add_executable(viewimage WIN32 ${SRC_FILES} )
endif ()

string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S")
target_compile_definitions(viewimage PRIVATE BUILD_TYPE="$<CONFIG>" BUILD_SYSTEM="CMake")

set_target_properties(viewimage PROPERTIES OUTPUT_NAME "ViewImage")

# Link the library to the target
target_link_libraries(viewimage PRIVATE ${SHLWAPI_LIB} Freetype::Freetype OpenGL::GL)
target_link_libraries(viewimage PRIVATE lunasvg)