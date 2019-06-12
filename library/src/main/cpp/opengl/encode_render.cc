//
// Created by wlanjie on 2019/4/26.
//

#include "encode_render.h"
#include "android_xlog.h"
#include "gl.h"

#define NV_YUY2_R_Y_709     0.18259
#define	NV_YUY2_G_Y_709     0.61423
#define	NV_YUY2_B_Y_709     0.06201

#define NV_YUY2_R_U_709     0.10065
#define	NV_YUY2_G_U_709     0.33857
#define	NV_YUY2_B_U_709     0.43922

#define NV_YUY2_R_V_709     0.43922
#define	NV_YUY2_G_V_709     0.39894
#define	NV_YUY2_B_V_709     0.04027

#define IS_LUMA_601_FLAG    true

#define NV_YUY2_R_Y_601     0.25679
#define	NV_YUY2_G_Y_601     0.50413
#define	NV_YUY2_B_Y_601     0.09791

#define NV_YUY2_R_U_601     0.14822
#define	NV_YUY2_G_U_601     0.29099
#define	NV_YUY2_B_U_601     0.43922

#define NV_YUY2_R_V_601     0.43922
#define	NV_YUY2_G_V_601     0.36779
#define	NV_YUY2_B_V_601     0.07142

#define IS_LUMA_UNKNOW_FLAG  true

#define NV_YUY2_R_Y_UNKNOW     0.299
#define	NV_YUY2_G_Y_UNKNOW     0.587
#define	NV_YUY2_B_Y_UNKNOW     0.114

#define NV_YUY2_R_U_UNKNOW     0.147
#define	NV_YUY2_G_U_UNKNOW     0.289
#define	NV_YUY2_B_U_UNKNOW     0.436

#define NV_YUY2_R_V_UNKNOW     0.615
#define	NV_YUY2_G_V_UNKNOW     0.515
#define	NV_YUY2_B_V_UNKNOW     0.100

/**
 * From RGB to YUV
 * 		Y = 0.299R + 0.587G + 0.114B
 * 		U = 0.492 (B-Y)
 * 		V = 0.877 (R-Y)
 *
 * It can also be represented as:
 * 		Y =  0.299R + 0.587G + 0.114B
 * 		U = -0.147R - 0.289G + 0.436B
 * 		V =  0.615R - 0.515G - 0.100B
 *
 * From YUV to RGB
 * 		R = Y + 1.140V
 * 		G = Y - 0.395U - 0.581V
 * 		B = Y + 2.032U
 *
 */
/**
 * 下面这个FragmentShader 主要作用
 * 以下代表着纹理的坐标
 *  	(0, 1) --------------- (1, 1)
 * 	      |               |
 * 	      |               |
 * 	      |               |
 * 	      |               |
 * 	      |               |
 *  (0, 0) --------------- (1, 0)
 *  首先取出一个纹理坐标中的点 比如说(0.5, 0.0)这样子的一个二维向量，然后调用texture2D这个内嵌函数
 *  会把sampler上的这个像素点以一个Vec4的结构拿出来，实际上就是(r, g, b, a)的形式, 一般处理起来就是
 *  直接给到gl_FragColor，说明这个像素点处理完了
 *  但是我们这里是这样子的，因为我们是把RGBA的数据转换为YUV422的数据，所以像素数据少了一般，而在Main函数
 *  的处理过程中，我们就需要每次执行要把两个像素点合并成一个像素点
 *  也就是  pixel1:(r,g,b,a)  pixel2:(r,g,b,a)
 *  		   		  Y1,U1,V1             Y2
 *  然后在按照YUY2的pixelFormat组合起来(Y1,U1,Y2,V1) 这样子就把8个字节表示的两个像素搞成了4个字节的YUY2表示的2个像素点
 *
 *  具体是如何做的转换呢？
 *  		float perHalfTexel = 0.5 / width; 把纹理坐标的一半除以纹理的宽度
 *  		当前像素点的x坐标上减去perHalfTexel 所在的像素点 转换为Y1,U1,V1
 *  		当前像素点的x坐标上加上perHalfTexel 所在的像素点 转换为Y2
 *
 *	但是应该注意的是 我们将像素减半了，所以应该我们调用OpenGL指令glViewPort的时候应该width降为一般，最终调用指令glReadPixels
 *	的时候读取的width也应为一半
    "uniform highp float sPerHalfTexel = 0.00069445;\n"
 */

static const char * ENCODER_YUV_FRAGMENT =
        "varying highp vec2 textureCoordinate;\n"
        "uniform sampler2D inputImageTexture;\n"
        "uniform mediump vec4 coefY;\n"
        "uniform mediump vec4 coefU;\n"
        "uniform mediump vec4 coefV;\n"
        "uniform highp float sPerHalfTexel;\n"
        "void main()\n"
        "{\n"
        "    highp vec2 texelOffset = vec2(sPerHalfTexel, 0);\n"
        "    lowp vec4 leftRGBA = texture2D(inputImageTexture, textureCoordinate - texelOffset);\n"
        "    lowp vec4 rightRGBA = texture2D(inputImageTexture, textureCoordinate + texelOffset);\n"
        "    lowp vec4 left = vec4(leftRGBA.rgb, 1);\n"
        "    lowp float y0 = dot(left, coefY);\n"
        "    lowp float y1 = dot(vec4(rightRGBA.rgb, 1), coefY);\n"
        "    lowp float u0 = dot(left, coefU);\n"
        "    lowp float v0 = dot(left, coefV);\n"
        "    gl_FragColor = vec4(y0, u0, y1, v0);\n"
        "}\n";

typedef struct VertexCoordVec{
    //物体坐标x
    GLfloat x;
    //物体坐标y
    GLfloat y;
    //纹理坐标x
    GLfloat s;
    //纹理坐标y
    GLfloat t;
} VertexCoordVec;

namespace trinity {

EncodeRender::EncodeRender() : OpenGL(DEFAULT_VERTEX_SHADER, ENCODER_YUV_FRAGMENT) {
    recording_ = false;
    fbo_ = 0;
    download_texture_ = 0;

    vertex_coordinate_ = new GLfloat[8];
    texture_coordinate_ = new GLfloat[8];
//    glGenTextures(1, &download_texture_);
    glGenFramebuffers(1, &fbo_);
//    glBindTexture(GL_TEXTURE_2D, download_texture_);
//    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, download_texture_, 0);

//    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
//    if (status != GL_FRAMEBUFFER_COMPLETE) {
//        LOGE("frame buffer error");
//    }
//    glBindTexture(GL_TEXTURE_2D, 0);
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

EncodeRender::EncodeRender(int width, int height) : OpenGL(width, height, DEFAULT_VERTEX_SHADER, ENCODER_YUV_FRAGMENT) {
    recording_ = false;
    fbo_ = 0;
    download_texture_ = 0;
}

EncodeRender::~EncodeRender() {
    if (fbo_ != 0) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
    if (vertex_coordinate_ != nullptr) {
        delete[] vertex_coordinate_;
        vertex_coordinate_ = nullptr;
    }
    if (texture_coordinate_ != nullptr) {
        delete[] texture_coordinate_;
        texture_coordinate_ = nullptr;
    }
}

void EncodeRender::CopyYUV2Image(GLuint texture, uint8_t *buffer, int width, int height) {
    ConvertYUY2(texture, width, height);
    int pairs = (width + 1) >> 1;
    DownloadImageFromTexture(download_texture_, buffer, pairs, height);
}

void EncodeRender::Destroy() {
    if (download_texture_ != 0) {
        glDeleteTextures(1, &download_texture_);
        download_texture_ = 0;
    }
}

void EncodeRender::ConvertYUY2(int texture_id, int width, int height) {
    const int pairs = (width + 1) >> 1;
    GLint internal_format = GL_RGBA;
    EnsureTextureStorage(internal_format, pairs, height);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, download_texture_, 0);
    int status  =glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("GL_FRAMEBUFFER_COMPLETE error");
    }
    /**
      * 首先glReadPixels读出来的格式是GL_RGBA,每一个像素是用四个字节表示
      * 而我们期望的读出来的是YUY2
      * width:4 height:4
      * RGBA size : 4 * 4 * 4 = 64
      * YUY2 suze : 4 * 4 * 2 = 32
      * glReadPixels()
      */
    glViewport(0, 0, pairs, height);
    SetFloat("sPerHalfTexel", 0.5f / width);

    if (recording_) {
        vertex_coordinate_[0] = -1.0f;
        vertex_coordinate_[1] = -1.0f;
        vertex_coordinate_[2] = 1.0f;
        vertex_coordinate_[3] = -1.0f;

        vertex_coordinate_[4] = -1.0f;
        vertex_coordinate_[5] = 1.0f;
        vertex_coordinate_[6] = 1.0f;
        vertex_coordinate_[7] = 1.0f;

        texture_coordinate_[0] = 1.0f;
        texture_coordinate_[1] = 1.0f;
        texture_coordinate_[2] = 1.0f;
        texture_coordinate_[3] = 0.0f;

        texture_coordinate_[4] = 0.0f;
        texture_coordinate_[5] = 1.0f;
        texture_coordinate_[6] = 0.0f;
        texture_coordinate_[7] = 0.0f;
    } else {
        vertex_coordinate_[0] = -1.0f;
        vertex_coordinate_[1] = 1.0f;
        vertex_coordinate_[2] = -1.0f;
        vertex_coordinate_[3] = -1.0f;

        vertex_coordinate_[4] = 1.0f;
        vertex_coordinate_[5] = 1.0f;
        vertex_coordinate_[6] = 1.0f;
        vertex_coordinate_[7] = -1.0f;

        texture_coordinate_[0] = 0.0f;
        texture_coordinate_[1] = 0.0f;
        texture_coordinate_[2] = 0.0f;
        texture_coordinate_[3] = 1.0f;

        texture_coordinate_[4] = 1.0f;
        texture_coordinate_[5] = 0.0f;
        texture_coordinate_[6] = 1.0f;
        texture_coordinate_[7] = 1.0f;
    }
    ProcessImage(texture_id, vertex_coordinate_, texture_coordinate_);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void EncodeRender::RunOnDrawTasks() {
    if (IS_LUMA_UNKNOW_FLAG) {
        GLfloat coefYVec[4] = { (GLfloat) NV_YUY2_R_Y_UNKNOW, (GLfloat) NV_YUY2_G_Y_UNKNOW, (GLfloat) NV_YUY2_B_Y_UNKNOW, (GLfloat) 16 / 255 };
        GLfloat coefUVec[4] = { -(GLfloat) NV_YUY2_R_U_UNKNOW, -(GLfloat) NV_YUY2_G_U_UNKNOW, (GLfloat) NV_YUY2_B_U_UNKNOW, (GLfloat) 128 / 255 };
        GLfloat coefVVec[4] = { (GLfloat) NV_YUY2_R_V_UNKNOW, -(GLfloat) NV_YUY2_G_V_UNKNOW, -(GLfloat) NV_YUY2_B_V_UNKNOW, (GLfloat) 128 / 255 };

        SetFloatVec4("coefY", 1, coefYVec);
        SetFloatVec4("coefU", 1, coefUVec);
        SetFloatVec4("coefV", 1, coefVVec);
    } else if (IS_LUMA_601_FLAG) {
        GLfloat coefYVec[4] = { (GLfloat) NV_YUY2_R_Y_601, (GLfloat) NV_YUY2_G_Y_601, (GLfloat) NV_YUY2_B_Y_601, (GLfloat) 16 / 255 };
        GLfloat coefUVec[4] = { -(GLfloat) NV_YUY2_R_U_601, -(GLfloat) NV_YUY2_G_U_601, (GLfloat) NV_YUY2_B_U_601, (GLfloat) 128 / 255 };
        GLfloat coefVVec[4] = { (GLfloat) NV_YUY2_R_V_601, -(GLfloat) NV_YUY2_G_V_601, -(GLfloat) NV_YUY2_B_V_601, (GLfloat) 128 / 255 };

        SetFloatVec4("coefY", 1, coefYVec);
        SetFloatVec4("coefU", 1, coefUVec);
        SetFloatVec4("coefV", 1, coefVVec);
    } else {
        GLfloat coefYVec[4] = { (GLfloat) NV_YUY2_R_Y_709, (GLfloat) NV_YUY2_G_Y_709, (GLfloat) NV_YUY2_B_Y_709, (GLfloat) 16 / 255 };
        GLfloat coefUVec[4] = { -(GLfloat) NV_YUY2_R_U_709, -(GLfloat) NV_YUY2_G_U_709, (GLfloat) NV_YUY2_B_U_709, (GLfloat) 128 / 255 };
        GLfloat coefVVec[4] = { (GLfloat) NV_YUY2_R_V_709, -(GLfloat) NV_YUY2_G_V_709, -(GLfloat) NV_YUY2_B_V_709, (GLfloat) 128 / 255 };

        SetFloatVec4("coefY", 1, coefYVec);
        SetFloatVec4("coefU", 1, coefUVec);
        SetFloatVec4("coefV", 1, coefVVec);
    }
}

void EncodeRender::OnDrawArrays() {

}

void EncodeRender::DownloadImageFromTexture(int texture_id, void *buffer, int width, int height) {
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id, 0);
    // Download image
    glReadPixels(0, 0, (GLsizei) width, (GLsizei) height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int EncodeRender::GetBufferSizeInBytes(int width, int height) {
    return 0;
}

void EncodeRender::EnsureTextureStorage(GLint internal_format, int yuy2_pairs, int height) {
    if (download_texture_ == 0) {
        glGenTextures(1, &download_texture_);
        glBindTexture(GL_TEXTURE_2D, download_texture_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    } else {
        glBindTexture(GL_TEXTURE_2D, download_texture_);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, yuy2_pairs, height, 0, internal_format, GL_UNSIGNED_BYTE, 0);
}

}
