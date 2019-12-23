#ifdef GL_ES
precision highp float;
varying highp vec2 textureCoordinate;
uniform highp float uScanLineJitter_x; // 长度
uniform highp float uScanLineJitter_y; // 密度
uniform highp float uColorDrift_x; // 颜色倾斜
uniform highp float uColorDrift_y; // 整体偏移
#else
varying vec2 textureCoordinate;
uniform float uScanLineJitter_x; // 长度
uniform float uScanLineJitter_y; // 密度
uniform float uColorDrift_x; // 颜色倾斜
uniform float uColorDrift_y; // 整体偏移
#endif
uniform sampler2D inputImageTexture;
uniform float uTimeStamp; 
vec2 uVerticalJump = vec2(0.0,0.0); // (amount, time)
uniform float uHorizontalShake;

float nrand(float x, float y) {
    return fract(sin(dot(vec2(x, y), vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    float u = textureCoordinate.x;
    float v = textureCoordinate.y;

    float jitter = nrand(v, uTimeStamp) * 2.0 - 1.0;
    jitter *= step(uScanLineJitter_y, abs(jitter)) * uScanLineJitter_x;
    float jump = mix(v, fract(v + uVerticalJump.y), uVerticalJump.x);
    float shake = (nrand(uTimeStamp, 2.0) - 0.5) * uHorizontalShake;
    
    // Color drift
    float drift = sin(jump + uColorDrift_y) * uColorDrift_x;
    
    vec4 src1 = texture2D(inputImageTexture, fract(vec2(u + jitter + shake, jump)));
    vec4 src2 = texture2D(inputImageTexture, fract(vec2(u + jitter + shake + drift, jump)));
    //src1.r, src1.g, src2.b 黄蓝
    //src1.r, src1.g, src1.b 无
    //src1.r, src2.g, src1.b 绿粉红
    //src1.r, src2.g, src2.b 蓝红
    //src2.r, src1.g, src1.b 蓝红
    //src2.r, src1.g, src2.b 粉红绿
    //src2.r, src2.g, src1.b 黄蓝
    //src2.r, src2.g, src2.b 无
    gl_FragColor = vec4(src1.r, src2.g, src1.b, 1.0);
}



