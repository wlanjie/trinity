#ifdef GL_ES
varying highp vec2 textureCoordinate;
uniform sampler2D inputImageTexture;
uniform sampler2D inputImageTexture2;
uniform lowp float mixturePercent;
uniform highp float scalePercent;
void main() {
    lowp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);    
    highp vec2 textureCoordinateToUse = textureCoordinate;
    highp vec2 center = vec2(0.5, 0.5);
    textureCoordinateToUse -= center;
    textureCoordinateToUse = textureCoordinateToUse / scalePercent;
    textureCoordinateToUse += center;
    lowp vec4 textureColor2 = texture2D(inputImageTexture, textureCoordinateToUse);    
    gl_FragColor = mix(textureColor, textureColor2, mixturePercent);
}
#else
varying vec2 textureCoordinate;
uniform sampler2D inputImageTexture;
uniform sampler2D inputImageTexture2;
uniform float mixturePercent;
uniform float scalePercent;
void main() {
    vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);    
    vec2 textureCoordinateToUse = textureCoordinate;
    vec2 center = vec2(0.5, 0.5);
    textureCoordinateToUse -= center;
    textureCoordinateToUse = textureCoordinateToUse / scalePercent;
    textureCoordinateToUse += center;
    vec4 textureColor2 = texture2D(inputImageTexture, textureCoordinateToUse);    
    gl_FragColor = mix(textureColor, textureColor2, mixturePercent);
}
#endif
