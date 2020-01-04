/*
* Copyright (C) 2019 Trinity. All rights reserved.
* Copyright (C) 2019 Wang LianJie <wlanjie888@gmail.com>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

//
//  gaussian_blur.cc
//  opengl
//
//  Created by wlanjie on 2019/8/30.
//  Copyright © 2019 com.wlanjie.opengl. All rights reserved.
//

#include "gaussian_blur.h"
#include <math.h>

#ifdef __ANDROID__
#include "android_xlog.h"
#endif

#define MAX_STRING_SIZE     4096
// #define M_PI                3.14159265358979323846264338327950288
#define MIN(x,y)            ( (x) < (y) ? (x) : (y) )

namespace trinity {

GaussianBlur::GaussianBlur(int width, int height) : blur_radius_pixels_(2.0f) {
    width_ = width;
    height_ = height;
    InitWithBlurSigma(4, 2.0);
}
    
GaussianBlur::GaussianBlur(int width, int height, float blur_radius_pixes) {
    width_ = width;
    height_ = height;
    float value = 24.0f * blur_radius_pixes;
    blur_radius_pixels_ = round(value);
    int calculated_sample_radius = 0;
    if (blur_radius_pixels_ >= 1.0f) {
        float mininum_weight = 1.0f / 256.0f;
        calculated_sample_radius = floor(sqrt(-2.0 * pow(blur_radius_pixels_, 2.0) * log(mininum_weight * sqrt(2.0 * M_PI * pow(blur_radius_pixels_, 2.0)))));
        // There's nothing to gain from handling odd radius sizes, due to the optimizations I use
        calculated_sample_radius += calculated_sample_radius % 2;
    }
    InitWithBlurSigma(calculated_sample_radius, blur_radius_pixels_);
}

GaussianBlur::~GaussianBlur() {
    delete vertical_blur_;
    vertical_blur_ = nullptr;
    delete horizontal_blur_;
    horizontal_blur_ = nullptr;
}
    
int GaussianBlur::OnDrawFrame(int texture_id) {
    int vertical_blur_id = vertical_blur_->OnDrawFrame(texture_id);
    int horizontal_blur_id = horizontal_blur_->OnDrawFrame(vertical_blur_id);
    return horizontal_blur_id;
}
    
void GaussianBlur::InitWithBlurSigma(int blur_radius, float sigma) {
    VertexShaderForOptimizedGaussianBlurOfRadius(blur_radius, sigma);
    FragmentShaderForOptimizedGaussianBlurOfRadius(blur_radius, sigma);
    const char* vertex_shader = vertex_shader_.c_str();
    const char* fragment_shader = fragment_shader_.c_str();
    vertical_blur_ = new VerticalGaussianBlur(width_, height_, vertex_shader, fragment_shader);
    horizontal_blur_ = new HorizontalGaussianBlur(width_, height_, vertex_shader, fragment_shader);
}    

void GaussianBlur::VertexShaderForOptimizedGaussianBlurOfRadius(int blur_radius, float sigma) {
    if (blur_radius < 1) {
        return;
    }
    float *standar_weights = new float[blur_radius + 1];
    float sum_of_weights = 0.0f;
    // First, generate the normal Gaussian weights for a given sigm
    for (int gaussian_weight_index = 0; gaussian_weight_index < blur_radius + 1; gaussian_weight_index++) {
        float weight = (1.0 / sqrt(2.0 * M_PI * pow(sigma, 2.0))) * exp(-pow(gaussian_weight_index, 2.0) / (2.0 * pow(sigma, 2.0)));
        standar_weights[gaussian_weight_index] = weight;
        if (gaussian_weight_index == 0) {
            sum_of_weights += weight;
        } else {
            sum_of_weights += 2.0 * weight;
        }
    }
    // Next, normalize these weights to prevent the clipping of the Gaussian curve at the end of the discrete samples from reducing luminance
    for (int current_blur_coordinate_index = 0; current_blur_coordinate_index < blur_radius + 1; current_blur_coordinate_index++) {
        standar_weights[current_blur_coordinate_index] = standar_weights[current_blur_coordinate_index] / sum_of_weights;
    }
    
    // From these weights we calculate the offsets to read interpolated values from
    int number_of_optimized_offsets = MIN(blur_radius / 2 + (blur_radius % 2), 7);
    float* optimized_gaussian_offsets = new float[number_of_optimized_offsets];
    for (int current_optimized_offset = 0; current_optimized_offset < number_of_optimized_offsets; current_optimized_offset++) {
        float first_weight = standar_weights[current_optimized_offset * 2 + 1];
        float second_weight = standar_weights[current_optimized_offset * 2 + 2];
        float optimized_weight = first_weight + second_weight;
        optimized_gaussian_offsets[current_optimized_offset] = (first_weight * (current_optimized_offset * 2 + 1) + second_weight * (current_optimized_offset * 2 + 2)) / optimized_weight;
    }
    
    char vertex_buffer[MAX_STRING_SIZE];
    sprintf(vertex_buffer, "\
            attribute vec4 position;\n\
            attribute vec4 inputTextureCoordinate;\n\
            uniform float texelWidthOffset;\n\
            uniform float texelHeightOffset;\n\
            varying vec2 blurCoordinates[%d];\n\
            void main() {\n\
            gl_Position = position;\n\
            vec2 singleStepOffset = vec2(texelWidthOffset, texelHeightOffset);\n", (1 + (number_of_optimized_offsets * 2)));
    
    std::string vertex(vertex_buffer);
    sprintf(vertex_buffer, "blurCoordinates[0] = inputTextureCoordinate.xy;\n");
    vertex.append(vertex_buffer);
    
    for (int current_optimized_offset = 0; current_optimized_offset < number_of_optimized_offsets; current_optimized_offset++) {
        sprintf(vertex_buffer, "blurCoordinates[%d] = inputTextureCoordinate.xy + singleStepOffset * %f;\n"\
                "blurCoordinates[%d] = inputTextureCoordinate.xy - singleStepOffset * %f;\n", (current_optimized_offset * 2) + 1, optimized_gaussian_offsets[current_optimized_offset], (current_optimized_offset * 2) + 2, optimized_gaussian_offsets[current_optimized_offset]);
        vertex.append(vertex_buffer);
    }
    vertex.append("}\n");
    delete[] optimized_gaussian_offsets;
    delete[] standar_weights;
    vertex_shader_ = vertex;
}

void GaussianBlur::FragmentShaderForOptimizedGaussianBlurOfRadius(int blur_radius, float sigma) {
    if (blur_radius < 1) {
        return;
    }
    float* standar_weights = new float[blur_radius + 1];
    float sum_of_weights = 0.0;
    for (int current_gaussian_weight_index = 0; current_gaussian_weight_index < blur_radius + 1; current_gaussian_weight_index++) {
        standar_weights[current_gaussian_weight_index] = (1.0 / sqrt(2.0 * M_PI * pow(sigma, 2.0))) * exp(-pow(current_gaussian_weight_index, 2.0) / (2.0 * pow(sigma, 2.0)));
        if (current_gaussian_weight_index == 0) {
            sum_of_weights += standar_weights[current_gaussian_weight_index];
        } else {
            sum_of_weights += 2.0 * standar_weights[current_gaussian_weight_index];
        }
    }
    for (int current_gaussian_weight_index = 0; current_gaussian_weight_index < blur_radius + 1; current_gaussian_weight_index++) {
        standar_weights[current_gaussian_weight_index] = standar_weights[current_gaussian_weight_index] / sum_of_weights;
    }
    int number_of_optimized_offsets = MIN(blur_radius / 2 + (blur_radius % 2), 7);
    int true_number_of_optimized_offsets = blur_radius / 2 + (blur_radius % 2);
    char fragment_buffer[MAX_STRING_SIZE];
    std::string fragment;
#ifdef __ANDROID__
    sprintf(fragment_buffer, "precision highp float;\n");
    fragment.append(fragment_buffer);
#endif
    sprintf(fragment_buffer, "\
            uniform sampler2D inputImageTexture;\n\
            uniform float texelWidthOffset;\n\
            uniform float texelHeightOffset;\n\
            varying vec2 blurCoordinates[%d];\n", 
            (1 + (number_of_optimized_offsets * 2)));
    fragment.append(fragment_buffer);
#if __ANDROID__
    sprintf(fragment_buffer, "void main() {\n\
                             lowp vec4 sum = vec4(0.0);\n");                    
#else
    sprintf(fragment_buffer, "void main() {\n\
                             vec4 sum = vec4(0.0);\n"); 
#endif        
    fragment.append(fragment_buffer);
    sprintf(fragment_buffer, "sum += texture2D(inputImageTexture, blurCoordinates[0]) * %f;\n", standar_weights[0]);
    fragment.append(fragment_buffer);
    for (int current_blur_coordinate_index = 0; current_blur_coordinate_index < number_of_optimized_offsets; current_blur_coordinate_index++) {
        float first_weight = standar_weights[current_blur_coordinate_index * 2 + 1];
        float second_weight = standar_weights[current_blur_coordinate_index * 2 + 2];
        float optimized_weight = first_weight + second_weight;
        sprintf(fragment_buffer, "sum += texture2D(inputImageTexture, blurCoordinates[%d]) * %f;\n", (current_blur_coordinate_index * 2) + 1, optimized_weight);
        fragment.append(fragment_buffer);
        sprintf(fragment_buffer, "sum += texture2D(inputImageTexture, blurCoordinates[%d]) * %f;\n", (current_blur_coordinate_index * 2) + 2, optimized_weight);
        fragment.append(fragment_buffer);
    }
    if (true_number_of_optimized_offsets > number_of_optimized_offsets) {
        sprintf(fragment_buffer, "vec2 singleStepOffset = vec2(texelWidthOffset, texelHeightOffset);\n");
        fragment.append(fragment_buffer);
        
        for (int current_overlow_texture_read = number_of_optimized_offsets; current_overlow_texture_read < true_number_of_optimized_offsets; current_overlow_texture_read++) {
            float first_weight = standar_weights[current_overlow_texture_read * 2 + 1];
            float second_weight = standar_weights[current_overlow_texture_read * 2 + 2];
            float optimized_weight = first_weight + second_weight;
            float optimized_offset = (first_weight * (current_overlow_texture_read * 2 + 1) + second_weight * (current_overlow_texture_read * 2 + 2)) / optimized_weight;
            sprintf(fragment_buffer, "sum += texture2D(inputImageTexture, blurCoordinates[0] + singleStepOffset * %f) * %f;\n", optimized_offset, optimized_weight);
            fragment.append(fragment_buffer);
            
            sprintf(fragment_buffer, "sum += texture2D(inputImageTexture, blurCoordinates[0] - singleStepOffset * %f) * %f;\n", optimized_offset, optimized_weight);
            fragment.append(fragment_buffer);
        }
    }
    fragment.append("gl_FragColor = sum;\n\
                    }\n");
    delete[] standar_weights;
    fragment_shader_ = fragment;
}
    
VerticalGaussianBlur::VerticalGaussianBlur(int width, int height, const char* vertex_shader, const char* fragment_shader) : FrameBuffer(width, height, vertex_shader, fragment_shader) {
    width_ = width;
    height_ = height;
}

VerticalGaussianBlur::~VerticalGaussianBlur() {

}

void VerticalGaussianBlur::RunOnDrawTasks() {
    SetFloat("texelWidthOffset", 0.0f);
    SetFloat("texelHeightOffset", 1.0f / height_); 
}

HorizontalGaussianBlur::HorizontalGaussianBlur(int width, int height, const char* vertex_shader, const char* fragment_shader) : FrameBuffer(width, height, vertex_shader, fragment_shader) {
    width_ = width;
    height_ = height;
}

HorizontalGaussianBlur::~HorizontalGaussianBlur() {

}

void HorizontalGaussianBlur::RunOnDrawTasks() {
    SetFloat("texelWidthOffset", 1.0f / width_);
    SetFloat("texelHeightOffset", 0.0f);
}

}
