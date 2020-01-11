#ifdef GL_ES
precision mediump float;
#endif
varying vec2 textureCoordinate;
uniform sampler2D inputImageTexture;         // 输入原图
uniform sampler2D blurTexture;          // 原图的高斯模糊纹理
uniform sampler2D highPassBlurTexture;  // 高反差保留的高斯模糊纹理
#ifdef GL_ES
uniform lowp float intensity;           // 磨皮程度
#else
uniform float intensity;           // 磨皮程度
#endif
void main() {
#ifdef GL_ES
    lowp vec4 sourceColor = texture2D(inputImageTexture, textureCoordinate);
    lowp vec4 blurColor = texture2D(blurTexture, textureCoordinate);
    lowp vec4 highPassBlurColor = texture2D(highPassBlurTexture, textureCoordinate);
    // 调节蓝色通道值
    mediump float value = clamp((min(sourceColor.b, blurColor.b) - 0.2) * 5.0, 0.0, 1.0);
    // 找到模糊之后RGB通道的最大值
    mediump float maxChannelColor = max(max(highPassBlurColor.r, highPassBlurColor.g), highPassBlurColor.b);
    // 计算当前的强度
    mediump float currentIntensity = (1.0 - maxChannelColor / (maxChannelColor + 0.2)) * value * intensity;
    // 混合输出结果
    lowp vec3 resultColor = mix(sourceColor.rgb, blurColor.rgb, currentIntensity);
    // 输出颜色
    gl_FragColor = vec4(resultColor, 1.0);
#else
    vec4 sourceColor = texture2D(inputImageTexture, textureCoordinate);
    vec4 blurColor = texture2D(blurTexture, textureCoordinate);
    vec4 highPassBlurColor = texture2D(highPassBlurTexture, textureCoordinate);
    // 调节蓝色通道值
    float value = clamp((min(sourceColor.b, blurColor.b) - 0.2) * 5.0, 0.0, 1.0);
    // 找到模糊之后RGB通道的最大值
    float maxChannelColor = max(max(highPassBlurColor.r, highPassBlurColor.g), highPassBlurColor.b);
    // 计算当前的强度
    float currentIntensity = (1.0 - maxChannelColor / (maxChannelColor + 0.2)) * value * intensity;
    // 混合输出结果
    vec3 resultColor = mix(sourceColor.rgb, blurColor.rgb, currentIntensity);
    // 输出颜色
    gl_FragColor = vec4(resultColor, 1.0);
#endif
}
