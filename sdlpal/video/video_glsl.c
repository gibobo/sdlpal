/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2011-2024, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// video_glsl.c: hacky SDL2 renderer that compatible of retroarch-style
// multipass shader preset by palxex, 2018
//

#include "video_glsl.h"
#include "common.h"
#include "mini_glloader.h"
#include "palcfg.h"
#include "util.h"
#include "video.h"
#include <SDL_render.h>
#include <SDL_hints.h>

#define MID_GLSLP "sdlpal.glslp"

extern SDL_Window        *gpWindow;
extern PAL_Surface       *gpScreenReal;
extern SDL_Renderer      *gpRenderer;

static uint32_t gProgramIds[2]={-1};
static uint32_t gVBOId;
static uint32_t gEBOId;
static int glversion_major, glversion_minor;
static int glslversion_major, glslversion_minor;

static SDL_Texture *origTexture;
static SDL_Texture *framePrevTextures = NULL;

struct VertexDataFormat {
    GLfloat position[3];
    GLfloat texCoord[2];
};

static char *plain_glsl_vert =
"#if __VERSION__ >= 130\n"
"   #define COMPAT_VARYING out\n"
"   #define COMPAT_ATTRIBUTE in\n"
"   #define COMPAT_TEXTURE texture\n"
"#else\n"
"   #define COMPAT_VARYING varying\n"
"   #define COMPAT_ATTRIBUTE attribute\n"
"   #define COMPAT_TEXTURE texture2D\n"
"#endif\n"
"#ifdef GL_ES\n"
"   #define COMPAT_PRECISION mediump\n"
"#else\n"
"   #define COMPAT_PRECISION\n"
"#endif\n"
"COMPAT_ATTRIBUTE vec4 VertexCoord;\n"
"COMPAT_ATTRIBUTE vec2 TexCoord;\n"
"COMPAT_VARYING vec2 v_texCoord;\n"
"void main() {\n"
"   gl_Position = VertexCoord;\n"
"   v_texCoord = TexCoord;\n"
"}\n";

static char *plain_glsl_frag =
"#if __VERSION__ >= 130\n"
"    #define COMPAT_VARYING in\n"
"    #define COMPAT_TEXTURE texture\n"
"    out vec4 FragColor;\n"
"#else\n"
"    #define COMPAT_VARYING varying\n"
"    #define COMPAT_TEXTURE texture2D\n"
"    #define FragColor gl_FragColor\n"
"#endif\n"
""
"#ifdef GL_ES\n"
"    #ifdef GL_FRAGMENT_PRECISION_HIGH\n"
"        precision highp float;\n"
"    #else\n"
"        precision mediump float;\n"
"    #endif\n"
"    #define COMPAT_PRECISION mediump\n"
"#else\n"
"    #define COMPAT_PRECISION\n"
"#endif\n"
""
"COMPAT_VARYING vec2 v_texCoord;\n"
"uniform sampler2D tex0;\n"
""
"const float SRGB_ALPHA = 0.055;\n"
"float linear_to_srgb(float channel) {\n"
"    if(channel <= 0.0031308)\n"
"        return 12.92 * channel;\n"
"    else\n"
"        return (1.0 + SRGB_ALPHA) * pow(channel, 1.0/2.4) - SRGB_ALPHA;\n"
"}\n"
""
"vec3 rgb_to_srgb(vec3 rgb) {\n"
"    return vec3(linear_to_srgb(rgb.r), linear_to_srgb(rgb.g), linear_to_srgb(rgb.b)    );\n"
"}\n"
""
"float srgb_to_linear(float channel) {\n"
"    if (channel <= 0.04045)\n"
"        return channel / 12.92;\n"
"    else\n"
"        return pow((channel + SRGB_ALPHA) / (1.0 + SRGB_ALPHA), 2.4);\n"
"}\n"
""
"vec3 srgb_to_rgb(vec3 srgb) {\n"
"    return vec3(srgb_to_linear(srgb.r),    srgb_to_linear(srgb.g),    srgb_to_linear(srgb.b));\n"
"}\n"
""
"void main() {\n"
"    vec4 srgb = COMPAT_TEXTURE(tex0 , v_texCoord.xy);\n"
"    FragColor = vec4(srgb_to_rgb(srgb.rgb), srgb.a);\n"
"#ifdef GL_ES\n"
"    FragColor.rgb = FragColor.bgr;\n"
"#endif\n"
"    vec3 color = FragColor.rgb;\n"
"    color = rgb_to_srgb(color);\n"
"    FragColor.rgb=color;\n"
"}\n";

static char *glslp_template = 
"orig_filter = %s\n"
"shaders = 1\n"
"shader0 = %s\n"
"scale_type0 = absolute\n"
"scale_x0 = %d\n"
"scale_y0 = %d\n"
"filter_linear0 = %s\n";

char *readShaderFile(const char *filename, GLuint type)
{
    FILE *fp = UTIL_OpenRequiredFileForMode(filename, "rb");
    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    char *buf = (char *)malloc(filesize + 1);
    fseek(fp, 0, SEEK_SET);
    fread(buf, filesize, 1, fp);
    fclose(fp);
    buf[filesize] = '\0';
    return buf;
}

char *skip_version(char *src) {
    int glslVersion = -1;
    SDL_sscanf(src, "#version %d", &glslVersion);
    if( glslVersion != -1 ){
        char *eol = strstr(src, "\n");
        for(int i = 0; i < eol-src; i++)
            src[i]=' ';
    }
    return src;
}

GLuint compileShader(const char *sourceOrFilename, GLuint shaderType, int is_source)
{
#define SHADER_TYPE(shaderType) (shaderType == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
    char *pShaderBuffer;
    char *source = (is_source) ? (char *)sourceOrFilename : readShaderFile(sourceOrFilename, shaderType);
    int lines = -1;
    size_t sourceLen = strlen(source) * 2;
    pShaderBuffer = malloc(sourceLen);
    memset(pShaderBuffer, 0, sourceLen);

#if GLES
    sprintf(pShaderBuffer, "#version %d%02d %s\r\n", glslversion_major, glslversion_minor, glslversion_major >= 3 ? "es" : "");
    lines++;
    if (SDL_GL_ExtensionSupported("GL_OES_standard_derivatives"))
    {
        sprintf(pShaderBuffer, "%s#extension GL_OES_standard_derivatives : enable\r\n", pShaderBuffer);
        lines++;
    }
    if (SDL_GL_ExtensionSupported("GL_EXT_shader_texture_lod"))
    {
        sprintf(pShaderBuffer, "%s#extension GL_EXT_shader_texture_lod : enable\r\n", pShaderBuffer);
        lines++;
    }

    // should be deduced via GL_ES/GL_FRAGMENT_PRECISION_HIGH combination since both is predefined
    // but unknown why manual define is a must for WebGL2
    if (glslversion_major >= 3)
    {
        sprintf(pShaderBuffer, "%sprecision highp float;\r\n", pShaderBuffer);
        lines++;
    }
#else
    sprintf(pShaderBuffer, "#version %d%02d\r\n", glslversion_major, glslversion_minor);
    lines++;
#endif

    sprintf(pShaderBuffer, "%s#line %d\r\n", pShaderBuffer, lines);
    sprintf(pShaderBuffer, "%s#define %s\r\n%s\r\n", pShaderBuffer, SHADER_TYPE(shaderType), is_source ? source : skip_version(source));
    if (!is_source)
        free((void *)source);

    // Create ID for shader
    GLuint result = glCreateShader(shaderType);
    // Define shader text
    glShaderSource(result, 1, (const GLchar *const *)&pShaderBuffer, NULL);
    // Compile shader
    glCompileShader(result);

    // Check vertex shader for errors
    GLint shaderCompiled = GL_FALSE;
    glGetShaderiv(result, GL_COMPILE_STATUS, &shaderCompiled);
    if (shaderCompiled != GL_TRUE)
    {
        GLint logLength;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0)
        {
            GLchar *log = (GLchar *)malloc(logLength);
            glGetShaderInfoLog(result, logLength, &logLength, log);
            free(log);
        }
        glDeleteShader(result);
        result = 0;
    }
    free(pShaderBuffer);
    return result;
}

GLuint compileProgram(const char *vtx, const char *frag, int is_source)
{
    GLuint programId = glCreateProgram();
    GLuint vtxShaderId = compileShader(vtx, GL_VERTEX_SHADER, is_source);
    GLuint fragShaderId = compileShader(frag, GL_FRAGMENT_SHADER, is_source);

    if (vtxShaderId && fragShaderId)
    {
        // Associate shader with program
        glAttachShader(programId, vtxShaderId);
        glAttachShader(programId, fragShaderId);
        glLinkProgram(programId);
        glValidateProgram(programId);

        // // Check the status of the compile/link
        GLint programLinked = GL_FALSE;
        glGetProgramiv(programId, GL_LINK_STATUS, &programLinked);
        assert(programLinked == GL_TRUE);
    }

    if (vtxShaderId)
        glDeleteShader(vtxShaderId);

    if (fragShaderId)
        glDeleteShader(fragShaderId);

    int position = glGetAttribLocation(programId, "VertexCoord");
    if (position >= 0)
    {
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(struct VertexDataFormat), (GLvoid *)offsetof(struct VertexDataFormat, position));
    }

    int texcoord = glGetAttribLocation(programId, "TexCoord");
    if (texcoord >= 0)
    {
        glEnableVertexAttribArray(texcoord);
        glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(struct VertexDataFormat), (GLvoid *)offsetof(struct VertexDataFormat, texCoord));
    }
    return programId;
}

int VIDEO_RenderTexture(int pass)
{
    // get needed uniform locations
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(gProgramIds[pass]);

    glBindBuffer(GL_ARRAY_BUFFER, gVBOId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gEBOId);

    // Draw quad using vertex data and index data
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, NULL);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glUseProgram(0);

    return 0;
}

void *VIDEO_GLSL_CreateTexture()
{
    // Recreate textures
    if( framePrevTextures )
        SDL_DestroyTexture(framePrevTextures);
    framePrevTextures = SDL_CreateTexture(gpRenderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, gConfig.dwTextureWidth, gConfig.dwTextureHeight);

    return (void *)framePrevTextures;
}

void VIDEO_GLSL_RenderCopy()
{
    SDL_Texture *gpTexture;
    gpTexture = framePrevTextures; //...
    if (gpTexture == NULL)
        return;

    origTexture = SDL_CreateTextureFromSurface(gpRenderer, (SDL_Surface *)gpScreenReal);
    SDL_GL_BindTexture(origTexture, NULL, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    int w,h;
    SDL_GetRendererOutputSize(gpRenderer, &w, &h);
    glViewport(0, 0, w, h);

    SDL_Texture *prevTexture = origTexture;
    SDL_SetRenderTarget(gpRenderer, gpTexture);
    SDL_RenderClear(gpRenderer);
    
    SDL_GL_BindTexture(prevTexture, NULL, NULL);
    VIDEO_RenderTexture(1);
    SDL_DestroyTexture(origTexture);

    SDL_SetRenderTarget(gpRenderer, NULL);
    SDL_RenderClear(gpRenderer);
    SDL_GL_BindTexture(gpTexture, NULL, NULL);
    VIDEO_RenderTexture(0);

    SDL_GL_SwapWindowWithResult(gpWindow);
}

void VIDEO_GLSL_Init()
{
#if GLES
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
#if SDL_VIDEO_OPENGL_EGL && (SDL_VIDEO_DRIVER_EMSCRIPTEN || SDL_VIDEO_DRIVER_WINRT)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
#else
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
#endif
}

void VIDEO_GLSL_Setup() {
    SDL_RendererInfo rendererInfo;
    SDL_GetRendererInfo(gpRenderer, &rendererInfo);

    char *glversion = (char*)glGetString(GL_VERSION);
    char *glslversion = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    SDL_sscanf(glversion, "%d.%d", &glversion_major, &glversion_minor);
    if(!strncmp(rendererInfo.name, "opengl", 6)) {
        assert(initGLExtensions(glversion_major));
	}
    GLint maxTextureSize, maxDrawBuffers, maxColorAttachments;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    if( glversion_major >= 3 ) {
        GLint n;
        glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    }
    SDL_sscanf(glslversion, "%d.%d", &glslversion_major, &glslversion_minor);
    
    // iOS native GLES supports VAO extension
#if GLES
    if(!strncmp(glversion, "OpenGL ES", 9)) {
        SDL_sscanf(glversion, "OpenGL ES %d.%d", &glversion_major, &glversion_minor);
    }
    if(!strncmp(glslversion, "OpenGL ES GLSL ES", 17)) {
        SDL_sscanf(glslversion, "OpenGL ES GLSL ES %d.%d", &glslversion_major, &glslversion_minor);
    }
#endif
    char *origGLSL = NULL;



    //Create IBO
    GLuint iData[] = {0, 1, 3, 2,}; //Set rendering indices
    glGenBuffers(1, &gEBOId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gEBOId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), iData, GL_STATIC_DRAW);

    //Create VBO
    struct VertexDataFormat vData[] = {
        {.position = {-1.0,-1.0, 0.0}, .texCoord = {0.0, 0.0}},
        {.position = { 1.0,-1.0, 0.0}, .texCoord = {1.0, 0.0}},
        {.position = { 1.0, 1.0, 0.0}, .texCoord = {1.0, 1.0}},
        {.position = {-1.0, 1.0, 0.0}, .texCoord = {0.0, 1.0}},
    };
    glGenBuffers(1, &gVBOId);
    glBindBuffer(GL_ARRAY_BUFFER, gVBOId);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(struct VertexDataFormat), vData, GL_DYNAMIC_DRAW);

    gProgramIds[0] = compileProgram(plain_glsl_vert, plain_glsl_frag, TRUE);
    gProgramIds[1] = compileProgram(gConfig.pszShader, gConfig.pszShader, FALSE);
}

void VIDEO_GLSL_Destroy()
{
    if (framePrevTextures)
        SDL_DestroyTexture(framePrevTextures);
    framePrevTextures = NULL;
}
