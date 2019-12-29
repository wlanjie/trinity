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

#include <iostream>
#include <fstream>

#include <stdio.h>
#include <glfw3.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <time.h>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#include "image_process.h"

#include "opengl.h"
#include "gl.h"
#include "gaussian_blur.h"
#include "blur_split_screen.h"
//
//extern "C" {
//#include "cJSON.h"
//}

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
    ImageProcess image_process;
    image_process.OnAction("param/zoomInOut", 0);
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        uint64_t current_time = (uint64_t) ((double) (clock() - start) / CLOCKS_PER_SEC * 1000) * 10;
//        printf("current_time: %lld\n", current_time);
        if (current_time == 200) {
//            image_process.OnUpdateAction(0, 300, 0);
        }
        int process_id = image_process.Process(texture_id, current_time, width, height, 0, 0);
        
        render_screen.ActiveProgram();
        render_screen.ProcessImage(process_id);
        glfwSwapBuffers(window);
        usleep(30 * 1000);
    }
    glfwTerminate();
    return 0;
}
