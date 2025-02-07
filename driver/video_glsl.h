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
// video_glsl.h: hacky SDL2 renderer that compatible of retroarch-style
// multipass shader preset header by palxex, 2018
//


#ifndef video_glsl_h
#define video_glsl_h

#ifdef __cplusplus
extern "C" {
#endif

void VIDEO_GLSL_Setup(int width, int height);
void VIDEO_GLSL_RenderCopy(void *data);

#ifdef __cplusplus
}
#endif

#endif /* video_glsl_h */
