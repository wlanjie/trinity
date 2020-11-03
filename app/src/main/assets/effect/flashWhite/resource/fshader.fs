#ifdef GL_ES
precision highp float;
#endif
varying vec2 textureCoordinate;
uniform sampler2D inputImageTexture;
uniform float alphaTimeLine;
void main() {
    gl_FragColor = (1.0 - alphaTimeLine) * texture2D(inputImageTexture, textureCoordinate) + alphaTimeLine * vec4(1.0,1.0,1.0,0.0);
}

