#include "rgblur.hpp"

#define GL_FRAGMENT_SHADER    0x8B30
#define GL_COMPILE_STATUS     0x8B81
#define GL_LINK_STATUS        0x8B82
#define GL_CLAMP_TO_EDGE      0x812F
#define GL_RGBA8              0x8058

typedef char GLchar;
typedef GLuint (WINAPI * PFNGLCREATESHADERPROC) (GLenum type);
typedef void (WINAPI * PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar* *string, const GLint *length);
typedef void (WINAPI * PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (WINAPI * PFNGLCREATEPROGRAMPROC) (void);
typedef void (WINAPI * PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (WINAPI * PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (WINAPI * PFNGLUSEPROGRAMPROC) (GLuint program);
typedef GLint (WINAPI * PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (WINAPI * PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
typedef void (WINAPI * PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);
typedef void (WINAPI * PFNGLUNIFORM2FPROC) (GLint location, GLfloat v0, GLfloat v1);
typedef void (WINAPI * PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (WINAPI * PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (WINAPI * PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (WINAPI * PFNGLDELETEPROGRAMPROC) (GLuint program);

static PFNGLCREATESHADERPROC    glCreateShader_ptr    = nullptr;
static PFNGLSHADERSOURCEPROC    glShaderSource_ptr    = nullptr;
static PFNGLCOMPILESHADERPROC   glCompileShader_ptr   = nullptr;
static PFNGLCREATEPROGRAMPROC   glCreateProgram_ptr   = nullptr;
static PFNGLATTACHSHADERPROC    glAttachShader_ptr    = nullptr;
static PFNGLLINKPROGRAMPROC     glLinkProgram_ptr     = nullptr;
static PFNGLUSEPROGRAMPROC      glUseProgram_ptr      = nullptr;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation_ptr = nullptr;
static PFNGLUNIFORM1IPROC       glUniform1i_ptr       = nullptr;
static PFNGLUNIFORM1FPROC       glUniform1f_ptr       = nullptr;
static PFNGLUNIFORM2FPROC       glUniform2f_ptr       = nullptr;
static PFNGLGETSHADERIVPROC     glGetShaderiv_ptr     = nullptr;
static PFNGLGETPROGRAMIVPROC    glGetProgramiv_ptr    = nullptr;
static PFNGLDELETESHADERPROC    glDeleteShader_ptr    = nullptr;
static PFNGLDELETEPROGRAMPROC   glDeleteProgram_ptr   = nullptr;

static bool g_BlurInitialized = false;
static bool g_GL20Supported   = false;
static GLuint g_BlurProgram   = 0;
static GLint g_BlurTexLoc       = -1;
static GLint g_BlurTexelSizeLoc = -1;
static GLint g_BlurSigmaLoc     = -1;
static GLint g_BlurMaxUVLoc     = -1;

static GLuint CompileBlurShader() {
    const char* fsSource = R"(
        uniform sampler2D u_texture;
        uniform vec2 u_texelSize;
        uniform float u_sigma;
        uniform vec2 u_maxUV;
        vec4 sampleTex(vec2 uv) {
            if (uv.x < 0.0 || uv.x > u_maxUV.x || uv.y < 0.0 || uv.y > u_maxUV.y) {
                return vec4(0.4, 0.4, 0.4, 1.0);
            }
            return texture2D(u_texture, uv);
        }
        void main() {
            vec2 uv = gl_TexCoord[0].st;
            vec4 col = vec4(0.0);
            float accum = 0.0;
            for (int x = -4; x <= 4; x++) {
                for (int y = -4; y <= 4; y++) {
                    vec2 offset = vec2(float(x), float(y)) * u_texelSize;
                    float weight = exp(-(float(x*x + y*y)) / (2.0 * u_sigma * u_sigma));
                    col += sampleTex(uv + offset) * weight;
                    accum += weight;
                }
            }
            gl_FragColor = col / accum;
        }
    )";

    GLuint fs = glCreateShader_ptr(GL_FRAGMENT_SHADER);
    glShaderSource_ptr(fs, 1, &fsSource, nullptr);
    glCompileShader_ptr(fs);

    GLint compiled = 0;
    glGetShaderiv_ptr(fs, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        glDeleteShader_ptr(fs);
        return 0;
    }


    GLuint prog = glCreateProgram_ptr();
    
    glAttachShader_ptr(prog, fs);
    glLinkProgram_ptr(prog);
    GLint linked = 0;
    glGetProgramiv_ptr(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        glDeleteShader_ptr(fs);
        glDeleteProgram_ptr(prog);
        return 0;
    }

    glDeleteShader_ptr(fs);
    return prog;
}

static void TryInitializeGL20() {
    if (g_BlurInitialized) return;
    g_BlurInitialized = true;

    
    const GLubyte* versionString = glGetString(GL_VERSION);
    std::cout << "Supported OpenGL Version: " << versionString << "\n";
    int major = versionString[0] - '0'; 
    if (major >= 2) {
        std::cout << "OpenGL 2.0+ blur enabled\n";
    } else {
        std::cout << "Blur not supported\n";
        return;
    }


    glCreateShader_ptr    = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
    glShaderSource_ptr    = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
    glCompileShader_ptr   = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
    glCreateProgram_ptr   = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
    glAttachShader_ptr    = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
    glLinkProgram_ptr     = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
    glUseProgram_ptr      = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    glGetUniformLocation_ptr = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
    glUniform1i_ptr       = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
    glUniform1f_ptr       = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
    glUniform2f_ptr       = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
    glGetShaderiv_ptr     = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
    glGetProgramiv_ptr    = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
    glDeleteShader_ptr    = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
    glDeleteProgram_ptr   = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");

    if (glCreateShader_ptr && glShaderSource_ptr && glCompileShader_ptr &&
        glCreateProgram_ptr && glAttachShader_ptr && glLinkProgram_ptr &&
        glUseProgram_ptr && glGetUniformLocation_ptr && glUniform1i_ptr &&
        glUniform1f_ptr && glUniform2f_ptr && glGetShaderiv_ptr && 
        glGetProgramiv_ptr && glDeleteShader_ptr && glDeleteProgram_ptr) 
    {
        g_BlurProgram = CompileBlurShader();
        if (g_BlurProgram != 0) {
            g_BlurTexLoc       = glGetUniformLocation_ptr(g_BlurProgram, "u_texture");
            g_BlurTexelSizeLoc = glGetUniformLocation_ptr(g_BlurProgram, "u_texelSize");
            g_BlurSigmaLoc     = glGetUniformLocation_ptr(g_BlurProgram, "u_sigma");
            g_BlurMaxUVLoc     = glGetUniformLocation_ptr(g_BlurProgram, "u_maxUV");
                g_GL20Supported = true;
        }
    }
}

void gaussian_blur_render(GlobalParams* m, int lW, int lH, double sigma, uint32_t offX, uint32_t offY) {
    TryInitializeGL20();
    if (!g_GL20Supported || lW <= 0 || lH <= 0) return;

    float scale = m->uiscale;
    lW *= m->uiscale;
    lH *= m->uiscale;
    offX *= m->uiscale;
    offY *= m->uiscale;


    int pixelX = (int)(offX );
    int pixelY = (int)((m->rlheight - ((int)offY + lH)));
    int pixelW = (int)(lW);
    int pixelH = (int)(lH);

    if (pixelW <= 0 || pixelH <= 0) return;

    int texW = 1; while (texW < pixelW) texW <<= 1;
    int texH = 1; while (texH < pixelH) texH <<= 1;

    float maxU = (float)pixelW / (float)texW;
    float maxV = (float)pixelH / (float)texH;

    if (sigma <= 0.1) sigma = 0.1;

    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pixelX, pixelY, pixelW, pixelH);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    glUseProgram_ptr(g_BlurProgram);
    glUniform1i_ptr(g_BlurTexLoc, 0);
    glUniform1f_ptr(g_BlurSigmaLoc, (float)sigma);
    glUniform2f_ptr(g_BlurTexelSizeLoc, 1.0f / (float)texW, 1.0f / (float)texH);
    glUniform2f_ptr(g_BlurMaxUVLoc, maxU, maxV);

    GLfloat screenVertices[] = {
        (GLfloat)offX,        (GLfloat)offY,
        (GLfloat)offX + lW,   (GLfloat)offY,
        (GLfloat)offX + lW,   (GLfloat)offY + lH,
        (GLfloat)offX,        (GLfloat)offY + lH
    };

    GLfloat screenTexCoords[] = {
        0.0f, maxV,
        maxU, maxV,
        maxU, 0.0f,
        0.0f, 0.0f
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, screenVertices);
    glTexCoordPointer(2, GL_FLOAT, 0, screenTexCoords);
    
    glDrawArrays(GL_QUADS, 0, 4);

    glUseProgram_ptr(0);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glPopAttrib();

    glDeleteTextures(1, &texture);
}