//
// Created by wlanjie on 2020-02-07.
//

#include "face_markup_render.h"
#include "gl.h"

#ifdef __ANDROID__
#include "android_xlog.h"
#else
#define LOGE(format, ...) fprintf(stdout, format, __VA_ARGS__)
#endif

const char* FACE_MARKUP_VERTEX =
        "attribute vec4 position;"
        "attribute vec2 inputTextureCoordinate;"
        "varying vec2 textureCoordinate;"
        "varying vec2 textureCoordinate2;"
        "void main() {"
        "   gl_Position = position;"
        "   textureCoordinate = inputTextureCoordinate;"
        "   textureCoordinate2 = position.xy * 0.5 + 0.5;"
        "}";

const char* FACE_MARKUP_FRAGMENT =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "varying highp vec2 textureCoordinate;\n"
        "varying highp vec2 textureCoordinate2;\n"
        "#else\n"
        "varying vec2 textureCoordinate;\n"
        "varying vec2 textureCoordinate2;\n"
        "#endif\n"
        "uniform sampler2D inputImageTexture;\n"
        "uniform sampler2D inputImageTexture2;\n"
        "uniform float intensity;"
        "uniform int blendMode;"
        "float blendHardLight(float base, float blend) {"
        "    return blend < 0.5 ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0 - blend));"
        "}"
        "vec3 blendHardLight(vec3 base, vec3 blend) {"
        "    return vec3(blendHardLight(base.r,blend.r),blendHardLight(base.g,blend.g),blendHardLight(base.b,blend.b));"
        "}"

        "float blendSoftLight(float base, float blend) {"
        "    return (blend<0.5)?(base+(2.0*blend-1.0)*(base-base*base)):(base+(2.0*blend-1.0)*(sqrt(base)-base));"
        "}"
        "vec3 blendSoftLight(vec3 base, vec3 blend) {"
        "    return vec3(blendSoftLight(base.r,blend.r),blendSoftLight(base.g,blend.g),blendSoftLight(base.b,blend.b));"
        "}"
        "vec3 blendMultiply(vec3 base, vec3 blend) {"
        "    return base*blend;"
        "}"
        "float blendOverlay(float base, float blend) {"
        "    return base<0.5?(2.0*base*blend):(1.0-2.0*(1.0-base)*(1.0-blend));"
        "}"
        "vec3 blendOverlay(vec3 base, vec3 blend) {"
        "    return vec3(blendOverlay(base.r,blend.r),blendOverlay(base.g,blend.g),blendOverlay(base.b,blend.b));"
        "}"
        "vec3 blendFunc(vec3 base, vec3 blend, int blendMode) {"
        "    if (blendMode == 0) {"
        "        return blend;"
        "    } else if (blendMode == 15) {"
        "       return blendMultiply(base, blend);"
        "    } else if (blendMode == 17) {"
        "        return blendOverlay(base, blend);"
        "    } else if (blendMode == 22) {"
        "        return blendHardLight(base, blend);"
        "    }"
        "    return blend;"
        "}"
        "void main() {"
        "    vec4 fgColor = texture2D(inputImageTexture2, textureCoordinate);"
        "    fgColor = fgColor * intensity;"
        "    vec4 bgColor = texture2D(inputImageTexture, textureCoordinate2);"
        "    if (fgColor.a == 0.0) {"
        "        gl_FragColor = bgColor;"
        "        return;"
        "    }"
        "    vec3 color = blendFunc(bgColor.rgb, clamp(fgColor.rgb * (1.0 / fgColor.a), 0.0, 1.0), blendMode);"
        "    gl_FragColor = vec4(bgColor.rgb * (1.0 - fgColor.a) + color.rgb * fgColor.a, 1.0);"
//        "   gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);"
        "}";
        
static const char* FACE_POINT_VERTEX_SHADER =
        "attribute vec4 position;"
        "void main() {"
        "   gl_Position = position;"
        "   gl_PointSize = 45.0;"
        "}";

static const char* FACE_POINT_FRAGMENT_SHADER =
        "void main() {"
        "   gl_FragColor = vec4(0.0,1.0,0.0,1.0);"
        "}";        
        
//#define SHOW_LAND_MAKE 0        

namespace trinity {

FaceMarkupRender::FaceMarkupRender() :
    program_(0)
    , prop_program_(0)
    , default_texture_coordinates_(nullptr)
    , default_vertex_coordinates_(nullptr)
    , texture_id_(0)
    , frame_buffer_id_(0)
    , source_width_(0)
    , source_height_(0)
    , frame_buffer_(nullptr) {
    default_vertex_coordinates_ = new GLfloat[8];
    default_texture_coordinates_ = new GLfloat[8];

    prop_program_ = CreateProgram(FACE_MARKUP_VERTEX, FACE_MARKUP_FRAGMENT);
    program_ = CreateProgram(DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
    InitCoordinates();
}

FaceMarkupRender::~FaceMarkupRender() {
    if (prop_program_ != 0) {
        glDeleteProgram(prop_program_);
        prop_program_ = 0;
    }
    if (nullptr != default_vertex_coordinates_) {
        delete[] default_vertex_coordinates_;
        default_vertex_coordinates_ = nullptr;
    }
    if (nullptr != default_texture_coordinates_) {
        delete[] default_texture_coordinates_;
        default_texture_coordinates_ = nullptr;
    }
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    DeleteFrameBuffer();
}

void FaceMarkupRender::CreateFrameBuffer(int width, int height) {
    DeleteFrameBuffer();
    glGenTextures(1, &texture_id_);
    glGenFramebuffers(1, &frame_buffer_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id_, 0);

    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
#if __ANDROID__
        LOGE("frame buffer error");
#endif
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    frame_buffer_ = new FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
}

void FaceMarkupRender::DeleteFrameBuffer() {
    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }
    if (frame_buffer_id_ != 0) {
        glDeleteFramebuffers(1, &frame_buffer_id_);
        frame_buffer_id_ = 0;
    }
    if (nullptr != frame_buffer_) {
        delete frame_buffer_;
        frame_buffer_ = nullptr;
    }
}

int FaceMarkupRender::OnDrawFrame(int origin_texture_id,
        int prop_texture_id,
        int source_width,
        int source_height,
        int blend_mode,
        float intensity,
        float* texture_coordinate,
        FaceDetectionReport *face_detection) {
        
    if (source_width_ != source_width && source_height_ != source_height) {
        source_width_ = source_width;
        source_height_ = source_height;
        CreateFrameBuffer(source_width, source_height);
    }
    int texture_id = frame_buffer_->OnDrawFrame(origin_texture_id);        
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id_);
    GLfloat* vertices = face_detection->VertexCoordinates();
    GLuint* indexs = face_detection->ElementIndexs();
    int element_count = face_detection->ElementCount();

    glViewport(0, 0, WIDTH, HEIGHT);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    auto input_image_texture = static_cast<GLuint>(glGetUniformLocation(program_, "inputImageTexture"));
    glUniform1i(input_image_texture, 0);
    auto position = static_cast<GLuint>(glGetAttribLocation(program_, "position"));
    glEnableVertexAttribArray(position);
    glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), default_vertex_coordinates_);
    auto input_texture_coordinate = static_cast<GLuint>(glGetAttribLocation(program_, "inputTextureCoordinate"));
    glEnableVertexAttribArray(input_texture_coordinate);
    glVertexAttribPointer(input_texture_coordinate, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), default_texture_coordinates_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(position);
    glDisableVertexAttribArray(input_texture_coordinate);
    glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(prop_program_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    GLint input_image_texture_location = glGetUniformLocation(prop_program_, "inputImageTexture");
    glUniform1i(input_image_texture_location, 1);

    GLint blend_model_location = glGetUniformLocation(prop_program_, "blendMode");
    glUniform1i(blend_model_location, blend_mode);
    GLint intensity_location = glGetUniformLocation(prop_program_, "intensity");
    glUniform1f(intensity_location, intensity);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, prop_texture_id);
    GLint input_image_texture_location2 = glGetUniformLocation(prop_program_, "inputImageTexture2");
    glUniform1i(input_image_texture_location2, 2);

    auto prop_position = static_cast<GLuint>(glGetAttribLocation(prop_program_, "position"));
    glEnableVertexAttribArray(prop_position);
    glVertexAttribPointer(prop_position, 2, GL_FLOAT, 0, 0, vertices);

    auto prop_input_texture_coordinate = static_cast<GLuint>(glGetAttribLocation(prop_program_,
                                                                              "inputTextureCoordinate"));
    glEnableVertexAttribArray(prop_input_texture_coordinate);
    glVertexAttribPointer(prop_input_texture_coordinate, 2, GL_FLOAT, 0, 0, texture_coordinate);
    glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, indexs);
    glDisableVertexAttribArray(prop_position);
    glDisableVertexAttribArray(prop_input_texture_coordinate);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);

    #ifdef SHOW_LAND_MARK
    glUseProgram(point_program_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 111 * 2 * sizeof(float), vertices);
    GLuint point_position_location_ = static_cast<GLuint>(glGetAttribLocation(point_program_, "position"));
    glEnableVertexAttribArray(point_position_location_);
    glVertexAttribPointer(point_position_location_, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glPointSize(5);
    glDrawArrays(GL_POINTS, 0, 111);
    #endif
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return texture_id_;
}

GLuint FaceMarkupRender::CreateProgram(const char *vertex, const char *fragment) {
    auto program = glCreateProgram();
    auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
    auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    CompileShader(vertex, vertexShader);
    CompileShader(fragment, fragmentShader);
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    int ret = Link(program);
    if (ret != 0) {
        program = 0;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

void FaceMarkupRender::CompileShader(const char *shader_string, GLuint shader) {
    glShaderSource(shader, 1, &shader_string, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    printf("%s\n", shader_string);
    printf("==================\n");
    if (!compiled) {
        GLint infoLen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen) {
            auto *buf = reinterpret_cast<char*>(malloc((size_t) infoLen));
            if (buf) {
                LOGE("shader_string: %s", shader_string);
                glGetShaderInfoLog(shader, infoLen, nullptr, buf);
                LOGE("Could not compile %d:\n%s\n", shader, buf);
                free(buf);
            }
            glDeleteShader(shader);
        }
    }
}

int FaceMarkupRender::Link(GLuint program) {
    glLinkProgram(program);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        GLint infoLen;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen) {
            auto *buf = reinterpret_cast<char*>(malloc((size_t) infoLen));
            if (buf) {
                glGetProgramInfoLog(program, infoLen, nullptr, buf);
                printf("%s", buf);
                free(buf);
            }
            glDeleteProgram(program);
        }
        return -1;
    }
    return 0;
}

void FaceMarkupRender::InitCoordinates() {
    default_vertex_coordinates_[0] = -1.0F;
    default_vertex_coordinates_[1] = -1.0F;
    default_vertex_coordinates_[2] = 1.0F;
    default_vertex_coordinates_[3] = -1.0F;
    default_vertex_coordinates_[4] = -1.0F;
    default_vertex_coordinates_[5] = 1.0F;
    default_vertex_coordinates_[6] = 1.0F;
    default_vertex_coordinates_[7] = 1.0F;

    default_texture_coordinates_[0] = 0.0F;
    default_texture_coordinates_[1] = 0.0F;
    default_texture_coordinates_[2] = 1.0F;
    default_texture_coordinates_[3] = 0.0F;
    default_texture_coordinates_[4] = 0.0F;
    default_texture_coordinates_[5] = 1.0F;
    default_texture_coordinates_[6] = 1.0F;
    default_texture_coordinates_[7] = 1.0F;
}

}
