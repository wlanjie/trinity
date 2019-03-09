#ifndef RECORDING_PREVIEW_RENDERER_H
#define RECORDING_PREVIEW_RENDERER_H

#include "opengl_media/render/video_gl_surface_render.h"
#include "opengl_media/texture/gpu_texture_frame.h"
#include "opengl_media/texture_copier/gpu_texture_frame_copier.h"
#include "common_tools.h"
#include "egl_core/gl_tools.h"

#define PREVIEW_FILTER_SEQUENCE_IN                                    0
#define PREVIEW_FILTER_SEQUENCE_OUT                                    10 * 60 * 60 * 1000000
#define PREVIEW_FILTER_POSITION                                        0.5

static GLfloat CAMERA_TRIANGLE_VERTICES[8] = {
        -1.0f, -1.0f,    // 0 top left
        1.0f, -1.0f,    // 1 bottom left
        -1.0f, 1.0f,  // 2 bottom right
        1.0f, 1.0f,    // 3 top right
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

class RecordingPreviewRenderer {
public:
    RecordingPreviewRenderer();

    virtual ~RecordingPreviewRenderer();

    void Init(int degress, bool isVFlip, int textureWidth, int textureHeight, int cameraWidth,
              int cameraHeight);

    void SetDegress(int degress, bool isVFlip);

    void ProcessFrame(float position);

    void DrawToView(int videoWidth, int videoHeight);

    void DrawToViewWithAutoFit(int videoWidth, int videoHeight, int texWidth, int texHeight);

    void Destroy();

    int GetCameraTexId();

    GLuint GetInputTexId() {
        return input_texture_id_;
    };

protected:
    GLuint fbo_;
    GPUTextureFrame *camera_texutre_frame_;
    //利用mCopier转换为Sampler2D格式的inputTexId
    GLuint input_texture_id_;
    //用于旋转的纹理id
    GLuint rotate_texture_id_;
    /** 1:把Camera的纹理拷贝到inputTexId **/
    GPUTextureFrameCopier *copier_;
    /** 3:把outputTexId渲染到View上 **/
    VideoGLSurfaceRender *renderer_;

    int degress_;
    bool is_vflip_;
    GLfloat *texture_coords_;
    int texture_coords_size_;
    int camera_width_;
    int camera_height_;

    void FillTextureCoords();

    float Flip(float i);

    GLfloat *GetVertexCoords();
};

#endif // RECORDING_PREVIEW_RENDERER_H
