attribute vec3 position;
attribute vec2 inputTextureCoordinate;
// 高斯算子左右偏移值，当偏移值为5时，高斯算子为 11 x 11
const int SHIFT_SIZE = 5;
#ifdef GL_ES
uniform highp float texelWidthOffset;
uniform highp float texelHeightOffset;
#else
uniform float texelWidthOffset;
uniform float texelHeightOffset;
#endif
varying vec2 textureCoordinate;
varying vec4 blurShiftCoordinates[SHIFT_SIZE];

void main() {
    gl_Position = vec4(position, 1.);
    textureCoordinate = inputTextureCoordinate.xy;
    vec2 singleStepOffset = vec2(texelWidthOffset, texelHeightOffset);
    for (int i = 0; i < SHIFT_SIZE; i++) {
        blurShiftCoordinates[i] = vec4(textureCoordinate.xy - float(i + 1) * singleStepOffset, 
                                       textureCoordinate.xy + float(i + 1) * singleStepOffset);
    }
}