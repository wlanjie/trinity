#ifndef GPU_TEXTURE_FRAME_COPIER_H
#define GPU_TEXTURE_FRAME_COPIER_H

#include <GLES2/gl2.h>
//#include <GLES2/gl2ext.h>
#include "../common_tools.h"
#include "texture_frame_copier.h"
#include "../opengl_media/texture_frame.h"

namespace trinity {

static const char *GPU_FRAME_VERTEX_SHADER =
        "attribute vec4 vPosition;\n"
        "attribute vec4 vTexCords;\n"
        "varying vec2 yuvTexCoords;\n"
        "uniform highp mat4 trans; \n"
        "void main() {\n"
        "  yuvTexCoords = vTexCords.xy;\n"
        "  gl_Position = trans * vPosition;\n"
        "}\n";

static const char *GPU_FRAME_FRAGMENT_SHADER =
        "#extension GL_OES_EGL_image_external : require\n"
        "precision mediump float;\n"
        "uniform samplerExternalOES yuvTexSampler;\n"
        "varying vec2 yuvTexCoords;\n"
        "void main() {\n"
        "  gl_FragColor = texture2D(yuvTexSampler, yuvTexCoords);\n"
        "}\n";

class GPUTextureFrameCopier : public TextureFrameCopier {
public:
    GPUTextureFrameCopier();

    virtual ~GPUTextureFrameCopier();

    virtual bool Init();

    virtual void RenderWithCoords(TextureFrame *textureFrame, GLuint texId, GLfloat *vertexCoords, GLfloat *textureCoords);

protected:
    GLint uniform_texture_;
};

}
#endif // GPU_TEXTURE_FRAME_COPIER_H
