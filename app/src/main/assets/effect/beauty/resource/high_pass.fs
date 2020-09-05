#ifdef GL_ES
precision mediump float;
#endif
varying vec2 textureCoordinate;
uniform sampler2D inputImageTexture;
uniform sampler2D blurTexture;
// 强光程度
const float intensity = 24.0;

void main() {
#ifdef GL_ES
    lowp vec4 sourceColor = texture2D(inputImageTexture, textureCoordinate);
    lowp vec4 blurColor = texture2D(blurTexture, textureCoordinate);
    // 高通滤波之后的颜色值
    highp vec4 highPassColor = sourceColor - blurColor;
    highPassColor.r = clamp(2.0 * highPassColor.r * highPassColor.r * intensity, 0.0, 1.0);
    highPassColor.g = clamp(2.0 * highPassColor.g * highPassColor.g * intensity, 0.0, 1.0);
    highPassColor.b = clamp(2.0 * highPassColor.b * highPassColor.b * intensity, 0.0, 1.0);
    // 输出的是把痘印等过滤掉
    gl_FragColor = vec4(highPassColor.rgb, 1.0);
#else
    vec4 sourceColor = texture2D(inputImageTexture, textureCoordinate);
    vec4 blurColor = texture2D(blurTexture, textureCoordinate);
    // 高通滤波之后的颜色值
    vec4 highPassColor = sourceColor - blurColor;
    highPassColor.r = clamp(2.0 * highPassColor.r * highPassColor.r * intensity, 0.0, 1.0);
    highPassColor.g = clamp(2.0 * highPassColor.g * highPassColor.g * intensity, 0.0, 1.0);
    highPassColor.b = clamp(2.0 * highPassColor.b * highPassColor.b * intensity, 0.0, 1.0);
    // 输出的是把痘印等过滤掉
    gl_FragColor = vec4(highPassColor.rgb, 1.0);
#endif
}