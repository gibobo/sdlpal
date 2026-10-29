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
#include "../src/driver.h"
#include "../src/video.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_GLLOADER_SDL)
#include "mini_glloader.h"
#elif defined(_GLLOADER_GLFW)
#define GLAD_GLES2_IMPLEMENTATION
#include <glad/gles2.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#endif

static uint32_t gProgramId = 0;
static int position = -1;
static int texcoord = -1;
static int texture = -1;
static int window_width = 0;
static int window_height = 0;
static const float p_vex[] = {-1, 1, 0, -1, -1, 0, 1, 1, 0, 1, -1, 0};
static const float p_tex[] = {0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0};

// Function prototype
char *readShaderFile(const char *filename);
char *skip_version(char *src);
GLuint compileShader(const char *sourceOrFilename, GLuint shaderType, int is_source);
GLuint compileProgram(const char *vtx, const char *frag, int is_source);

char *readShaderFile(const char *filename)
{
    int64_t filesize = 0;
    char *buf = NULL;
    FILE *fp = NULL;
    if ((fp = fopen(filename, "rb")))
    {
        fseek(fp, 0, SEEK_END);
        filesize = ftell(fp);
        buf = (char *)malloc((size_t)filesize + 1);
        fseek(fp, 0, SEEK_SET);
        fread(buf, (size_t)filesize, 1, fp);
        fclose(fp);
        buf[filesize] = '\0';
    }
    return buf;
}

char *skip_version(char *src)
{
    int32_t glslVersion = -1;
    sscanf(src, "#version %d", &glslVersion);
    if (glslVersion != -1)
    {
        char *eol = strstr(src, "\n");
        for (int i = 0; i < eol - src; i++)
            src[i] = ' ';
    }
    return src;
}

GLuint compileShader(const char *sourceOrFilename, GLuint shaderType, int is_source)
{
#define SHADER_TYPE(shaderType) (shaderType == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
    char *pShaderBuffer;
    char *source = (is_source) ? (char *)sourceOrFilename : readShaderFile(sourceOrFilename);
    uint32_t sourceLen = (uint32_t)strlen(source) * 2U;
    pShaderBuffer = (char *)malloc(sourceLen);
    memset(pShaderBuffer, 0, sourceLen);

    snprintf(pShaderBuffer, sourceLen, "#define %s\r\n%s\r\n", SHADER_TYPE(shaderType), is_source ? source : skip_version(source));
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
            GLchar *log = (GLchar *)malloc((size_t)logLength);
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

void VIDEO_GLSL_Initialize(uint32_t width, uint32_t height)
{
    window_width = width;
    window_height = height;

    if (gProgramId)
        return;
#if defined(_GLLOADER_SDL)
    assert(initGLExtensions(2));
#elif defined(_GLLOADER_GLFW)
    gladLoadGLES2(glfwGetProcAddress);
#endif

    char *pszShader = strdup("shaders/plain.glsl");
    gProgramId = compileProgram(pszShader, pszShader, 0);
    free(pszShader);

    position = glGetAttribLocation(gProgramId, "VertexCoord");
    glVertexAttribPointer((GLuint)position, 3, GL_FLOAT, GL_FALSE, 0, p_vex);

    texcoord = glGetAttribLocation(gProgramId, "TexCoord");
    glVertexAttribPointer((GLuint)texcoord, 2, GL_FLOAT, GL_FALSE, 0, p_tex);

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, (GLuint *)&texture);
    glBindTexture(GL_TEXTURE_2D, (GLuint)texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_W, SCREEN_H, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
}

void VIDEO_GLSL_RenderCopy(const void *data)
{
    glViewport(0, 0, window_width, window_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    if (data)
    {
        glBindTexture(GL_TEXTURE_2D, (GLuint)texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_W, SCREEN_H, GL_RGB, GL_UNSIGNED_BYTE, data);
    }

    glUseProgram(gProgramId);

    glEnableVertexAttribArray((GLuint)texcoord);
    glEnableVertexAttribArray((GLuint)position);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray((GLuint)position);
    glDisableVertexAttribArray((GLuint)texcoord);
    glUseProgram(0);
}

void VIDEO_GLSL_Destroy(void)
{
    if (gProgramId != 0)
        glDeleteProgram(gProgramId);

    if (texture != -1)
        glDeleteTextures(1, (GLuint *)&texture);

    window_width = 0;
    window_height = 0;
    gProgramId = 0;
    texture = -1;
    position = -1;
    texcoord = -1;
}
