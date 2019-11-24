//
//  glsl_program.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef glsl_program_h
#define glsl_program_h

#include "video_render_types.h"

GLSLProgram* glsl_program_get(ModelType type, int pixel_format);
GLSLProgramDistortion* glsl_program_distortion_get();
void glsl_program_clear_all();

#endif  // glsl_program_h
