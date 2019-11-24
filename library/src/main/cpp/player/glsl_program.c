//
//  glsl_program.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//
#include "glsl_program.h"
#define STR(s) #s

static void init_rect_nv12();
static void init_rect_yuv_420p();
static void init_rect_oes();
static void init_ball_nv12();
static void init_ball_yuv_420p();
static void init_ball_oes();
static void init_expand_nv12();
static void init_expand_yuv_420p();
static void init_expand_oes();

static const char * vs_rect = STR(
    attribute vec3 position;
    attribute vec2 texcoord;
    uniform float width_adjustment;
    uniform float x_scale;
    uniform float y_scale;
    uniform int frame_rotation;
    varying vec2 tx;

    const mat2 rotation90 = mat2(
            0.0, -1.0,
            1.0, 0.0
    );
    const mat2 rotation180 = mat2(
            -1.0, 0.0,
            0.0, -1.0
    );
    const mat2 rotation270 = mat2(
            0.0, 1.0,
            -1.0, 0.0
    );
    void main(){
        tx = vec2(texcoord.x * width_adjustment, texcoord.y);
        vec2 xy = vec2(position.x * x_scale, position.y * y_scale);
        if(frame_rotation == 1){
            xy = rotation90 * xy;
        }else if(frame_rotation == 2){
            xy = rotation180 * xy;
        }else if(frame_rotation == 3){
            xy = rotation270 * xy;
        }
        gl_Position = vec4(xy, position.z, 1.0);
    }
);

static const char * vs_ball = STR(
    attribute vec3 position;
    attribute vec2 texcoord;
    uniform float width_adjustment;
    uniform mat4 modelMatrix;
    uniform mat4 viewMatrix;
    uniform mat4 projectionMatrix;
    varying vec2 tx;

    void main(){
        tx = vec2(texcoord.x * width_adjustment, texcoord.y);
        gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
    }
);

static const char * vs_expand = STR(
    attribute vec4 position;
    attribute vec2 texcoord;
    uniform float width_adjustment;
    uniform mat4 modelMatrix;
    varying vec2 tx;
    void main(void){
        tx = vec2(texcoord.x * width_adjustment, texcoord.y);

        vec4 ballPos = modelMatrix * position;
        float j = asin(ballPos.y);
        float i = degrees(acos(ballPos.x / cos(j)));
        i -= 180.0;
        j = degrees(j);
        if(ballPos.z < 0.0){ i = -i; }
        float xx = i / 180.0 * 1.1;
        float yy = j / 90.0 * 1.1;
        gl_Position = vec4(xx, yy, 0.1, 1.0);
    }
);

static const char * fs_egl_ext = "#extension GL_OES_EGL_image_external : require\n"
"precision mediump float;\n"
"uniform mat4 tx_matrix;\n"
"uniform samplerExternalOES tex_y;\n"
"varying vec2 tx;\n"
"void main(){\n"
"    vec2 tx_transformed = (tx_matrix * vec4(tx, 0, 1.0)).xy;\n"
"    gl_FragColor = texture2D(tex_y, tx_transformed);\n"
"}\n";

static const char * fs_nv12 = STR(
    precision mediump float;
    varying vec2 tx;
    uniform sampler2D tex_y;
    uniform sampler2D tex_u;
    void main(void)
    {
        vec2 tx_flip_y = vec2(tx.x, 1.0 - tx.y);
        float y = texture2D(tex_y, tx_flip_y).r;
        vec4 uv = texture2D(tex_u, tx_flip_y);
        float u = uv.r - 0.5;
        float v = uv.a - 0.5;
        float r = y +             1.402 * v;
        float g = y - 0.344 * u - 0.714 * v;
        float b = y + 1.772 * u;
        gl_FragColor = vec4(r, g, b, 1.0);
    }
);

static const char * fs_yuv_420p = STR(
    precision mediump float;
    varying vec2 tx;
    uniform sampler2D tex_y;
    uniform sampler2D tex_u;
    uniform sampler2D tex_v;
    void main(void)
    {
        vec2 tx_flip_y = vec2(tx.x, 1.0 - tx.y);
        float y = texture2D(tex_y, tx_flip_y).r;
        float u = texture2D(tex_u, tx_flip_y).r - 0.5;
        float v = texture2D(tex_v, tx_flip_y).r - 0.5;
        float r = y +             1.402 * v;
        float g = y - 0.344 * u - 0.714 * v;
        float b = y + 1.772 * u;
        gl_FragColor = vec4(r, g, b, 1.0);
    }
);

static const char * vs_distortion = STR(
    attribute vec2 aPosition;
    attribute float aVignette;
    attribute vec2 aRedTextureCoord;
    attribute vec2 aGreenTextureCoord;
    attribute vec2 aBlueTextureCoord;
    varying vec2 vRedTextureCoord;
    varying vec2 vBlueTextureCoord;
    varying vec2 vGreenTextureCoord;
    varying float vVignette;
    uniform float uTextureCoordScale;
    void main() {
        gl_Position = vec4(aPosition, 0.0, 1.0);
        vRedTextureCoord = aRedTextureCoord.xy * uTextureCoordScale;
        vGreenTextureCoord = aGreenTextureCoord.xy * uTextureCoordScale;
        vBlueTextureCoord = aBlueTextureCoord.xy * uTextureCoordScale;
        vVignette = aVignette;
    }
);

static const char * fs_distortion = STR(
    precision mediump float;
    varying vec2 vRedTextureCoord;
    varying vec2 vBlueTextureCoord;
    varying vec2 vGreenTextureCoord;
    varying float vVignette;
    uniform sampler2D uTextureSampler;
    void main() {
        gl_FragColor = vVignette * vec4(texture2D(uTextureSampler, vRedTextureCoord).r,
                                        texture2D(uTextureSampler, vGreenTextureCoord).g,
                                        texture2D(uTextureSampler, vBlueTextureCoord).b, 1.0);
    }
);

static GLSLProgram rect_nv12 = {
        .has_init = 0,
        .init = init_rect_nv12
};
static GLSLProgram rect_yuv_420p = {
        .has_init = 0,
        .init = init_rect_yuv_420p
};
static GLSLProgram rect_oes = {
        .has_init = 0,
        .init = init_rect_oes
};
static GLSLProgram ball_nv12 = {
        .has_init = 0,
        .init = init_ball_nv12
};
static GLSLProgram ball_yuv_420p = {
        .has_init = 0,
        .init = init_ball_yuv_420p
};
static GLSLProgram ball_oes = {
        .has_init = 0,
        .init = init_ball_oes
};
static GLSLProgram expand_nv12 = {
        .has_init = 0,
        .init = init_expand_nv12
};
static GLSLProgram expand_yuv_420p = {
        .has_init = 0,
        .init = init_expand_yuv_420p
};
static GLSLProgram expand_oes = {
        .has_init = 0,
        .init = init_expand_oes
};
static GLSLProgramDistortion distortion = {
        .has_init = 0
};

static GLuint loadShader(GLenum shaderType, const char * shaderSrc){
    GLuint shader;
    GLint compiled;
    shader = glCreateShader(shaderType);
    if(shader == 0) return 0;
    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled){
        GLint infoLen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen > 0){
            char * infoLog = (char *)malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            LOGE("compile shader error ==>\n%s\n\n%s\n", shaderSrc, infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}


static GLuint loadProgram(const char * vsSrc, const char * fsSrc){
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fsSrc);
    if(vertexShader == 0 || fragmentShader == 0) return 0;
    GLint linked;
    GLuint pro = glCreateProgram();
    if(pro == 0){
        LOGE("create program error!");
    }
    glAttachShader(pro, vertexShader);
    glAttachShader(pro, fragmentShader);
    glLinkProgram(pro);
    glGetProgramiv(pro, GL_LINK_STATUS, &linked);
    if(!linked){
        GLint infoLen;
        glGetProgramiv(pro, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen > 0){
            char * infoLog = (char *)malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(pro, infoLen, NULL, infoLog);
            LOGE("link program error ==>\n%s\n", infoLog);
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(pro);
        return 0;
    }
    return pro;
}

static void init_rect_nv12(){
    GLuint pro = loadProgram(vs_rect, fs_nv12);
    rect_nv12.program = pro;
    rect_nv12.positon_location = glGetAttribLocation(pro, "position");
    rect_nv12.texcoord_location = glGetAttribLocation(pro, "texcoord");
    rect_nv12.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    rect_nv12.x_scale_location = glGetUniformLocation(pro, "x_scale");
    rect_nv12.y_scale_location = glGetUniformLocation(pro, "y_scale");
    rect_nv12.frame_rotation_location = glGetUniformLocation(pro, "frame_rotation");
    rect_nv12.tex_y = glGetUniformLocation(pro, "tex_y");
    rect_nv12.tex_u = glGetUniformLocation(pro, "tex_u");
}

static void init_rect_yuv_420p(){
    GLuint pro = loadProgram(vs_rect, fs_yuv_420p);
    rect_yuv_420p.program = pro;
    rect_yuv_420p.positon_location = glGetAttribLocation(pro, "position");
    rect_yuv_420p.texcoord_location = glGetAttribLocation(pro, "texcoord");
    rect_yuv_420p.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    rect_yuv_420p.x_scale_location = glGetUniformLocation(pro, "x_scale");
    rect_yuv_420p.y_scale_location = glGetUniformLocation(pro, "y_scale");
    rect_yuv_420p.frame_rotation_location = glGetUniformLocation(pro, "frame_rotation");
    rect_yuv_420p.tex_y = glGetUniformLocation(pro, "tex_y");
    rect_yuv_420p.tex_u = glGetUniformLocation(pro, "tex_u");
    rect_yuv_420p.tex_v = glGetUniformLocation(pro, "tex_v");
}

static void init_rect_oes(){
    GLuint pro = loadProgram(vs_rect, fs_egl_ext);
    rect_oes.program = pro;
    rect_oes.positon_location = glGetAttribLocation(pro, "position");
    rect_oes.texcoord_location = glGetAttribLocation(pro, "texcoord");
    rect_oes.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    rect_oes.x_scale_location = glGetUniformLocation(pro, "x_scale");
    rect_oes.y_scale_location = glGetUniformLocation(pro, "y_scale");
    rect_oes.frame_rotation_location = glGetUniformLocation(pro, "frame_rotation");
    rect_oes.tex_y = glGetUniformLocation(pro, "tex_y");
    rect_oes.texture_matrix_location = glGetUniformLocation(pro, "tx_matrix");
}

static void init_ball_nv12(){
    GLuint pro = loadProgram(vs_ball, fs_nv12);
    ball_nv12.program = pro;
    ball_nv12.positon_location = glGetAttribLocation(pro, "position");
    ball_nv12.texcoord_location = glGetAttribLocation(pro, "texcoord");
    ball_nv12.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    ball_nv12.modelMatrixLoc = glGetUniformLocation(pro, "modelMatrix");
    ball_nv12.viewMatrixLoc = glGetUniformLocation(pro, "viewMatrix");
    ball_nv12.projectionMatrixLoc = glGetUniformLocation(pro, "projectionMatrix");
    ball_nv12.tex_y = glGetUniformLocation(pro, "tex_y");
    ball_nv12.tex_u = glGetUniformLocation(pro, "tex_u");
}

static void init_ball_yuv_420p() {
    GLuint pro = loadProgram(vs_ball, fs_yuv_420p);
    ball_yuv_420p.program = pro;
    ball_yuv_420p.positon_location = glGetAttribLocation(pro, "position");
    ball_yuv_420p.texcoord_location = glGetAttribLocation(pro, "texcoord");
    ball_yuv_420p.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    ball_yuv_420p.modelMatrixLoc = glGetUniformLocation(pro, "modelMatrix");
    ball_yuv_420p.viewMatrixLoc = glGetUniformLocation(pro, "viewMatrix");
    ball_yuv_420p.projectionMatrixLoc = glGetUniformLocation(pro, "projectionMatrix");
    ball_yuv_420p.tex_y = glGetUniformLocation(pro, "tex_y");
    ball_yuv_420p.tex_u = glGetUniformLocation(pro, "tex_u");
    ball_yuv_420p.tex_v = glGetUniformLocation(pro, "tex_v");
}

static void init_ball_oes(){
    GLuint pro = loadProgram(vs_ball, fs_egl_ext);
    ball_oes.program = pro;
    ball_oes.positon_location = glGetAttribLocation(pro, "position");
    ball_oes.texcoord_location = glGetAttribLocation(pro, "texcoord");
    ball_oes.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    ball_oes.modelMatrixLoc = glGetUniformLocation(pro, "modelMatrix");
    ball_oes.viewMatrixLoc = glGetUniformLocation(pro, "viewMatrix");
    ball_oes.projectionMatrixLoc = glGetUniformLocation(pro, "projectionMatrix");
    ball_oes.tex_y = glGetUniformLocation(pro, "tex_y");
    ball_oes.texture_matrix_location = glGetUniformLocation(pro, "tx_matrix");
}

static void init_expand_nv12(){
    GLuint pro = loadProgram(vs_expand, fs_nv12);
    expand_nv12.program = pro;
    expand_nv12.positon_location = glGetAttribLocation(pro, "position");
    expand_nv12.texcoord_location = glGetAttribLocation(pro, "texcoord");
    expand_nv12.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    expand_nv12.modelMatrixLoc = glGetUniformLocation(pro, "modelMatrix");
    expand_nv12.tex_y = glGetUniformLocation(pro, "tex_y");
    expand_nv12.tex_u = glGetUniformLocation(pro, "tex_u");
}

static void init_expand_yuv_420p(){
    GLuint pro = loadProgram(vs_expand, fs_yuv_420p);
    expand_yuv_420p.program = pro;
    expand_yuv_420p.positon_location = glGetAttribLocation(pro, "position");
    expand_yuv_420p.texcoord_location = glGetAttribLocation(pro, "texcoord");
    expand_yuv_420p.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    expand_yuv_420p.modelMatrixLoc = glGetUniformLocation(pro, "modelMatrix");
    expand_yuv_420p.tex_y = glGetUniformLocation(pro, "tex_y");
    expand_yuv_420p.tex_u = glGetUniformLocation(pro, "tex_u");
    expand_yuv_420p.tex_v = glGetUniformLocation(pro, "tex_v");
}

static void init_expand_oes(){
    GLuint pro = loadProgram(vs_expand, fs_egl_ext);
    expand_oes.program = pro;
    expand_oes.positon_location = glGetAttribLocation(pro, "position");
    expand_oes.texcoord_location = glGetAttribLocation(pro, "texcoord");
    expand_oes.linesize_adjustment_location = glGetUniformLocation(pro, "width_adjustment");
    expand_oes.modelMatrixLoc = glGetUniformLocation(pro, "modelMatrix");
    expand_oes.tex_y = glGetUniformLocation(pro, "tex_y");
    expand_oes.texture_matrix_location = glGetUniformLocation(pro, "tx_matrix");
}

GLSLProgram * glsl_program_get(ModelType type, int pixel_format){
    GLSLProgram * pro = NULL;
    switch(type){
        case Rect:
            switch(pixel_format){
                case AV_PIX_FMT_NV12:
                    pro = &rect_nv12;
                    break;
                case AV_PIX_FMT_YUV420P:
                    pro = &rect_yuv_420p;
                    break;
                case PIX_FMT_EGL_EXT:
                    pro = &rect_oes;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    if(pro != NULL && pro->has_init == 0){
        pro->init();
        pro->has_init = 1;
    }
    return pro;
}

GLSLProgramDistortion * glsl_program_distortion_get(){
    if(distortion.has_init == 0){
        GLuint pro = loadProgram(vs_distortion, fs_distortion);
        distortion.program = pro;
        distortion.positon_location = glGetAttribLocation(pro, "aPosition");
        distortion.vignette_location = glGetAttribLocation(pro, "aVignette");
        distortion.texcoord_r_location = glGetAttribLocation(pro, "aRedTextureCoord");
        distortion.texcoord_g_location = glGetAttribLocation(pro, "aGreenTextureCoord");
        distortion.texcoord_b_location = glGetAttribLocation(pro, "aBlueTextureCoord");
        distortion.texcoord_scale_location = glGetUniformLocation(pro, "uTextureCoordScale");
        distortion.tex = glGetUniformLocation(pro, "uTextureSampler");
    }
    return &distortion;
}

void glsl_program_clear_all(){
    glDeleteProgram(rect_nv12.program);
    rect_nv12.has_init = 0;

    glDeleteProgram(rect_yuv_420p.program);
    rect_yuv_420p.has_init = 0;

    glDeleteProgram(rect_oes.program);
    rect_oes.has_init = 0;

    glDeleteProgram(ball_nv12.program);
    ball_nv12.has_init = 0;

    glDeleteProgram(ball_yuv_420p.program);
    ball_yuv_420p.has_init = 0;

    glDeleteProgram(ball_oes.program);
    ball_oes.has_init = 0;

    glDeleteProgram(expand_nv12.program);
    expand_nv12.has_init = 0;

    glDeleteProgram(expand_yuv_420p.program);
    expand_yuv_420p.has_init = 0;

    glDeleteProgram(expand_oes.program);
    expand_oes.has_init = 0;

    glDeleteProgram(distortion.program);
    distortion.has_init = 0;
}