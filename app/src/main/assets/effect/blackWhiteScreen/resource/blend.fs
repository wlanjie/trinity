#ifdef GL_ES
precision highp float;
#endif

varying vec2 textureCoordinate;
uniform sampler2D inputImageTexture;
uniform sampler2D inputImageTextureBlurred;
uniform sampler2D inputImageTextureLookupFront;
uniform float heightRatio;

#ifdef GL_ES
void main() {
	vec4 clearColor = clamp(texture2D(inputImageTexture, textureCoordinate), 0.0, 1.0);
	vec4 blurColor = clamp(texture2D(inputImageTextureBlurred, textureCoordinate.xy), 0.0, 1.0);
	//front filter
	highp float blueColor = clearColor.b * 63.0;
	highp vec2 quad1;
    quad1.y = floor(floor(blueColor) / 8.0);
    quad1.x = floor(blueColor) - (quad1.y * 8.0);
	highp vec2 texPos1;
    texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * clearColor.r);
    texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * clearColor.g);
	lowp vec4 newColorFront = texture2D(inputImageTextureLookupFront, texPos1);

	if(textureCoordinate.y > 0.5 - heightRatio / 2.0 && textureCoordinate.y < 0.5 + heightRatio / 2.0){
		gl_FragColor = newColorFront;
	}
	else{
		gl_FragColor = blurColor;
	}
}
#else
void main() {
	vec4 clearColor = clamp(texture2D(inputImageTexture, textureCoordinate), 0.0, 1.0);
	vec4 blurColor = clamp(texture2D(inputImageTextureBlurred, textureCoordinate.xy), 0.0, 1.0);
	//front filter
	float blueColor = clearColor.b * 63.0;
	vec2 quad1;
    quad1.y = floor(floor(blueColor) / 8.0);
    quad1.x = floor(blueColor) - (quad1.y * 8.0);
	vec2 texPos1;
    texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * clearColor.r);
    texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * clearColor.g);
	vec4 newColorFront = texture2D(inputImageTextureLookupFront, texPos1);

	if(textureCoordinate.y > 0.5 - heightRatio / 2.0 && textureCoordinate.y < 0.5 + heightRatio / 2.0){
		gl_FragColor = newColorFront;
	}
	else{
		gl_FragColor = blurColor;
	}
}
#endif