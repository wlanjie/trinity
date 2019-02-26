//
// Created by wlanjie on 2019/2/22.
//

#ifndef TRINITY_PREVIEW_RENDERER_H
#define TRINITY_PREVIEW_RENDERER_H

#include <GLES2/gl2.h>
#include "../common/opengl_media/texture_frame.h"
#include "../common/texture_copier/gpu_texture_frame_copier.h"
#include "../common/opengl_media/render/video_gl_surface_render.h"
#include "../common/opengl_media/gpu_texture_frame.h"

namespace trinity {

#define PREVIEW_FILTER_SEQUENCE_IN									0
#define PREVIEW_FILTER_SEQUENCE_OUT									10 * 60 * 60 * 1000000
#define PREVIEW_FILTER_POSITION										0.5

static GLfloat CAMERA_TRIANGLE_VERTICES[8] = {
        -1.0f, -1.0f,	// 0 top left
        1.0f, -1.0f,	// 1 bottom left
        -1.0f, 1.0f,  // 2 bottom right
        1.0f, 1.0f,	// 3 top right
};

static GLfloat CAMERA_TEXTURE_NO_ROTATION[8] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
};

static GLfloat CAMERA_TEXTURE_ROTATED_90[8] = {
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f
};

static GLfloat CAMERA_TEXTURE_ROTATED_180[8] = {
        1.0f, 0.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
};

static GLfloat CAMERA_TEXTURE_ROTATED_270[8] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f
};


class PreviewRenderer {

public:
    PreviewRenderer();
    ~PreviewRenderer();

    void Init(int degress, bool is_vflip, int texture_width, int texture_height, int camera_width, int camera_height);
    void SetDegress(int degress, bool is_vflip);
    void ProcessFrame(float position);
    void DrawToView(int video_width, int video_height);
    void DrawToViewWithAutoFit(int video_width, int video_height, int texture_width, int texture_height);
    void Dealloc();

    int GetCameraTextureId();

    GLuint GetInputTextureId() {
        return input_texture_id_;
    }
protected:
    GLuint fbo_;
    GLuint input_texture_id_;
    GPUTextureFrame* camera_texture_frame_;
    GLuint rotate_texture_id_;
    GPUTextureFrameCopier* copier_;
    VideoGLSurfaceRender* render_;

    int degress_;
    bool is_vflip_;
    GLfloat* texture_coords_;
    int texture_coords_size_;
    int texture_width_;
    int texture_height_;
    int camera_width_;
    int camera_height_;

    void FillTextureCoords();
    float Flip(float i);
    GLfloat* GetVertexCoords();
};

}
#endif //TRINITY_PREVIEW_RENDERER_H
