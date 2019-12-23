#ifdef GL_ES
precision highp float;
#endif

varying vec2 uv;
varying vec2 hdConformedUV;
varying vec2 uRenderSize;

uniform float uTime;
uniform sampler2D inputImageTexture;
uniform sampler2D inputImageTexture2;

float within(float left, float right, float x) { return step(left, x) * (1.0 - step(right, x)); }

float funcpow(float x) { return x * x * x; }

void main() {
    
    float time = uTime;
    
    float modThreshold = 10.;
    float onThreshold = 9.5;
    float weight1 = step(onThreshold, mod(time, modThreshold)) * funcpow((mod(time, modThreshold) - onThreshold) * 2.);
    float weight2 = step(onThreshold, mod(time - 3., modThreshold)) * funcpow((mod(time - 3., modThreshold) - onThreshold) * 2.);
    float weight3 = step(onThreshold, mod(time - 7., modThreshold)) * funcpow((mod(time - 7., modThreshold) - onThreshold) * 2.);
    
    vec2 uvNoise = fract(floor(uRenderSize * uv / vec2(32.0 * hdConformedUV / uv)) / vec2(64.) + floor(vec2(time) * vec2(3291., 2389.)) / vec2(64.));
    
    float blockThresh = fract(time * 3.04) * fract(time * 3.04) * (0.5 + weight1 * 20.0 + weight3 * 100.0);
    float lineThresh = fract(time * 2.392) * fract(time * 2.392) * (0.55 + weight2 * 50.0);
    vec4 noiseBlockLookup = texture2D(inputImageTexture2, uvNoise);
    vec4 noiseLineLookup = texture2D(inputImageTexture2, vec2(uvNoise.y, 0.1));
    float blockOn = 1. - step(blockThresh, noiseBlockLookup.r);
    float lineOn = 1. - step(lineThresh, noiseLineLookup.g * 1.2);
    float blockOrLine = clamp(blockOn + lineOn, 0., 1.);
    
    vec2 dist = (fract(uvNoise) - .5) * (0.8 + weight1 * 100.0);
    vec2 offset = blockOrLine * dist;
    vec2 offsetR = offset * 1.1;
    vec2 offsetG = offset * .2;
    vec2 offsetB = offset * .125;
    float r = texture2D(inputImageTexture, uv + offsetR).r;
    float g = texture2D(inputImageTexture, uv + offsetG).g;
    float b = texture2D(inputImageTexture, uv + offsetB).b;
    
    gl_FragColor = (1. - blockOn) * vec4(vec3(r, g, b), 1.) + blockOn * vec4(vec3(g, g, g), 1.);
    
//    float maskType = fract(uv.y * 100.);
//    vec4 mask = within(0., .333, maskType) * vec4(10., 0., 0., 1.) + within(.333, .666, maskType) * vec4(0., 10., 0., 1.) + within(.666, 1., maskType) * vec4(0., 0., 10., 1.);
//    float coloredLines =
//    clamp(2. - step(blockThresh, noiseBlockLookup.b * 1.5) - step(lineThresh, noiseLineLookup.g * 86.5), 0., 1.);
//    gl_FragColor = (1. - coloredLines) * gl_FragColor + coloredLines * gl_FragColor * mask;
}
