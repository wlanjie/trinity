#ifdef GL_ES
precision highp float;
#endif

attribute vec3 position;
attribute vec2 inputTextureCoordinate;

varying vec2 textureCoordinate;
varying vec2 textureCoordinate2;

void main() {
    gl_Position = vec4(position, 1.);
    textureCoordinate = inputTextureCoordinate.xy;
    textureCoordinate2 = inputTextureCoordinate.xy * 0.5 + 0.5;
}
