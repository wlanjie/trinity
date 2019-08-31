//
//  gaussian_blur.h
//  opengl
//
//  Created by wlanjie on 2019/8/30.
//  Copyright © 2019 com.wlanjie.opengl. All rights reserved.
//

#ifndef gaussian_blur_h
#define gaussian_blur_h

#include <stdio.h>
#include <string>
#include "frame_buffer.h"

namespace trinity {

class VerticalGaussianBlur : public FrameBuffer {
 public:
    VerticalGaussianBlur(int width, int height, const char* vertex_shader, const char* fragment_shader);
    ~VerticalGaussianBlur();
    
 protected:
    virtual void RunOnDrawTasks();
    
 private:
    int width_;
    int height_;
};    
    
class HorizontalGaussianBlur : public FrameBuffer {
 public:
    HorizontalGaussianBlur(int width, int height, const char* vertex_shader, const char* fragment_shader);
    ~HorizontalGaussianBlur();
    
 protected:
    virtual void RunOnDrawTasks();
    
 private:
    int width_;
    int height_; 
};   
    
    
class GaussianBlur {
 public:
    GaussianBlur(int width, int height);
    explicit GaussianBlur(int width, int height, float blur_radius_pixes);
    ~GaussianBlur();
    
    int OnDrawFrame(int texture_id);
private:
    void InitWithBlurSigma(int blur_radius, float sigma);
    void VertexShaderForOptimizedGaussianBlurOfRadius(int blur_radius, float sigma);
    void FragmentShaderForOptimizedGaussianBlurOfRadius(int blur_radius, float sigma);
    
private:
    std::string vertex_shader_;
    std::string fragment_shader_;
    float blur_radius_pixels_;
    int width_;
    int height_;
    VerticalGaussianBlur* vertical_blur_;
    HorizontalGaussianBlur* horizontal_blur_;
};

}

#endif /* gaussian_blur_h */
