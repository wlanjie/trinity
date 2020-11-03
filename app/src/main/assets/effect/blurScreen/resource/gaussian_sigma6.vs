attribute vec3 position;
attribute vec2 inputTextureCoordinate;

uniform float texelWidthOffset;
uniform float texelHeightOffset;
uniform float scale;
varying vec2 blurCoordinates[15];
void main()
{
    gl_Position = vec4(position, 1.);
    
    vec2 singleStepOffset = vec2(texelWidthOffset, texelHeightOffset);
    blurCoordinates[0] = inputTextureCoordinate;
    blurCoordinates[1] = inputTextureCoordinate.xy + singleStepOffset * 1.489585 * scale;
    blurCoordinates[2] = inputTextureCoordinate.xy - singleStepOffset * 1.489585 * scale;
    blurCoordinates[3] = inputTextureCoordinate.xy + singleStepOffset * 3.475713 * scale;
    blurCoordinates[4] = inputTextureCoordinate.xy - singleStepOffset * 3.475713 * scale;
    blurCoordinates[5] = inputTextureCoordinate.xy + singleStepOffset * 5.461879 * scale;
    blurCoordinates[6] = inputTextureCoordinate.xy - singleStepOffset * 5.461879 * scale;
    blurCoordinates[7] = inputTextureCoordinate.xy + singleStepOffset * 7.448104 * scale;
    blurCoordinates[8] = inputTextureCoordinate.xy - singleStepOffset * 7.448104 * scale;
    blurCoordinates[9] = inputTextureCoordinate.xy + singleStepOffset * 9.434408 * scale;
    blurCoordinates[10] = inputTextureCoordinate.xy - singleStepOffset * 9.434408 * scale;
    blurCoordinates[11] = inputTextureCoordinate.xy + singleStepOffset * 11.420812 * scale;
    blurCoordinates[12] = inputTextureCoordinate.xy - singleStepOffset * 11.420812 * scale;
    blurCoordinates[13] = inputTextureCoordinate.xy + singleStepOffset * 13.407332 * scale;
    blurCoordinates[14] = inputTextureCoordinate.xy - singleStepOffset * 13.407332 * scale;
}
