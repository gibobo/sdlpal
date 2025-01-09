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
#include "mini_glloader.h"
#include "palcfg.h"
#include "video.h"
#include <SDL_render.h>
#include <assert.h>

extern SDL_Renderer *gpRenderer;

static uint32_t gProgramId;
static int position = -1;
static int texcoord = -1;
static int texture = -1;
static int glversion_major, glversion_minor;
static int glslversion_major, glslversion_minor;
static const float p_vex[] = {-1, 1, 0, -1, -1, 0, 1, 1, 0, 1, -1, 0};
static const float p_tex[] = {0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0};

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

#ifdef GLES
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

    return programId;
}

void VIDEO_GLSL_Setup(const char * rendererName) {

    char *glversion = (char*)glGetString(GL_VERSION);
    char *glslversion = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    
#ifdef GLES
    if(!strncmp(glversion, "OpenGL ES", 9)) {
        SDL_sscanf(glversion, "OpenGL ES %d.%d", &glversion_major, &glversion_minor);
    }
    if(!strncmp(glslversion, "OpenGL ES GLSL ES", 17)) {
        SDL_sscanf(glslversion, "OpenGL ES GLSL ES %d.%d", &glslversion_major, &glslversion_minor);
    }
#else
    SDL_sscanf(glversion, "%d.%d", &glversion_major, &glversion_minor);
#endif

    SDL_sscanf(glslversion, "%d.%d", &glslversion_major, &glslversion_minor);

    if(!strncmp(rendererName, "opengl", 6)) {
        assert(initGLExtensions(glversion_major));
	}
    if( glversion_major >= 3 ) {
        GLint n;
        glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    }

    gProgramId = compileProgram(gConfig.pszShader, gConfig.pszShader, FALSE);

    position = glGetAttribLocation(gProgramId, "VertexCoord");
    glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, 0, p_vex);

    texcoord = glGetAttribLocation(gProgramId, "TexCoord");
    glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE, 0, p_tex);

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 320, 200, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void VIDEO_GLSL_RenderCopy(void *data) {
    int w, h;
    SDL_GetRendererOutputSize(gpRenderer, &w, &h);
    glViewport(0, 0, w, h);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 320, 200, GL_RGB, GL_UNSIGNED_BYTE, data);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glUseProgram(gProgramId);

    glEnableVertexAttribArray(texcoord);
    glEnableVertexAttribArray(position);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(position);
    glDisableVertexAttribArray(texcoord);
    glUseProgram(0);
}
