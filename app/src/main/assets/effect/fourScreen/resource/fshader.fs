#ifdef GL_ES
precision highp float;
uniform sampler2D inputImageTexture;
varying highp vec2 textureCoordinate;
void main() {
    int row = int(textureCoordinate.x * 2.0);
    int col = int(textureCoordinate.y * 2.0);
    vec2 textureCoordinateToUse = textureCoordinate;
    textureCoordinateToUse.x = (textureCoordinate.x - float(row) / 2.0) * 2.0;
    textureCoordinateToUse.y = (textureCoordinate.y - float(col) / 2.0) * 2.0;
    gl_FragColor=texture2D(inputImageTexture, textureCoordinateToUse);
}
#else
uniform sampler2D inputImageTexture;
varying vec2 textureCoordinate;
void main() {
    int row = int(textureCoordinate.x * 2.0);
    int col = int(textureCoordinate.y * 2.0);
    vec2 textureCoordinateToUse = textureCoordinate;
    textureCoordinateToUse.x = (textureCoordinate.x - float(row) / 2.0) * 2.0;
    textureCoordinateToUse.y = (textureCoordinate.y - float(col) / 2.0) * 2.0;
    gl_FragColor=texture2D(inputImageTexture, textureCoordinateToUse);
}
#endif