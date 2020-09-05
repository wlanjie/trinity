#ifdef GL_ES
precision mediump float;
#endif
varying vec2 textureCoordinate;
uniform sampler2D inputImageTexture;
// The left and right offset value of the Gaussian operator, when the offset value is 2, the Gaussian operator is 5 x 5
const int SHIFT_SIZE = 2;
varying vec4 blurShiftCoordinates[SHIFT_SIZE];
void main() {
    // Calculate the color value of the current coordinate
    vec4 currentColor = texture2D(inputImageTexture, textureCoordinate);
#ifdef GL_ES
    mediump vec3 sum = currentColor.rgb;
    // Calculate the sum of the color values of the offset coordinates
    for (int i = 0; i < SHIFT_SIZE; i++) {
        sum += texture2D(inputImageTexture, blurShiftCoordinates[i].xy).rgb;
        sum += texture2D(inputImageTexture, blurShiftCoordinates[i].zw).rgb;
    }
    // Find the average
    gl_FragColor = vec4(sum * 1.0 / float(2 * SHIFT_SIZE + 1), currentColor.a);
#else
    vec3 sum = currentColor.rgb;
    // Calculate the sum of the color values of the offset coordinates
    for (int i = 0; i < SHIFT_SIZE; i++) {
        sum += texture2D(inputImageTexture, blurShiftCoordinates[i].xy).rgb;
        sum += texture2D(inputImageTexture, blurShiftCoordinates[i].zw).rgb;
    }
    // Find the average
    gl_FragColor = vec4(sum * 1.0 / float(2 * SHIFT_SIZE + 1), currentColor.a);
#endif
}