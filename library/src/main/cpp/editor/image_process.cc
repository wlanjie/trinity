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

#include "image_process.h"
#include <utility>
#include "android_xlog.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define FILTER "Filter"
#define FLASH_WHITE "FlashWhite"
#define SPLIT_SCREEN "SplitScreen"

namespace trinity {

ImageProcess::ImageProcess() {
}

ImageProcess::~ImageProcess() {
    ClearAction();
}

int ImageProcess::Process(int texture_id, uint64_t current_time, int width, int height, int input_color_type, int output_color_type) {
    return OnProcess(texture_id, current_time, width, height);
}

int ImageProcess::Process(uint8_t *frame, uint64_t current_time, int width, int height, int input_color_type, int output_color_type) {
    return OnProcess(0, current_time, width, height);
}

int ImageProcess::OnProcess(int texture_id, uint64_t current_time, int width, int height) {
    int texture = texture_id;
    for (auto& effect : effects_) {
        FrameBuffer* frame_buffer = effect.second;
        int process_texture = frame_buffer->OnDrawFrame(texture, current_time);
        texture = process_texture;
    }
    return texture;
}

void ImageProcess::OnAction(char* config, int action_id) {
    ParseConfig(config, action_id);
}

void ImageProcess::ParseConfig(char *config, int action_id) {
    cJSON* json = cJSON_Parse(config);
    if (nullptr != json) {
        cJSON* start_time_json = cJSON_GetObjectItem(json, "startTime");
        cJSON* end_time_json = cJSON_GetObjectItem(json, "endTime");
        int start_time = 0;
        if (nullptr != start_time_json) {
            start_time = start_time_json->valueint;
        }
        int end_time = INT_MAX;
        if (nullptr != end_time_json) {
            end_time = end_time_json->valueint;
        }
        cJSON* effect_type_json = cJSON_GetObjectItem(json, "effectType");
        if (nullptr != effect_type_json) {
            char* effect_type = effect_type_json->valuestring;
            if (strcmp(effect_type, FILTER) == 0) {
                OnFilter(start_time, end_time, json, action_id);
            } else if (strcmp(effect_type, FLASH_WHITE) == 0) {
                OnFlashWhite(10, start_time, end_time, action_id);
            } else if (strcmp(effect_type, SPLIT_SCREEN) == 0) {
                int split_count = 0;
                cJSON* split_count_json = cJSON_GetObjectItem(json, "splitScreenCount");
                if (nullptr != split_count_json) {
                    split_count = split_count_json->valueint;
                }
                OnSplitScreen(split_count, start_time, end_time, action_id);
            }
        }
        cJSON_Delete(json);
    }
}

void ImageProcess::RemoveAction(int action_id) {
    auto result = effects_.find(action_id);
    if (result != effects_.end()) {
        FrameBuffer* frame_buffer = effects_[action_id];
        delete frame_buffer;
        effects_.erase(action_id);
    }
}

void ImageProcess::ClearAction() {
    for (auto& effect : effects_) {
        FrameBuffer* buffer = effect.second;
        delete buffer;
    }
    effects_.clear();
}

void ImageProcess::OnFilter(int start_time, int end_time, cJSON* json, int action_id) {
    cJSON* intensity_json = cJSON_GetObjectItem(json, "intensity");
    float intensity = 1.0f;
    if (nullptr != intensity_json) {
        intensity = static_cast<float>(intensity_json->valuedouble);
    }
    cJSON* lut_json = cJSON_GetObjectItem(json, "lut");
    if (nullptr != lut_json) {
        char* lut_path = lut_json->valuestring;
        int lut_width = 0;
        int lut_height = 0;
        int channels = 0;
        unsigned char* lut_buffer = stbi_load(lut_path, &lut_width, &lut_height, &channels, STBI_rgb_alpha);
        if (nullptr == lut_buffer) {
            return;
        }
        if ((lut_width == 512 && lut_height == 512) || (lut_width == 64 && lut_height == 64)) {
            auto result = effects_.find(action_id);
            if (result == effects_.end()) {
                Filter *filter = new Filter(lut_buffer, 720, 1280);
                filter->SetStartTime(start_time);
                filter->SetEndTime(end_time);
                filter->SetIntensity(intensity);
                effects_.insert(std::pair<int, FrameBuffer *>(action_id, filter));
            } else {
                FrameBuffer *frame_buffer = effects_[action_id];
                frame_buffer->SetStartTime(start_time);
                frame_buffer->SetEndTime(end_time);
                Filter *filter = dynamic_cast<Filter *>(frame_buffer);
                if (nullptr != filter) {
                    filter->SetIntensity(intensity);
                    filter->UpdateLut(lut_buffer, 720, 1280);
                }
            }
        }
        stbi_image_free(lut_buffer);
    }
}

void ImageProcess::OnRotate(float rotate, int action_id) {
}

void ImageProcess::OnFlashWhite(int time, uint64_t start_time, uint64_t end_time, int action_id) {
    auto result = effects_.find(action_id);
    if (result == effects_.end()) {
        FlashWhite* flash_write = new FlashWhite(720, 1280);
        flash_write->SetStartTime(start_time);
        flash_write->SetEndTime(end_time);
        effects_.insert(std::pair<int, FrameBuffer*>(action_id, flash_write));
    } else {
        FrameBuffer* frame_buffer = effects_[action_id];
        frame_buffer->SetStartTime(start_time);
        frame_buffer->SetEndTime(end_time);
//        FlashWhite* flash_white = dynamic_cast<FlashWhite*>(frame_buffer);
//        if (nullptr != flash_white) {
//
//        }
    }
}

void ImageProcess::OnSplitScreen(int screen_count, uint64_t start_time, uint64_t end_time, int action_id) {
    auto result = effects_.find(action_id);
    if (result == effects_.end()) {
        FrameBuffer* frame_buffer = nullptr;
        if (screen_count == 2) {
            frame_buffer = new FrameBuffer(720, 1280, DEFAULT_VERTEX_SHADER, SCREEN_TWO_FRAGMENT_SHADER);
        } else if (screen_count == 3) {
            frame_buffer = new FrameBuffer(720, 1280, DEFAULT_VERTEX_SHADER, SCREEN_THREE_FRAGMENT_SHADER);
        } else if (screen_count == 4) {
            frame_buffer = new FrameBuffer(720, 1280, DEFAULT_VERTEX_SHADER, SCREEN_FOUR_FRAGMENT_SHADER);
        } else if (screen_count == 6) {
            frame_buffer = new FrameBuffer(720, 1280, DEFAULT_VERTEX_SHADER, SCREEN_SIX_FRAGMENT_SHADER);
        } else if (screen_count == 9) {
            frame_buffer = new FrameBuffer(720, 1280, DEFAULT_VERTEX_SHADER, SCREEN_NINE_FRAGMENT_SHADER);
        }
        if (frame_buffer == nullptr) {
            return;
        }
        frame_buffer->SetStartTime(start_time);
        frame_buffer->SetEndTime(end_time);
        effects_.insert(std::pair<int, FrameBuffer*>(action_id, frame_buffer));
    } else {
        FrameBuffer* frame_buffer = effects_[action_id];
        frame_buffer->SetStartTime(start_time);
        frame_buffer->SetEndTime(end_time);
    }
}

}  // namespace trinity
