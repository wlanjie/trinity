#ifdef GL_ES
precision highp float;
varying highp vec2 textureCoordinate;
uniform highp float uScanLineJitter_x; // length
uniform highp float uScanLineJitter_y; // density
uniform highp float uColorDrift_x; // Color tilt
uniform highp float uColorDrift_y; // Overall offset
#else
varying vec2 textureCoordinate;
uniform float uScanLineJitter_x; // length
uniform float uScanLineJitter_y; // density
uniform float uColorDrift_x; // Color tilt
uniform float uColorDrift_y; // Overall offset
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
    //src1.r, src1.g, src2.b Yellow and blue
    //src1.r, src1.g, src1.b no
    //src1.r, src2.g, src1.b Green pink
    //src1.r, src2.g, src2.b Blue red
    //src2.r, src1.g, src1.b Blue red
    //src2.r, src1.g, src2.b Pink green
    //src2.r, src2.g, src1.b Yellow and blue
    //src2.r, src2.g, src2.b no
    gl_FragColor = vec4(src1.r, src2.g, src1.b, 1.0);
}



