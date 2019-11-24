//
//  texture.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include "texture.h"

void initTexture(AVPlayContext * pd) {
    VideoRenderContext * ctx = pd->video_render_context;
    if(pd->is_sw_decode){
        //软解数据可能会有Y/U/V三个通道的数据,或者Y/UV两个通道,所以初始化了三个纹理
        glGenTextures(3, ctx->texture);
        for(int i = 0; i < 3; i++){
            glBindTexture(GL_TEXTURE_2D, ctx->texture[i]);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
    }else{
        //硬解的时候只需要创建一个surfacetexture即可
        glGenTextures(1, &ctx->texture[3]);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, ctx->texture[3]);
        glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
}

void texture_delete(AVPlayContext * pd){
    VideoRenderContext * ctx = pd->video_render_context;
    if(pd->is_sw_decode) {
        glDeleteTextures(3, ctx->texture);
    }else{
        glDeleteTextures(1, &ctx->texture[3]);
    }
}

void update_texture_yuv420p(Model *model, AVFrame *frame){
    glBindTexture(GL_TEXTURE_2D, model->texture[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[0], frame->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[0]);
    glBindTexture(GL_TEXTURE_2D, model->texture[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[1], frame->height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[1]);
    glBindTexture(GL_TEXTURE_2D, model->texture[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[2], frame->height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[2]);
}

void bind_texture_yuv420p(Model *model){
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, model->texture[0]);
    glUniform1i(model->program->tex_y, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, model->texture[1]);
    glUniform1i(model->program->tex_u, 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, model->texture[2]);
    glUniform1i(model->program->tex_v, 2);
}

void update_texture_nv12(Model *model, AVFrame *frame){
    glBindTexture(GL_TEXTURE_2D, model->texture[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[0], frame->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[0]);
    glBindTexture(GL_TEXTURE_2D, model->texture[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, frame->linesize[1] / 2, frame->height / 2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, frame->data[1]);
}

void bind_texture_nv12(Model *model){
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, model->texture[0]);
    glUniform1i(model->program->tex_y, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, model->texture[1]);
    glUniform1i(model->program->tex_u, 1);
}

void bind_texture_oes(Model *model){
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, model->texture[3]);
    glUniform1i(model->program->tex_y, 0);
    glUniformMatrix4fv(model->program->texture_matrix_location, 1, GL_FALSE, model->texture_matrix);
}

void update_texture_oes(__attribute__((unused)) Model *model, __attribute__((unused)) AVFrame *frame){

}