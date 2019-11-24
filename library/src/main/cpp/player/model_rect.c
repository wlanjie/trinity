//
//  model_rect.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include "model_rect.h"
#include "mesh_factory.h"
#include "texture.h"
#include "glsl_program.h"

static int video_width = 0, video_height = 0;

static inline void resize_video(Model *model){
    int screen_width = model->viewport_w, screen_height = model->viewport_h;
    if(screen_width == 0
            || screen_height == 0
            || video_width == 0
            || video_height == 0
            ){
        model->x_scale = 1.0f;
        model->y_scale = 1.0f;
    }
    float screen_rate = (float)screen_width / (float)screen_height;
    float video_rate = (float)video_width / (float)video_height;
    if(model->frame_rotation == ROTATION_90 || model->frame_rotation == ROTATION_270){
        screen_rate = (float)screen_height / (float)screen_width;
    }
    if(screen_rate > video_rate){
        model->x_scale = video_rate / screen_rate;
        model->y_scale = 1.0f;
    }else{
        model->x_scale = 1.0f;
        model->y_scale = screen_rate / video_rate;
    }
}

void model_rect_resize(Model *model, int w, int h) {
    model->viewport_w = w;
    model->viewport_h = h;
    resize_video(model);
}

static void draw_rect(Model *model){
    GLSLProgram * program = model->program;
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, model->viewport_w, model->viewport_h);
    model->bind_texture(model);
    glUniform1f(program->linesize_adjustment_location, model->width_adjustment);
    glUniform1f(program->x_scale_location, model->x_scale);
    glUniform1f(program->y_scale_location, model->y_scale);
    glUniform1i(program->frame_rotation_location, model->frame_rotation);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->vbos[2]);
    glDrawElements(GL_TRIANGLES, (GLsizei) model->elementsCount, GL_UNSIGNED_INT, 0);
}

void update_frame_rect(Model *model, AVFrame * frame){
    if(model->pixel_format != frame->format){
        model->pixel_format = (enum AVPixelFormat)frame->format;
        model->program = glsl_program_get(model->type, model->pixel_format);
        glUseProgram(model->program->program);
        switch(frame->format){
            case AV_PIX_FMT_YUV420P:
                model->bind_texture = bind_texture_yuv420p;
                model->updateTexture = update_texture_yuv420p;
                break;
            case AV_PIX_FMT_NV12:
                model->bind_texture = bind_texture_nv12;
                model->updateTexture = update_texture_nv12;
                break;
            case PIX_FMT_EGL_EXT:
                model->bind_texture = bind_texture_oes;
                model->updateTexture = update_texture_oes;
                break;
            default:
                LOGE("not support this pix_format ==> %d", frame->format);
                return;
        }
        glBindBuffer(GL_ARRAY_BUFFER, model->vbos[0]);
        glVertexAttribPointer((GLuint) model->program->positon_location, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray((GLuint) model->program->positon_location);
        glBindBuffer(GL_ARRAY_BUFFER, model->vbos[1]);
        glVertexAttribPointer((GLuint) model->program->texcoord_location, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray((GLuint) model->program->texcoord_location);
    }
    model->updateTexture(model, frame);
    // some video linesize > width
    model->width_adjustment = (float)frame->width / (float)frame->linesize[0];
    if(frame->width != video_width
       || frame->height != video_height
       || frame->FRAME_ROTATION != model->frame_rotation
            ){
        video_width = frame->width;
        video_height = frame->height;
        model->frame_rotation = frame->FRAME_ROTATION;
        resize_video(model);
    }
}

Model * model_rect_create(){
    Model *model = (Model *) malloc(sizeof(Model));
    model->type = Rect;
    model->program = NULL;
    model->draw = NULL;
    model->updateTexture = NULL;
    model->resize = model_rect_resize;
    model->update_frame = update_frame_rect;
    model->draw = draw_rect;
    model->pixel_format = AV_PIX_FMT_NONE;
    model->width_adjustment = 1;
    model->x_scale = 1.0f;
    model->y_scale = 1.0f;
    model->frame_rotation = ROTATION_0;

    model->updateModelRotation = NULL;
    model->update_distance = NULL;
    model->updateHead = NULL;

    Mesh *rect = get_rect_mesh();
    glGenBuffers(3, model->vbos);
    glBindBuffer(GL_ARRAY_BUFFER, model->vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * rect->ppLen, rect->pp, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, model->vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * rect->ttLen, rect->tt, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->vbos[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * rect->indexLen, rect->index,
                 GL_STATIC_DRAW);
    model->elementsCount = (size_t) rect->indexLen;
    free_mesh(rect);
    glDisable(GL_CULL_FACE);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    return model;
}