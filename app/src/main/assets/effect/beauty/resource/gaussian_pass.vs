attribute vec4 position;
attribute vec4 inputTextureCoordinate;

// The left and right offset value of the Gaussian operator, when the offset value is 2, the Gaussian operator is 5 x 5
const int SHIFT_SIZE = 2;

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
    gl_Position = position;
    textureCoordinate = inputTextureCoordinate.xy;
    // Offset step
    vec2 singleStepOffset = vec2(texelWidthOffset, texelHeightOffset);
    // Record offset coordinates
    for (int i = 0; i < SHIFT_SIZE; i++) {
        blurShiftCoordinates[i] = vec4(textureCoordinate.xy - float(i + 1) * singleStepOffset,
                                       textureCoordinate.xy + float(i + 1) * singleStepOffset);
    }
}