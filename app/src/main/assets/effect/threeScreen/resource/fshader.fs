#ifdef GL_ES
precision highp float;
varying highp vec2 textureCoordinate;
#else
varying vec2 textureCoordinate;
#endif

uniform sampler2D inputImageTexture0;
void main() {
    int col = int(textureCoordinate.y * 3.0);
    vec2 textureCoordinateToUse = textureCoordinate;
    textureCoordinateToUse.y = (textureCoordinate.y - float(col) / 3.0) * 3.0;
    textureCoordinateToUse.y = textureCoordinateToUse.y/960.0*320.0+1.0/3.0;
    gl_FragColor=texture2D(inputImageTexture0, textureCoordinateToUse);
}