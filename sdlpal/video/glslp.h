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
// glslp.h: retroarch-style shader preset parser header by palxex, 2018
//

#ifndef glslp_h
#define glslp_h

#include <SDL_render.h>

#define MAX_INDEX 2
#define MAX_PARAMETERS 200

enum wrap_mode {
    WRAP_REPEAT,
    WRAP_CLAMP_TO_EDGE,
    WRAP_CLAMP_TO_BORDER
};

enum scale_type {
    SCALE_SOURCE,
    SCALE_VIEWPORT,
    SCALE_ABSOLUTE
};

typedef struct tagSHADERPARAM {
    //by defination
    char *shader;
    char *alias;
    char filter_linear;
    enum wrap_mode wrap_mode;
    enum scale_type scale_type_x, scale_type_y;
    float scale_x, scale_y;
    char mipmap_input;
    char float_framebuffer;
    char srgb_framebuffer;
    int frame_count_mod;
}shader_param;

typedef struct tagTEXTUREPARAMS {
    //by defination
    char *texture_name;
    char *texture_path;
    enum wrap_mode wrap_mode;
    char linear;
    char mipmap;
    
    //by implementation
    SDL_Texture *sdl_texture;
    int texture_unit;
    int slots_pass[MAX_INDEX]; //corresponding every pass
}texture_param;

typedef struct tagUNIFORMPARAMS {
    //by defination
    char *parameter_name;
    char *desc;
    float value;
    float value_default;
    float minimum;
    float maximum;
    float step;
}uniform_param;

typedef struct tagGLSLP {
    char *orig_filter;
    int shaders;
    shader_param *shader_params;
    int textures;
    texture_param *texture_params;
}GLSLP;

extern GLSLP gGLSLP;

char *get_glslp_path(const char *filename);

char parse_glslp(const char *, GLSLP *);
char *serialize_glslp(const GLSLP *);

void destroy_glslp(GLSLP *);

#endif /* glslp_h */
