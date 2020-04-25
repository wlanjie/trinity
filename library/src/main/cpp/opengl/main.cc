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
#include "face.h"

//
//extern "C" {
//#include "cJSON.h"
//}

using namespace trinity;

static float make_land_mark[] = {
    146.69012,
    176.36653,
    147.43333,
    186.29279,
    148.6127,
    196.25972,
    150.38898,
    206.1813,
    152.48157,
    216.00528,
    154.84981,
    225.8128,
    157.70853,
    235.48413,
    161.40039,
    244.83386,
    166.22977,
    253.73709,
    172.2808,
    262.0086,
    179.17354,
    269.65305,
    186.71468,
    276.91913,
    194.64233,
    283.68005,
    203.04936,
    289.8985,
    212.3331,
    295.011,
    222.6648,
    298.11322,
    233.45834,
    298.4821,
    243.34111,
    295.99762,
    251.78839,
    290.81552,
    258.59277,
    283.92175,
    264.51624,
    276.31494,
    270.20312,
    268.6408,
    275.75867,
    260.76782,
    280.8286,
    252.74976,
    285.2124,
    244.36981,
    288.6377,
    235.64056,
    291.3692,
    226.63326,
    293.6554,
    217.45056,
    295.30817,
    208.15834,
    296.2428,
    198.8099,
    296.58636,
    189.4002,
    296.40643,
    180.05132,
    295.93112,
    170.8013,
    162.21164,
    167.41162,
    174.49937,
    159.45103,
    187.7869,
    158.17184,
    201.10698,
    158.83946,
    213.57437,
    161.36235,
    246.06778,
    160.61205,
    256.79514,
    157.1607,
    268.29514,
    155.44969,
    279.71582,
    155.61958,
    289.98227,
    162.22589,
    231.19962,
    180.30101,
    232.2302,
    194.12614,
    233.28546,
    208.23964,
    234.33209,
    222.23563,
    215.19405,
    230.45853,
    226.27249,
    232.43417,
    233.95502,
    234.07236,
    241.27452,
    232.01434,
    250.18289,
    228.55719,
    176.76874,
    181.91612,
    184.9711,
    176.10786,
    203.85977,
    177.34744,
    209.7261,
    184.60504,
    201.41104,
    186.7325,
    184.24933,
    185.87143,
    249.4902,
    182.7622,
    254.61044,
    175.16074,
    272.17493,
    172.2864,
    279.57568,
    177.39516,
    273.39532,
    181.90451,
    257.52173,
    184.12375,
    175.19987,
    166.37537,
    188.13887,
    165.99306,
    201.10573,
    166.80548,
    213.66776,
    168.38591,
    246.4972,
    167.32684,
    257.51013,
    164.85648,
    268.68765,
    163.07877,
    279.74506,
    162.4202,
    194.67284,
    174.71817,
    192.77933,
    187.43085,
    193.52979,
    181.54399,
    263.0868,
    171.80194,
    265.70676,
    184.11147,
    264.54874,
    178.3244,
    219.97232,
    182.86105,
    240.83542,
    181.94067,
    215.22543,
    213.65591,
    248.09625,
    212.21088,
    210.35754,
    223.56622,
    253.65015,
    221.41225,
    197.6759,
    246.77386,
    210.75452,
    245.61713,
    224.03568,
    244.59415,
    233.00378,
    245.66895,
    241.37799,
    243.92227,
    252.1563,
    243.70355,
    262.1295,
    244.0937,
    255.46628,
    257.98907,
    243.60008,
    267.56747,
    233.54153,
    269.89276,
    222.72769,
    268.85553,
    207.3868,
    260.72592,
    203.29584,
    248.47023,
    223.71603,
    249.7411,
    233.09692,
    250.49094,
    241.89365,
    248.96794,
    256.6396,
    246.08023,
    242.3882,
    257.39618,
    233.13391,
    259.26904,
    223.208,
    258.3466,
    194.07779,
    180.45633,
    263.18317,
    177.4411,
};

class MakeFaceDetection : public FaceDetection {
 public:
    virtual void FaceDetector(std::vector<FaceDetectionReport*>& face_detection) {
        FaceDetectionReport* report = new FaceDetectionReport();
        report->SetLandMarks(make_land_mark, 106 * 2);
        report->left = 146;
        report->right = 296;
        report->top = 155;
        report->bottom = 298;
        report->roll = -0.022232315;
        report->score = 0.9999269;
        report->yaw = -0.13338177;
        report->pitch = -0.13473696;
        face_detection.push_back(report);
        face_detection.push_back(report);
    }
};

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    GLFWwindow* window = glfwCreateWindow(512, 512, "OpenGL", nullptr, nullptr);
    if(!window) {
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);

    glDisable(GL_DEPTH_TEST);

    int window_width;
    int window_height;
    glfwGetFramebufferSize(window, &window_width, &window_height);
    
    trinity::OpenGL render_screen(window_width, window_height, DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER);

    int nrChannels;
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    GLuint texture_id;

    unsigned char *data = stbi_load("app/src/main/assets/timg.jpeg", &width, &height, &nrChannels, 0);
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
    
//    unsigned char *prop_data = stbi_load("app/src/main/assets/effect/roseEyeMakeup/FaceMakeupV2_2999/mask2999/mask2999_000.png", &width, &height, &nrChannels, STBI_rgb_alpha);
//    printf("width = %d height = %d\n", width, height);
//
//    GLuint prop_texture_id;
//    glGenTextures(1, &prop_texture_id);
//    glActiveTexture(GL_TEXTURE1);
//    glBindTexture(GL_TEXTURE_2D, prop_texture_id);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, prop_data);
//    glBindTexture(GL_TEXTURE_2D, 0);
//    stbi_image_free(prop_data);

    clock_t start = clock();
    ImageProcess image_process;
    char* name = "app/src/main/assets/effect/countDown";
    auto* detection = new MakeFaceDetection();
    image_process.OnAction(name, 0, detection);
    FacePoint face_point;
    face_point.SetSourceSize(512, 512);
    face_point.SetTargetSize(512, 512);
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        uint64_t current_time = (uint64_t) ((double) (clock() - start) / CLOCKS_PER_SEC * 1000) * 30;
//        printf("current_time: %lld\n", current_time);
        if (current_time == 200) {
//            image_process.OnUpdateAction(0, 300, 0);
        }
        int process_id = image_process.Process(texture_id, current_time, width, height, 0, 0);
//        int process_id = face_point.OnDrawFrame(texture_id, prop_texture_id);
        
        render_screen.ActiveProgram();
        render_screen.ProcessImage(process_id);
        glfwSwapBuffers(window);
        usleep(30 * 1000);
    }
    glDeleteTextures(1, &texture_id);
    glfwTerminate();
    delete detection;
//    while (true) {
//        usleep(20 * 1000);
//    }
    return 0;
}
