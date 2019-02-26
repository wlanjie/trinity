#ifndef 	VIDEO_PLAYER_GL_SURFACE_RENDER_H
#define VIDEO_PLAYER_GL_SURFACE_RENDER_H

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace trinity {

static const char *OUTPUT_VIEW_VERTEX_SHADER =
		"attribute vec4 position;    \n"
		"attribute vec2 texcoord;   \n"
		"varying vec2 v_texcoord;     \n"
		"void main(void)               \n"
		"{                            \n"
		"   gl_Position = position;  \n"
		"   v_texcoord = texcoord;  \n"
		"}                            \n";

static const char *OUTPUT_VIEW_FRAG_SHADER =
		"varying highp vec2 v_texcoord;\n"
		"uniform sampler2D yuvTexSampler;\n"
		"void main() {\n"
		"  gl_FragColor = texture2D(yuvTexSampler, v_texcoord);\n"
		"}\n";

/**
 * Video OpenGL View
 */
class VideoGLSurfaceRender {
public:
	VideoGLSurfaceRender();

	virtual ~VideoGLSurfaceRender();

	bool Init(int width, int height);

	void ResetRenderSize(int left, int top, int width, int height);

	void RenderToView(GLuint texID, int screenWidth, int screenHeight);

	void RenderToView(GLuint texID);

	void RenderToViewWithAutofit(GLuint texID, int screenWidth, int screenHeight, int texWidth, int texHeight);

	void RenderToViewWithAutoFill(GLuint texID, int screenWidth, int screenHeight, int texWidth, int texHeight);

	void RenderToVFlipTexture(GLuint inputTexId, GLuint outputTexId);

	void RenderToTexture(GLuint inputTexId, GLuint outputTexId);

	void RenderToAutoFitTexture(GLuint inputTexId, int width, int height, GLuint outputTexId);

	void RenderToCroppedTexture(GLuint inputTexId, GLuint outputTexId, int originalWidth, int originalHeight);

	void RenderToEncoderTexture(GLuint inputTexId, GLuint outputTexId);

	void Dealloc();

protected:
	GLint backing_left_;
	GLint backing_top_;
	GLint backing_width_;
	GLint backing_height_;

	char *vertex_shader_;
	char *fragment_shader_;

	bool initialized_;

	GLuint prog_id_;
	GLuint vertex_coords_;
	GLuint texture_coords_;
	GLint uniform_texture_;

	void CheckGlError(const char *op);

	GLuint LoadProgram(char *pVertexSource, char *pFragmentSource);

	GLuint LoadShader(GLenum shaderType, const char *pSource);

	float CalcCropRatio(int screenWidth, int screenHeight, int texWidth, int texHeight);
};

}
#endif //VIDEO_PLAYER_GL_SURFACE_RENDER_H
