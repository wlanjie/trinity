#ifdef GL_ES
precision highp float;
varying highp vec2 textureCoordinate;
#else
varying vec2 textureCoordinate;
#endif

uniform sampler2D inputImageTexture;

void main() {
    int col = int(textureCoordinate.y * 2.0);
    vec2 textureCoordinateToUse = textureCoordinate;
    textureCoordinateToUse.y = (textureCoordinate.y - float(col) / 2.0) * 2.0;
    textureCoordinateToUse.y = textureCoordinateToUse.y/960.0*480.0+1.0/4.0;
    gl_FragColor=texture2D(inputImageTexture, textureCoordinateToUse);
}
