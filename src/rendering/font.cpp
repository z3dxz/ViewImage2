#include "font.hpp"
#include <cmath>

// referenced from https://gist.github.com/baines/b0f9e4be04ba4e6f56cab82eef5008ff

#define NUM_GLYPHS 128

struct glyph_info {
	int x0, y0, x1, y1;
	int x_off, y_off;
	float advance;
};

struct atlas {
    int width=0;
    int height=0;
    glyph_info info[NUM_GLYPHS];
    GLuint gltxt=0;
    int fontsize=0;
    FT_Face face=0;
};

float uiscale;

std::vector<atlas*> atlases;

bool gen_atlas_element(const FT_Face face, int font_size, atlas* out) {

    if(!face) return false;

    FT_Set_Pixel_Sizes(face, 0, font_size);

	int max_dim = (1 + (face->size->metrics.height >> 6)) * ceilf(sqrtf(NUM_GLYPHS));
	int tex_width = 1;
	while(tex_width < max_dim) tex_width <<= 1;
	int tex_height = tex_width;

	char* pixels = (char*)calloc(tex_width * tex_height, 1);
    if(!pixels) {
        MessageBox(0, "Not enough memory", "Text Atlas generation failed", MB_OK | MB_ICONERROR);
        return false;
    }
	int pen_x = 0, pen_y = 0;

    for(int i = 0; i < NUM_GLYPHS; i++){
		FT_Load_Char(face, i, FT_LOAD_RENDER); 
		FT_Bitmap* bmp = &face->glyph->bitmap;

		if(pen_x + bmp->width >= tex_width){
			pen_x = 0;
			pen_y += ((face->size->metrics.height >> 6) + 1);
		}

		for(int row = 0; row < bmp->rows; ++row){
			for(int col = 0; col < bmp->width; ++col){
				int x = pen_x + col;
				int y = pen_y + row;
				pixels[y * tex_width + x] = bmp->buffer[row * bmp->pitch + col];
			}
		}

        out->info[i].x0 = pen_x;
        out->info[i].y0 = pen_y;
        out->info[i].x1 = pen_x + bmp->width;
        out->info[i].y1 = pen_y + bmp->rows;
        out->info[i].x_off   = face->glyph->bitmap_left;
        out->info[i].y_off   = face->glyph->bitmap_top;
        
        out->info[i].advance = (float)face->glyph->advance.x / 64.0f;
        pen_x += bmp->width + 1;
    }

    out->width = tex_width;
    out->height = tex_height;
    out->fontsize = font_size;
    out->face = face;

    if (out->gltxt) glDeleteTextures(1, &out->gltxt);
    glGenTextures(1, &out->gltxt);
    glBindTexture(GL_TEXTURE_2D, out->gltxt);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA8, tex_width, tex_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width, tex_height, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    free(pixels);

    return true;
}

float uiuncomp(GLuint input){
    return input;//(input/uiscale)+0.5f;
    // not used anymore
}

void RenderStringLoop(atlas* current_atlas, FT_Face face, const char* inputstr, float startX, float locY, int size) {
    float x = startX;
    bool has_kerning = FT_HAS_KERNING(face);
    uint32_t previous_glyph_index = 0;

    for(const char* p = inputstr; *p; p++){
        uint8_t ch = (uint8_t)(*p);
        if(ch >= NUM_GLYPHS) continue;

        uint32_t current_glyph_index = FT_Get_Char_Index(face, ch);
        if (has_kerning && previous_glyph_index && current_glyph_index) {
            FT_Vector delta;
            FT_Get_Kerning(face, previous_glyph_index, current_glyph_index, FT_KERNING_DEFAULT, &delta);
            x += (float)delta.x / 64.0f;
        }
        previous_glyph_index = current_glyph_index;

        glyph_info& gi = current_atlas->info[ch];

        float w = (float)(gi.x1 - gi.x0);
        float h = (float)(gi.y1 - gi.y0);

        float xpos = floorf(x) + (float)gi.x_off;
        float ypos_top    = (float)locY + (float)size - (float)gi.y_off;
        float ypos_bottom = ypos_top + h;

        float u0 = (float)(gi.x0) / current_atlas->width;
        float v0 = (float)(gi.y0) / current_atlas->height;
        float u1 = (float)(gi.x1) / current_atlas->width;
        float v1 = (float)(gi.y1) / current_atlas->height;


        GLfloat vertices[] = {
            uiuncomp(xpos),     uiuncomp(ypos_top),
            uiuncomp(xpos + w), uiuncomp(ypos_top),
            uiuncomp(xpos + w), uiuncomp(ypos_bottom),
            uiuncomp(xpos),     uiuncomp(ypos_bottom)
        };

            
        GLfloat texCoords[] = {
            u0, v0,
            u1, v0,
            u1, v1,
            u0, v1
        };
        
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, vertices);
        glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

        glDrawArrays(GL_QUADS, 0, 4);

        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);

        x += floorf(gi.advance);
    }
}

void PlaceStringBuf(FT_Face face, int size0, const char* inputstr, uint32_t locX0, uint32_t locY0, uint32_t color, float uiscale0, bool shadow){
    uiscale = uiscale0;
    int locX = locX0 * uiscale;
    int locY = locY0 * uiscale;

    locY-=1;

    int size = size0*uiscale;

    atlas* current_atlas = nullptr;

    for(atlas* at : atlases) {
        if(at->fontsize == size && at->face == face) {
            current_atlas = at;
            break;
        }
    }

    if(!current_atlas) {
        if (atlases.size() >= 16) { // cache limit
            atlas* oldest = atlases.front();
            if (oldest->gltxt) {
                glDeleteTextures(1, &oldest->gltxt);
            }
            delete oldest;
            atlases.erase(atlases.begin());
        }

        atlas* atl = new atlas;
        if(!gen_atlas_element(face, size, atl)){
            printf("Font error!\n");
            delete atl;
            return;
        }

        atlases.push_back(atl);
        current_atlas = atl;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, current_atlas->gltxt);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Extract original colors and alpha
    uint8_t a = (color >> 24) & 0xFF; 
    uint8_t r = (color >> 16) & 0xFF; 
    uint8_t g = (color >> 8)  & 0xFF; 
    uint8_t b = (color)       & 0xFF;

    float base_shadow_alpha = (a / 255.0f) * 0.18f;

    if(shadow) {
        struct ShadowPass { float dx; float dy; float alpha_mult; };
        ShadowPass shadow_passes[] = {
            // Harder core shadow
            { 1.0f,  1.0f,  1.00f },
            { 2.0f,  2.0f,  0.75f },
            
            // Mid-distance soft shadow
            { 0.0f,  2.0f,  0.50f },
            { 2.0f,  0.0f,  0.50f },
            { 3.0f,  3.0f,  0.40f },
            
            // Far outer glow/blur ring
            { -1.0f, 3.0f,  0.25f },
            {  3.0f, 1.0f,  0.25f },
            {  1.0f, 4.0f,  0.20f },
            {  4.0f, 2.0f,  0.20f }
        };

        for(const auto& pass : shadow_passes) {

            uint8_t shadow_a = (uint8_t)(base_shadow_alpha * pass.alpha_mult * 255.0f);
            
            glColor4ub(0, 0, 0, shadow_a); 
            
            RenderStringLoop(current_atlas, face, inputstr, (float)locX + pass.dx, (float)locY + pass.dy, size);
        }
    }

    // base text
    glColor4ub(r, g, b, a);
    RenderStringLoop(current_atlas, face, inputstr, (float)locX, (float)locY, size);

    glBindTexture(GL_TEXTURE_2D, 0);
}