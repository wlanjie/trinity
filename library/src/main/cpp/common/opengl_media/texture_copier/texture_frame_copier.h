#ifndef TEXTURE_FRAME_COPIER_H
#define TEXTURE_FRAME_COPIER_H

#include "common_tools.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "opengl_media/texture/texture_frame.h"
#include "egl_core/gl_tools.h"

static const char* NO_FILTER_VERTEX_SHADER =
		"attribute vec4 vPosition;\n"
		"attribute vec4 vTexCords;\n"
		"varying vec2 yuvTexCoords;\n"
		"uniform highp mat4 texMatrix;\n"
		"uniform highp mat4 trans; \n"
		"void main() {\n"
		"  yuvTexCoords = (texMatrix*vTexCords).xy;\n"
		"  gl_Position = trans * vPosition;\n"
		"}\n";

static const char* NO_FILTER_FRAGMENT_SHADER =
	    "varying vec2 yuvTexCoords;\n"
	    "uniform sampler2D yuvTexSampler;\n"
		"void main() {\n"
		"  gl_FragColor = texture2D(yuvTexSampler, yuvTexCoords);\n"
		"}\n";

class TextureFrameCopier {
public:
	TextureFrameCopier();
    virtual ~TextureFrameCopier();

    virtual bool Init() = 0;
    virtual void RenderWithCoords(TextureFrame *textureFrame, GLuint texId, GLfloat *vertexCoords,
								  GLfloat *textureCoords) = 0;
    virtual void Destroy();

protected:
    char* vertex_shader_;
    char* fragment_shader_;

	bool initialized_;
	GLuint prog_id_;
	GLuint vertex_coords_;
	GLuint texture_coords_;
	GLint uniform_texture_matrix_;
	GLint uniform_transforms_;
};

#endif // TEXTURE_FRAME_COPIER_H
