#ifdef GL_ES
precision highp float;
#endif

varying vec2 textureCoordinate;
uniform sampler2D inputImageTexture;
uniform sampler2D inputImageTextureBlurred;

void main() {
    int col = int(textureCoordinate.y * 3.0);
	vec2 textureCoordinateToUse = textureCoordinate;
	textureCoordinateToUse.y = (textureCoordinate.y - float(col) / 3.0) * 3.0;
	textureCoordinateToUse.y = textureCoordinateToUse.y / 3.0 + 1.0 / 3.0;
    if (col == 1)
		gl_FragColor = texture2D(inputImageTexture, textureCoordinateToUse);
	else 
        gl_FragColor = texture2D(inputImageTextureBlurred, textureCoordinate);
}
