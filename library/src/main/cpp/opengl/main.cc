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
#include "scale.h"
#include "soul_scale.h"
#include "shake.h"
#include "skin_needling.h"
#include "seventy_second.h"
#include "hallucination.h"
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

FrameBuffer* runScale() {
    Scale* frame_buffer = new Scale(512, 512);
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
    frame_buffer->SetScalePercent(scale_percent, 15);
    return frame_buffer;
}

FrameBuffer* runSoulScale() {
    SoulScale* frame_buffer = new SoulScale(512, 512);
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
    return frame_buffer;
}

FrameBuffer* runShake() {
    Shake* frame_buffer = new Shake(512, 512);
    float* shake_scale = new float[15];
    shake_scale[0] = 1.0;
    shake_scale[1] = 1.07;
    shake_scale[2] = 1.1;
    shake_scale[3] = 1.13;
    shake_scale[4] = 1.17;
    shake_scale[5] = 1.2;
    shake_scale[6] = 1.2;
    shake_scale[7] = 1;
    shake_scale[8] = 1;
    shake_scale[9] = 1;
    shake_scale[10] = 1;
    shake_scale[11] = 1;
    shake_scale[12] = 1;
    shake_scale[13] = 1;
    shake_scale[14] = 1;
    shake_scale[15] = 1;
    frame_buffer->SetScalePercent(shake_scale, 15);
    return frame_buffer;
}

FrameBuffer* runSkinNeeding() {
    SkinNeedling* frame_buffer = new SkinNeedling(512, 512);
    std::ifstream out;
    out.open("param/skin_needling.json");
    out.seekg(0, std::ios::end);
    int length = out.tellg();
    if (length > 0) {
        out.seekg(0, std::ios::beg);
        char* buffer = new char[length];
        out.read(buffer, length);
        out.close();
        printf("%s", buffer);
        cJSON* param_json = cJSON_Parse(buffer);
        cJSON* effect_json = cJSON_GetObjectItem(param_json, "effect");
        int effect_size = cJSON_GetArraySize(effect_json);

        for (int i = 0; i < effect_size; i++) {
            cJSON* effect_item_json = cJSON_GetArrayItem(effect_json, i);
            cJSON* fragment_uniforms_json = cJSON_GetObjectItem(effect_item_json, "fragmentUniforms");
            int fragment_uniforms_size = cJSON_GetArraySize(fragment_uniforms_json);
            for (int index = 0; index < fragment_uniforms_size; index++) {
                cJSON* fragment_uniforms_item = cJSON_GetArrayItem(fragment_uniforms_json, index);
                cJSON* name_json = cJSON_GetObjectItem(fragment_uniforms_item, "name");
                char* name = cJSON_GetStringValue(name_json);
//                printf("name: %s", name);
                cJSON* data_json = cJSON_GetObjectItem(fragment_uniforms_item, "data");
                int data_size = cJSON_GetArraySize(data_json);
                float* param_value = new float[data_size];
                for (int data_index = 0; data_index < data_size; data_index++) {
                    cJSON* data_item = cJSON_GetArrayItem(data_json, data_index);
                    double value = data_item->valuedouble;
                    param_value[data_index] = value;
                }
                if (strcmp("lineJitterX", name) == 0) {
                    frame_buffer->setLineJitterX(param_value, data_size);
                }
                if (strcmp("lineJitterY", name) == 0) {
                    frame_buffer->setLineJitterY(param_value, data_size);
                }
                if (strcmp("driftX", name) == 0) {
                    frame_buffer->setDriftX(param_value, data_size);
                }
                if (strcmp("driftY", name) == 0) {
                    frame_buffer->setDriftY(param_value, data_size);
                }
                printf("");
            }
        }
        cJSON_Delete(param_json);
        delete[] buffer;
    }
    return frame_buffer;
}

FrameBuffer* runSeventySecond() {
    SeventySecond* frame_buffer = new SeventySecond(512, 512);
    return frame_buffer;
}

FrameBuffer* runHallucination() {
    Hallucination* frame_buffer = new Hallucination(512, 512);
    std::ifstream out;
    out.open("param/hallucination.json");
    out.seekg(0, std::ios::end);
    int length = out.tellg();
    if (length > 0) {
       out.seekg(0, std::ios::beg);
       char* buffer = new char[length];
       out.read(buffer, length);
       out.close();
       printf("%s", buffer);
       cJSON* param_json = cJSON_Parse(buffer);
       cJSON* effect_json = cJSON_GetObjectItem(param_json, "effect");
       int effect_size = cJSON_GetArraySize(effect_json);
       for (int i = 0; i < effect_size; i++) {
           cJSON* effect_item_json = cJSON_GetArrayItem(effect_json, i);
           cJSON* fragment_uniforms_json = cJSON_GetObjectItem(effect_item_json, "fragmentUniforms");
           int fragment_uniforms_size = cJSON_GetArraySize(fragment_uniforms_json);
           for (int index = 0; index < fragment_uniforms_size; index++) {
               cJSON* fragment_uniforms_item = cJSON_GetArrayItem(fragment_uniforms_json, index);
               cJSON* name_json = cJSON_GetObjectItem(fragment_uniforms_item, "name");
               char* name = cJSON_GetStringValue(name_json);
               printf("name: %s\n", name);
               
               if (strcmp("inputImageTextureLookup", name) == 0) {
                    cJSON* data_json = cJSON_GetObjectItem(fragment_uniforms_item, "data");
                    int data_size = cJSON_GetArraySize(data_json);
                    char* param_value = new char[data_size];
                    char* lookup_path;
                    for (int data_index = 0; data_index < data_size; data_index++) {
                        cJSON* data_item = cJSON_GetArrayItem(data_json, data_index);
                        char* value = data_item->valuestring;
                        lookup_path = value;
//                        param_value[data_index] = value;
                    }
                    
                    int width = 0;
                    int height = 0;
                    int nrChannels = 0;
                    unsigned char *data = stbi_load(lookup_path, &width, &height, &nrChannels, 0);
                    printf("width = %d height = %d\n", width, height);

                    GLuint texture_id = 0; 
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
                    frame_buffer->SetTextureLookupTexture(texture_id);
               } 
               
           }
       }
       cJSON_Delete(param_json);
       delete[] buffer;
    }
    return frame_buffer;
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
    image_process.OnAction("param/blurScreen", 0);
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
