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
 */
//
// Created by wlanjie on 2019-06-05.
//

#ifndef TRINITY_IMAGE_PROCESS_H
#define TRINITY_IMAGE_PROCESS_H

#include <cstdint>
#include <vector>
#include <map>
#include "action.h"
#include "flash_white.h"
#include "filter.h"
#include "blur_split_screen.h"
#include "gaussian_blur.h"

extern "C" {
#include "cJSON.h"
};

#define NO_ACTION -1
#define HORIZONTAL 0
#define VERTICAL 1

namespace trinity {

class ImageProcess {
 public:
    ImageProcess();
    ~ImageProcess();

    int Process(int texture_id, uint64_t current_time,
            int width, int height,
            int input_color_type, int output_color_type);

    int Process(uint8_t* frame, uint64_t current_time,
            int width, int height,
            int input_color_type, int output_color_type);

    void OnAction(char* config, int action_id);
    void RemoveAction(int action_id);
    void ClearAction();

 private:
    void ParseConfig(char* config, int action_id = -1);
    int OnProcess(int texture_id, uint64_t current_time, int width, int height);
    void OnRotate(float rotate, int action_id);
    void OnFlashWhite(int time, uint64_t start_time, uint64_t end_time, int action_id = NO_ACTION);
    void OnSplitScreen(int screen_count, uint64_t start_time, uint64_t end_time, int action_id = NO_ACTION);
    void OnBlurSplitScreen(uint64_t start_time, uint64_t end_time, int action_id = NO_ACTION);
    void OnFilter(int start_time, int end_time, cJSON* json, int action_id);

 private:
    std::map<int, FrameBuffer*> effects_;
};

}  // namespace trinity

#endif  // TRINITY_IMAGE_PROCESS_H
