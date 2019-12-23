#ifdef GL_ES
precision highp float;
attribute vec3 position;
attribute vec2 inputTextureCoordinate;
varying vec2 textureCoordinate;
void main() {
    gl_Position = vec4(position,1.0);
    textureCoordinate = inputTextureCoordinate.xy;
}
#else
attribute vec3 position;
attribute vec2 inputTextureCoordinate;
varying vec2 textureCoordinate;
void main() {
    gl_Position = vec4(position,1.0);
    textureCoordinate = inputTextureCoordinate.xy;
}
#endif
