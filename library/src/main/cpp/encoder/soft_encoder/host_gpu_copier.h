#ifndef HOST_GPU_COPIER_H
#define HOST_GPU_COPIER_H
#include "CommonTools.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "egl_core/gl_tools.h"

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

static char * vertexShaderStr =
    "attribute highp vec2 posAttr;\n"
    "attribute highp vec2 texCoordAttr;\n"
    "varying highp vec2 texCoord;\n"
    "void main()\n"
    "{\n"
    "    texCoord = texCoordAttr;\n"
    "    gl_Position = vec4(posAttr, 0, 1);\n"
    "}\n";

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

static char * convertToYUY2FragmentShaderStr =
    "varying highp vec2 texCoord;\n"
    "uniform sampler2D sampler;\n"
    "uniform mediump vec4 coefY;\n"
    "uniform mediump vec4 coefU;\n"
    "uniform mediump vec4 coefV;\n"
    "uniform highp float sPerHalfTexel;\n"
    "void main()\n"
    "{\n"
    "    highp vec2 texelOffset = vec2(sPerHalfTexel, 0);\n"
    "    lowp vec4 leftRGBA = texture2D(sampler, texCoord - texelOffset);\n"
    "    lowp vec4 rightRGBA = texture2D(sampler, texCoord + texelOffset);\n"
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

class HostGPUCopier {
public:
	HostGPUCopier();
    virtual ~HostGPUCopier();

    void copyYUY2Image(GLuint ipTex, byte* yuy2ImageBuffer, int width, int height);
    void destroy();
private:
    bool isRecording;
    GLuint vertexShader;
    GLuint pixelShader;

    GLuint FBO;
    GLuint downloadTex;

    char* mVertexShader;
    char* mFragmentShader;
    GLuint mGLProgId;

    GLuint attrLoc_pos;
    GLuint attrLoc_texCoord;
    GLint mGLUniformTexture;
    GLint uniformLoc_coefY;
    GLint uniformLoc_coefU;
    GLint uniformLoc_coefV;
    GLint uniformLoc_sPerHalfTexel;

    bool prepareConvertToYUY2Program();
    void convertYUY2(GLuint ipTex, int width, int height);
    void downloadImageFromTexture(GLuint texId, void *imageBuf, unsigned int imageWidth, unsigned int imageHeight);

    int getBufferSizeInBytes(int width, int height);

	void ensureTextureStorage(GLint internalFormat, const unsigned int yuy2Pairs, int height);
};

#endif // HOST_GPU_COPIER_H
