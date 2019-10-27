//
//  seventy_second.h
//  opengl
//  70s特效
//  Created by wlanjie on 2019/10/26.
//  Copyright © 2019 com.wlanjie.opengl. All rights reserved.
//

#ifndef seventy_second_h
#define seventy_second_h

#include "frame_buffer.h"

#ifdef __ANDROID__
static const char* SEVENTY_SECOND_VERTEX_SHADER = 
    "";
    
static const char* SEVENTY_SECOND_FRAGMENT_SHADER = 
    "";
#else
static const char* SEVENTY_SECOND_VERTEX_SHADER = 
    "attribute vec4 position;                                                                                                                                   \n"
    "attribute vec4 inputTextureCoordinate;                                                                                                                     \n"
    "varying vec2 uv;                                                                                                                                           \n"
    "varying vec2 conformedUV;                                                                                                                                  \n"
    "varying vec2 renderSize;                                                                                                                                   \n"
    "uniform int imageWidth;                                                                                                                                    \n"
    "uniform int imageHeight;                                                                                                                                   \n"
    "vec2 calculateHdConformedUV(vec2 uv, vec2 renderSize) {                                                                                                    \n"
    "   float longestDim = max(renderSize.x, renderSize.y);                                                                                                     \n"
    "   float ratio = longestDim / 1280.0;                                                                                                                      \n"
    "   vec2 hdConformedUV = vec2(0.0);                                                                                                                         \n"
    "   if (renderSize.y >= renderSize.x) {                                                                                                                     \n"
    "       hdConformedUV.x = uv.x * renderSize.x / (renderSize.x / ratio);                                                                                     \n"
    "       hdConformedUV.y = uv.y * ratio;                                                                                                                     \n"
    "   } else {                                                                                                                                                \n"
    "       hdConformedUV.x = uv.x * ratio;                                                                                                                     \n"
    "       hdConformedUV.y = uv.y * renderSize.y / (renderSize.y / ratio);                                                                                     \n"
    "   }                                                                                                                                                       \n"
    "   return hdConformedUV;                                                                                                                                   \n"
    "}                                                                                                                                                          \n"
    "void main() {                                                                                                                                              \n"
    "   gl_Position = position;                                                                                                                                 \n"
    "   uv = position.xy * 0.5 + 0.5;                                                                                                                           \n"
    "   renderSize = vec2(imageWidth, imageHeight);                                                                                                             \n"
    "   conformedUV = calculateHdConformedUV(uv, renderSize);                                                                                                   \n"
    "}                                                                                                                                                          \n";
    
static const char* SEVENTY_SECOND_FRAGMENT_SHADER = 
    "varying vec2 conformedUV;                                                                                                                                  \n"
    "varying vec2 uv;                                                                                                                                           \n"
    "varying vec2 renderSize;                                                                                                                                   \n"
    "uniform float uTime;                                                                                                                                       \n"
    "uniform sampler2D inputImageTexture;                                                                                                                       \n"
    "vec3 permute(vec3 x) {                                                                                                                                     \n"
    "    return mod(((x*34.0)+1.0)*x, vec3(289.0));                                                                                                             \n"
    "}                                                                                                                                                          \n"
    "float snoise(vec2 v) {                                                                                                                                     \n"
    "    const vec4 C = vec4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0                                                                                         \n"
    "                        0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)                                                                                         \n"
    "                        -0.577350269189626,  // -1.0 + 2.0 * C.x                                                                                           \n"
    "                        0.024390243902439); // 1.0 / 41.0                                                                                                  \n"
        // First corner
    "    vec2 i  = floor(v + dot(v, C.yy) );                                                                                                                    \n"
    "    vec2 x0 = v -   i + dot(i, C.xx);                                                                                                                      \n"
        
        // Other corners
    "    vec2 i1;                                                                                                                                               \n"
        //i1.x = step( x0.y, x0.x ); // x0.x > x0.y ? 1.0 : 0.0
        //i1.y = 1.0 - i1.x;
    "    i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);                                                                                                  \n"
        // x0 = x0 - 0.0 + 0.0 * C.xx ;
        // x1 = x0 - i1 + 1.0 * C.xx ;
        // x2 = x0 - 1.0 + 2.0 * C.xx ;
    "    vec4 x12 = x0.xyxy + C.xxzz;                                                                                                                           \n"
    "    x12.xy -= i1;                                                                                                                                          \n"
        
        // Permutations
    "    i = mod(i, vec2(289.0)); // Avoid truncation effects in permutation                                                                                    \n"
    "    vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 )) + i.x + vec3(0.0, i1.x, 1.0 ));                                                                \n"
        
    "    vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);                                                                     \n"
    "    m = m*m;                                                                                                                                               \n"
    "    m = m*m;                                                                                                                                               \n"
        
        // Gradients: 41 points uniformly over a line, mapped onto a diamond.
        // The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
        
    "    vec3 x = 2.0 * fract(p * C.www) - 1.0;                                                                                                                 \n"
    "    vec3 h = abs(x) - 0.5;                                                                                                                                 \n"
    "    vec3 ox = floor(x + 0.5);                                                                                                                              \n"
    "    vec3 a0 = x - ox;                                                                                                                                      \n"
        
        // Normalise gradients implicitly by scaling m
        // Approximation of: m *= inversesqrt( a0*a0 + h*h );
    "    m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );                                                                                            \n"
        
        // Compute final noise value at P
    "    vec3 g;                                                                                                                                                \n"
    "    g.x  = a0.x  * x0.x  + h.x  * x0.y;                                                                                                                    \n"
    "    g.yz = a0.yz * x12.xz + h.yz * x12.yw;                                                                                                                 \n"
    "    return 130.0 * dot(m, g);                                                                                                                              \n"
    "}                                                                                                                                                          \n"
    "float funcexp(float x, float cyclePeriod, float onThreshold) {                                                                                             \n"
    "    float xx = (mod(x,cyclePeriod) - cyclePeriod + onThreshold) / onThreshold;                                                                             \n"
    "    float y = xx*xx*xx;                                                                                                                                    \n"    
    "    return step(cyclePeriod - onThreshold, mod(x,cyclePeriod)) * y;                                                                                        \n"
    "}                                                                                                                                                          \n"
    "const float PI_4_4 = 3.14159265359;                                                                                                                        \n"
    "void main() {                                                                                                                                              \n"
    "    float fastNoise = snoise(vec2(sin(uTime) * 50.));                                                                                                      \n"
    "    float slowNoise = clamp(abs(snoise(vec2(uTime / 20., 0.))), 0., 1.);                                                                                   \n"
    "    float weight1 = funcexp(uTime, 10., 0.5);                                                                                                              \n"
    "    float weight2 = funcexp(uTime - 3., 10., 0.5);                                                                                                         \n"
    "    float weight3 = funcexp(uTime - 7., 10., 0.5);                                                                                                         \n"
    "    float distorsion1 = 1. + step(2.92, mod(uTime + 100., 3.)) * (abs(sin(PI_4_4 * 12.5 * (uTime + 100.))) * 40.) * (slowNoise + .3) + (weight1 * 50. + weight3 * 50.) * fastNoise; \n"
    "    float distOffset = -snoise(vec2((uv.y - uTime) * 3., .0)) * .2;                                                                                        \n"    
    "    distOffset = distOffset*distOffset*distOffset * distorsion1*distorsion1 + snoise(vec2((uv.y - uTime) * 50., 0.)) * ((weight2 * 50.) * fastNoise) * .001; \n"
    "    vec2 finalDistOffset = vec2(fract(uv.x + distOffset), fract(uv.y - uTime * weight3 * 50. * fastNoise));                                                \n"
    "    vec2 offset = ((step(4., mod(uTime, 5.)) * abs(sin(PI_4_4 * uTime)) / 4.) * slowNoise + weight2 * 5. * fastNoise) * vec2(cos(-.5784902357984), sin(-.5784902357984)); \n"
    "    vec4 cr = texture2D(inputImageTexture, finalDistOffset + offset);                                                                                      \n"    
    "    vec4 cg = texture2D(inputImageTexture, finalDistOffset);                                                                                               \n"
    "    vec4 cb = texture2D(inputImageTexture, finalDistOffset - offset);                                                                                      \n"    
    "    vec3 color = vec3(cr.r, cg.g, cb.b);                                                                                                                   \n"
    "    vec2 sc = vec2(sin(uv.y * 800.), cos(uv.y * 800.));                                                                                                    \n"
    "    vec3 copy = color + color * vec3(sc.x, sc.y, sc.x) * .5;;                                                                                              \n"
    "    color = mix(color, copy, .4);                                                                                                                          \n"
    "    vec2 st = floor(renderSize * uv / vec2(4. * conformedUV / uv));                                                                                     \n"    
    "    vec4 snow = vec4(abs(fract(sin(dot(st * uTime,vec2(1.389,30.28))) * 753.39032)) * (abs(slowNoise)/5. + (weight1 * 1. + weight2 * 5. + weight3 * 10.) * fastNoise)); \n"
    "    color += vec3(snow);                                                                                                                                   \n"        
    "    gl_FragColor = vec4(color, 1.);                                                                                                                        \n"
    "}                                                                                                                                                          \n";        
#endif

namespace trinity {

class SeventySecond : public FrameBuffer {

 public:
    SeventySecond(int width, int height) : FrameBuffer(width, height, SEVENTY_SECOND_VERTEX_SHADER, SEVENTY_SECOND_FRAGMENT_SHADER) {
        width_ = width;
        height_ = height;
        time_ = 60;
        index_ = 1;
    }
    
 protected:
    virtual void RunOnDrawTasks() {
        if (width_ > 0 && height_ > 0) {
            SetInt("imageWidth", width_);
            SetInt("imageHeight", height_);
            SetFloat("uTime", time_ * index_);
            index_++;
        }
    }    
 private:
    int width_;
    int height_;
    int time_;
    int index_;        
};  
  
}  // trinity

#endif /* seventy_second_h */
