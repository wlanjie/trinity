#include "yuv_texture_frame_copier.h"
#include "../egl/gl_tools.h"
#include "../matrix.h"

#define LOG_TAG "YUVTextureFrameCopier"

namespace trinity {

YUVTextureFrameCopier::YUVTextureFrameCopier() {
    vertex_shader_ = const_cast<char *>(NO_FILTER_VERTEX_SHADER);
    fragment_shader_ = const_cast<char *>(YUV_FRAME_FRAGMENT_SHADER);
}

YUVTextureFrameCopier::~YUVTextureFrameCopier() {
}

bool YUVTextureFrameCopier::Init() {
    prog_id_ = loadProgram(vertex_shader_, fragment_shader_);
    if (!prog_id_) {
        LOGE("Could not create program.");
        return false;
    }
    vertex_coords_ = glGetAttribLocation(prog_id_, "vPosition");
    checkGlError("glGetAttribLocation vPosition");
    texture_coords_ = glGetAttribLocation(prog_id_, "vTexCords");
    checkGlError("glGetAttribLocation vTexCords");
    glUseProgram(prog_id_);
    uniform_samplers_[0] = glGetUniformLocation(prog_id_, "s_texture_y");
    uniform_samplers_[1] = glGetUniformLocation(prog_id_, "s_texture_u");
    uniform_samplers_[2] = glGetUniformLocation(prog_id_, "s_texture_v");

    uniform_transforms_ = glGetUniformLocation(prog_id_, "trans");
    checkGlError("glGetUniformLocation mUniformTransforms");

    uniform_texure_matrix_ = glGetUniformLocation(prog_id_, "texMatrix");
    checkGlError("glGetUniformLocation mUniformTexMatrix");

    initialized_ = true;
    return true;
}

void YUVTextureFrameCopier::RenderWithCoords(TextureFrame *textureFrame, GLuint texId, GLfloat *vertexCoords,
                                             GLfloat *textureCoords) {
    glBindTexture(GL_TEXTURE_2D, texId);
    checkGlError("glBindTexture");

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texId, 0);
    checkGlError("glFramebufferTexture2D");

    glUseProgram(prog_id_);
    if (!initialized_) {
        return;
    }
    glVertexAttribPointer(vertex_coords_, 2, GL_FLOAT, GL_FALSE, 0, vertexCoords);
    glEnableVertexAttribArray(vertex_coords_);
    glVertexAttribPointer(texture_coords_, 2, GL_FLOAT, GL_FALSE, 0, textureCoords);
    glEnableVertexAttribArray(texture_coords_);
    /* Binding the input texture */
    textureFrame->BindTexture(uniform_samplers_);

    float texTransMatrix[4 * 4];
    matrixSetIdentityM(texTransMatrix);
    glUniformMatrix4fv(uniform_texure_matrix_, 1, GL_FALSE, (GLfloat *) texTransMatrix);

    float rotateMatrix[4 * 4];
    matrixSetIdentityM(rotateMatrix);
    glUniformMatrix4fv(uniform_transforms_, 1, GL_FALSE, (GLfloat *) rotateMatrix);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(vertex_coords_);
    glDisableVertexAttribArray(texture_coords_);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
//    LOGI("draw waste time is %ld", (getCurrentTime() - startDrawTimeMills));
}

}