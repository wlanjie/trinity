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
//  main.cc
//  opengl
//
//  Created by wlanjie on 2019/8/26.
//  Copyright © 2019 com.wlanjie.opengl. All rights reserved.
//

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include <stdio.h>
#include <glfw3.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <time.h>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "opengl.h"
#include "gl.h"
#include "gaussian_blur.h"
#include "blur_split_screen.h"
#include "scale.h"
#include "soul_scale.h"

using namespace trinity;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(512, 512, "OpenGL", nullptr, nullptr);
    if(!window) {
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    
    trinity::OpenGL render_screen(512, 512, DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);
    
    int nrChannels;
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    GLuint texture_id;
    
    unsigned char *data = stbi_load("test_image/lenna.png", &width, &height, &nrChannels, 0);
    printf("width = %d height = %d\n", width, height);
    
    glGenTextures(1, &texture_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    
    clock_t start = clock();
    
//    GaussianBlur gaussian_blur(512, 512, 1.0f);
//    BlurSplitScreen blur_split_screen(512, 512);
//    FrameBuffer* frame_buffer = new BlurSplitScreen(512, 512);
//    Scale* frame_buffer = new Scale(512, 512);
    SoulScale* frame_buffer = new SoulScale(512, 512);
    float* scale_percent = new float[15];
    scale_percent[0] = 1.0f;
    scale_percent[1] = 1.07f;
    scale_percent[2] = 1.14f;
    scale_percent[3] = 1.21f;
    scale_percent[4] = 1.26f;
    scale_percent[5] = 1.32f;
    scale_percent[6] = 1.45f;
    scale_percent[7] = 1.53f;
    scale_percent[8] = 1.66f;
    scale_percent[9] = 1.79f;
    scale_percent[10] = 1.96f;
    scale_percent[11] = 1.84f;
    scale_percent[12] = 1.76f;
    scale_percent[13] = 1.67f;
    scale_percent[14] = 1.45f;
    scale_percent[15] = 1.28f;
    
    float* soul_scale = new float[15];
    soul_scale[0] = 1.084553;
    soul_scale[1] = 1.173257;
    soul_scale[2] = 1.266176;
    soul_scale[3] = 1.363377;
    soul_scale[4] = 1.464923;
    soul_scale[5] = 1.570877;
    soul_scale[6] = 1.681300;
    soul_scale[7] = 1.796254;
    soul_scale[8] = 1.915799;
    soul_scale[9] = 2.039995;
    soul_scale[10] = 2.168901;
    soul_scale[11] = 2.302574;
    soul_scale[12] = 2.302574;
    soul_scale[13] = 2.302574;
    soul_scale[14] = 2.302574;
    soul_scale[15] = 2.302574;
    
    frame_buffer->SetScalePercent(soul_scale, 15);
    
    float* mix = new float[15];
    mix[0] = 0.411498;
    mix[1] = 0.340743;
    mix[2] = 0.283781;
    mix[3] = 0.237625;
    mix[4] = 0.199993;
    mix[5] = 0.169133;
    mix[6] = 0.143688;
    mix[7] = 0.122599;
    mix[8] = 0.037117;
    mix[9] = 0.028870;
    mix[10] = 0.022595;
    mix[11] = 0.017788;
    mix[12] = 0.010000;
    mix[13] = 0.010000;
    mix[14] = 0.010000;
    mix[15] = 0.010000;
    
    frame_buffer->SetMixPercent(mix, 15);
    
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        uint64_t current_time = (uint64_t) ((double) (clock() - start) / CLOCKS_PER_SEC * 1000);
//        printf("time: %lld\n", current_time);
        int process_id = frame_buffer->OnDrawFrame(texture_id);
        render_screen.ProcessImage(process_id);
        glfwSwapBuffers(window);
        usleep(30 * 1000);
    }
    glfwTerminate();
    return 0;
}
