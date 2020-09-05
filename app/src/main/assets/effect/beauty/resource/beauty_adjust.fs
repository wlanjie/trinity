#ifdef GL_ES
precision mediump float;
#endif
varying vec2 textureCoordinate;
uniform sampler2D inputImageTexture;         // Input original image
uniform sampler2D blurTexture;          // Gaussian blur texture of the original image
uniform sampler2D highPassBlurTexture;  // Gaussian blur texture preserved by high contrast
#ifdef GL_ES
uniform lowp float intensity;           // Degree of microdermabrasion
#else
uniform float intensity;           // Degree of microdermabrasion
#endif
void main() {
#ifdef GL_ES
    lowp vec4 sourceColor = texture2D(inputImageTexture, textureCoordinate);
    lowp vec4 blurColor = texture2D(blurTexture, textureCoordinate);
    lowp vec4 highPassBlurColor = texture2D(highPassBlurTexture, textureCoordinate);
    // Adjust the blue channel value
    mediump float value = clamp((min(sourceColor.b, blurColor.b) - 0.2) * 5.0, 0.0, 1.0);
    // Find the maximum value of the RGB channel after blur
    mediump float maxChannelColor = max(max(highPassBlurColor.r, highPassBlurColor.g), highPassBlurColor.b);
    // Calculate current intensity
    mediump float currentIntensity = (1.0 - maxChannelColor / (maxChannelColor + 0.2)) * value * intensity;
    // Mixed output results
    lowp vec3 resultColor = mix(sourceColor.rgb, blurColor.rgb, currentIntensity);
    // Output color
    gl_FragColor = vec4(resultColor, 1.0);
#else
    vec4 sourceColor = texture2D(inputImageTexture, textureCoordinate);
    vec4 blurColor = texture2D(blurTexture, textureCoordinate);
    vec4 highPassBlurColor = texture2D(highPassBlurTexture, textureCoordinate);
    // Adjust the blue channel value
    float value = clamp((min(sourceColor.b, blurColor.b) - 0.2) * 5.0, 0.0, 1.0);
    // Find the maximum value of the RGB channel after blur
    float maxChannelColor = max(max(highPassBlurColor.r, highPassBlurColor.g), highPassBlurColor.b);
    // Calculate current intensity
    float currentIntensity = (1.0 - maxChannelColor / (maxChannelColor + 0.2)) * value * intensity;
    // Mixed output results
    vec3 resultColor = mix(sourceColor.rgb, blurColor.rgb, currentIntensity);
    // Output color
    gl_FragColor = vec4(resultColor, 1.0);
#endif
}
