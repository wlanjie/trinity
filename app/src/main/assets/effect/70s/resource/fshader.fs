#ifdef GL_ES
precision highp float;
#endif
varying vec2 hdConformedUV;
varying vec2 uv;
varying vec2 uRenderSize;
uniform float uTime;
uniform sampler2D inputImageTexture;

vec3 permute(vec3 x) {
    return mod(((x*34.0)+1.0)*x, vec3(289.0));
}

float snoise(vec2 v) {
    const vec4 C = vec4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                        0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                        -0.577350269189626,  // -1.0 + 2.0 * C.x
                        0.024390243902439); // 1.0 / 41.0
    // First corner
    vec2 i  = floor(v + dot(v, C.yy) );
    vec2 x0 = v -   i + dot(i, C.xx);
    
    // Other corners
    vec2 i1;
    //i1.x = step( x0.y, x0.x ); // x0.x > x0.y ? 1.0 : 0.0
    //i1.y = 1.0 - i1.x;
    i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    // x0 = x0 - 0.0 + 0.0 * C.xx ;
    // x1 = x0 - i1 + 1.0 * C.xx ;
    // x2 = x0 - 1.0 + 2.0 * C.xx ;
    vec4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;
    
    // Permutations
    i = mod(i, vec2(289.0)); // Avoid truncation effects in permutation
    vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
                     + i.x + vec3(0.0, i1.x, 1.0 ));
    
    vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
    m = m*m ;
    m = m*m ;
    
    // Gradients: 41 points uniformly over a line, mapped onto a diamond.
    // The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
    
    vec3 x = 2.0 * fract(p * C.www) - 1.0;
    vec3 h = abs(x) - 0.5;
    vec3 ox = floor(x + 0.5);
    vec3 a0 = x - ox;
    
    // Normalise gradients implicitly by scaling m
    // Approximation of: m *= inversesqrt( a0*a0 + h*h );
    m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
    
    // Compute final noise value at P
    vec3 g;
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    g.yz = a0.yz * x12.xz + h.yz * x12.yw;
    return 130.0 * dot(m, g);
}

float funcexp(float x, float cyclePeriod, float onThreshold) {
    float xx = (mod(x,cyclePeriod) - cyclePeriod + onThreshold) / onThreshold;
    float y = xx*xx*xx;
    return step(cyclePeriod - onThreshold, mod(x,cyclePeriod)) * y;
}

const float PI_4_4 = 3.14159265359;

void main() {
    
    float fastNoise = snoise(vec2(sin(uTime) * 50.));
    float slowNoise = clamp(abs(snoise(vec2(uTime / 20., 0.))), 0., 1.);
    float weight1 = funcexp(uTime, 10., 0.5);
    float weight2 = funcexp(uTime - 3., 10., 0.5);
    float weight3 = funcexp(uTime - 7., 10., 0.5);
    
    float distorsion1 = 1. + step(2.92, mod(uTime + 100., 3.)) * (abs(sin(PI_4_4 * 12.5 * (uTime + 100.))) * 40.) * (slowNoise + .3) + (weight1 * 50. + weight3 * 50.) * fastNoise;
    
    float distOffset = -snoise(vec2((uv.y - uTime) * 3., .0)) * .2;
    distOffset = distOffset*distOffset*distOffset * distorsion1*distorsion1 + snoise(vec2((uv.y - uTime) * 50., 0.)) * ((weight2 * 50.) * fastNoise) * .001;
    vec2 finalDistOffset = vec2(fract(uv.x + distOffset), fract(uv.y - uTime * weight3 * 50. * fastNoise));
    
    vec2 offset = ((step(4., mod(uTime, 5.)) * abs(sin(PI_4_4 * uTime)) / 4.) * slowNoise + weight2 * 5. * fastNoise) * vec2(cos(-.5784902357984), sin(-.5784902357984));
    vec4 cr = texture2D(inputImageTexture, finalDistOffset + offset);
    vec4 cg = texture2D(inputImageTexture, finalDistOffset);
    vec4 cb = texture2D(inputImageTexture, finalDistOffset - offset);
    vec3 color = vec3(cr.r, cg.g, cb.b);
    
    vec2 sc = vec2(sin(uv.y * 800.), cos(uv.y * 800.));
    vec3 copy = color + color * vec3(sc.x, sc.y, sc.x) * .5;;
    color = mix(color, copy, .4);
    
    vec2 st = floor(uRenderSize * uv / vec2(4. * hdConformedUV / uv));
    vec4 snow = vec4(abs(fract(sin(dot(st * uTime,vec2(1.389,30.28))) * 753.39032)) * (abs(slowNoise)/5. + (weight1 * 1. + weight2 * 5. + weight3 * 10.) * fastNoise));
    color += vec3(snow);
    
    gl_FragColor = vec4(color, 1.);
}
