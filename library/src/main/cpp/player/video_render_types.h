//
//  video_render_types.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef video_render_types_h
#define video_render_types_h

#include <jni.h>
#include <stdlib.h>
//#include <GLES3/gl3.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <libavutil/frame.h>
#include <EGL/egl.h>
#include <stdbool.h>
#include "macro.h"

typedef enum {
    NONE = -1,
    Rect = 0,
    Ball = 1,
//    VR = 2,
//    Planet = 3,
//    Architecture = 4,
//    Expand = 5
} ModelType;

typedef struct GLSLProgram {
    uint8_t has_init;

    void (*init)();

    GLuint program;
    GLint positon_location;
    GLint texcoord_location;
    GLint linesize_adjustment_location;
    GLint x_scale_location;
    GLint y_scale_location;
    GLint frame_rotation_location;
    GLint modelMatrixLoc;
    GLint viewMatrixLoc;
    GLint projectionMatrixLoc;
    GLint tex_y, tex_u, tex_v;
    GLint texture_matrix_location;
} GLSLProgram;

typedef struct GLSLProgramDistortion{
    uint8_t has_init;
    GLuint program;
    GLint positon_location;
    GLint texcoord_r_location;
    GLint texcoord_g_location;
    GLint texcoord_b_location;
    GLint vignette_location;
    GLint texcoord_scale_location;
    GLint tex;
} GLSLProgramDistortion;

typedef struct struct_xl_eye{
    GLuint point_buffer_id;
    GLuint index_buffer_id;
    size_t index_count;
}xl_eye;

typedef struct Model {
    ModelType type;
    enum AVPixelFormat pixel_format;
    GLuint vbos[3];
    size_t elementsCount;
    GLuint texture[4];
    GLSLProgram * program;

    GLfloat modelMatrix[16];
    GLfloat view_matrix[16];
    GLfloat projectionMatrix[16];
    GLfloat head_matrix[16];
    GLfloat texture_matrix[16];
    float width_adjustment;
    int viewport_w, viewport_h;

    // only RECT Model use
    float x_scale, y_scale;
    int frame_rotation;

    //only VR Model use
    GLfloat view_matrix_r[16];
    xl_eye left_eye, right_eye;
    GLSLProgramDistortion * program_distortion;
    GLuint frame_texture_id;
    GLuint color_render_id;
    GLuint frame_buffer_id;

    void (*draw)(struct Model *model);

    void (*resize)(struct Model *model, int w, int h);

    void (*updateTexture)(struct Model *model, AVFrame *frame);

    void (*bind_texture)(struct Model *model);

    void (*update_frame)(struct Model *model, AVFrame *frame);

    // rx ry rz ∈ [-2π, 2π]
    void (*updateModelRotation)(struct Model *model, GLfloat rx, GLfloat ry, GLfloat rz,bool enable_tracker);

    // distance ∈ [-1.0, 1.0]
    void (*update_distance)(struct Model *model, GLfloat distance);

    void (*updateHead)(struct Model *model, GLfloat *headMatrix);
} Model;

#define NO_CMD 0
#define CMD_SET_WINDOW 1
#define CMD_CHANGE_MODEL 1 << 1
#define CMD_CHANGE_SCALE 1 << 2
#define CMD_CHANGE_ROTATION 1 << 3

typedef enum DrawMode {
    wait_frame, fixed_frequency
} DrawMode;

typedef struct VideoRenderContext {
    JNIEnv *env;
    int width;
    int height;
    EGLConfig config;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    struct ANativeWindow *window;
    struct ANativeWindow *texture_window;
    GLuint texture[4];
    Model *model;
    bool enable_tracker;
    pthread_mutex_t *lock;
    DrawMode draw_mode;

    uint8_t cmd;
    ModelType require_model_type;
    jfloat require_model_scale;
    float require_model_rotation[3];

    void (*set_window)(struct VideoRenderContext *ctx, struct ANativeWindow *window);

    void (*change_model)(struct VideoRenderContext *ctx, ModelType model_type);

} VideoRenderContext;
#endif  // video_render_types_h
