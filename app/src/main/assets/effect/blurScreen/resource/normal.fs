#ifdef GL_ES
precision highp float; 
#endif

varying vec2 textureCoordinate; 
uniform sampler2D inputImageTexture; 

void main() {
	gl_FragColor = texture2D(inputImageTexture, textureCoordinate);
}
