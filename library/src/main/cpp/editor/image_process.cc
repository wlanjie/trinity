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

#define FILTER                  "Filter"
#define FLASH_WHITE             "FlashWhite"
#define SPLIT_SCREEN            "SplitScreen"
#define BLUR_SPLIT_SCREEN       "BlurSplitScreen"
#define SOUL_SCALE              "SoulScale"
#define SHAKE                   "Shake"
#define SKIN_NEEDLING           "SkinNeedling"

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
        cJSON* effect_json = cJSON_GetObjectItem(json, "effect");
        if (nullptr != effect_json) {
            int effect_size = cJSON_GetArraySize(effect_json);
            for (int i = 0; i < effect_size; ++i) {
                cJSON *effect_item_json = cJSON_GetArrayItem(effect_json, i);
                cJSON *name_json = cJSON_GetObjectItem(effect_item_json, "name");
                cJSON *vertex_shader_json = cJSON_GetObjectItem(effect_item_json, "vertexShader");
                cJSON *fragment_shader_json = cJSON_GetObjectItem(effect_item_json, "fragmentShader");
                cJSON *start_time_json = cJSON_GetObjectItem(effect_item_json, "startTime");
                cJSON *end_time_json = cJSON_GetObjectItem(effect_item_json, "endTime");
                cJSON *vertex_uniforms_json = cJSON_GetObjectItem(effect_item_json, "vertexUniforms");
                cJSON *fragment_uniforms_json = cJSON_GetObjectItem(effect_item_json, "fragmentUniforms");

                if (nullptr == name_json) {
                    continue;
                }
                char* name = name_json->valuestring;
                char* vertex_shader = nullptr;
                if (nullptr != vertex_shader_json) {
                    vertex_shader = vertex_shader_json->valuestring;
                }
                char* fragment_shader = nullptr;
                if (nullptr != fragment_shader_json) {
                    fragment_shader = fragment_shader_json->valuestring;
                }
                int start_time = 0;
                if (nullptr != start_time_json) {
                    start_time = start_time_json->valueint;
                }
                int end_time = INT_MAX;
                if (nullptr != end_time_json) {
                    end_time = end_time_json->valueint;
                }
                int vertex_uniforms_size = 0;
                if (nullptr != vertex_uniforms_json) {
                    vertex_uniforms_size = cJSON_GetArraySize(vertex_uniforms_json);
                }
                if (strcmp(name, FILTER) == 0) {
                    // 滤镜
                    OnFilter(start_time, end_time, effect_item_json, action_id);
                } else if (strcmp(name, FLASH_WHITE) == 0) {
                    // 闪白
                    OnFlashWhite(fragment_uniforms_json, start_time, end_time, action_id);
                } else if (strcmp(name, SPLIT_SCREEN) == 0) {
                    // 分屏
                    OnSplitScreen(fragment_uniforms_json, start_time, end_time, action_id);
                } else if (strcmp(name, BLUR_SPLIT_SCREEN) == 0) {
                    // 模糊分屏
                    OnBlurSplitScreen(fragment_uniforms_json, start_time, end_time, action_id);
                } else if (strcmp(name, SOUL_SCALE) == 0) {
                    // 灵魂出窍
                    OnSoulScale(fragment_uniforms_json, start_time, end_time, action_id);
                } else if (strcmp(name, SHAKE) == 0) {
                    // 抖动
                    OnShake(fragment_uniforms_json, start_time, end_time, action_id);
                } else if (strcmp(name, SKIN_NEEDLING) == 0) {
                    // 毛刺
                    OnSkinNeedling(fragment_uniforms_json, start_time, end_time, action_id);
                }
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
    LOGI("enter %s start_time: %d end_time: %d action_id: %d", __func__, start_time, end_time, action_id);
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
            LOGE("load filter image error.");
            return;
        }
        if ((lut_width == 512 && lut_height == 512) || (lut_width == 64 && lut_height == 64)) {
            auto result = effects_.find(action_id);
            if (result == effects_.end()) {
                auto *filter = new Filter(lut_buffer, 720, 1280);
                filter->SetStartTime(start_time);
                filter->SetEndTime(end_time);
                filter->SetIntensity(intensity);
                effects_.insert(std::pair<int, FrameBuffer *>(action_id, filter));
            } else {
                FrameBuffer *frame_buffer = effects_[action_id];
                frame_buffer->SetStartTime(start_time);
                frame_buffer->SetEndTime(end_time);
                auto *filter = dynamic_cast<Filter *>(frame_buffer);
                if (nullptr != filter) {
                    filter->SetIntensity(intensity);
                    filter->UpdateLut(lut_buffer, 720, 1280);
                }
            }
        }
        stbi_image_free(lut_buffer);
    }
    LOGI("leave %s", __func__);
}

void ImageProcess::OnRotate(float rotate, int action_id) {
}

void ImageProcess::OnFlashWhite(cJSON* fragment_uniforms, int start_time, int end_time, int action_id) {
    auto result = effects_.find(action_id);
    if (result == effects_.end()) {
        int fragment_uniforms_size = 0;
        if (nullptr != fragment_uniforms) {
            fragment_uniforms_size = cJSON_GetArraySize(fragment_uniforms);
        }
        auto* flash_write = new FlashWhite(720, 1280);
        flash_write->SetStartTime(start_time);
        flash_write->SetEndTime(end_time);
        for (int index = 0; index < fragment_uniforms_size; ++index) {
            cJSON *fragment_uniforms_item_json = cJSON_GetArrayItem(fragment_uniforms, index);
            if (nullptr == fragment_uniforms_item_json) {
                return;
            }
            cJSON *fragment_uniforms_name_json = cJSON_GetObjectItem(fragment_uniforms_item_json, "name");
            cJSON* fragment_uniforms_data_json = cJSON_GetObjectItem(fragment_uniforms_item_json, "data");
            if (nullptr == fragment_uniforms_name_json) {
                return;
            }
            if (nullptr == fragment_uniforms_data_json) {
                return;
            }
            int size = cJSON_GetArraySize(fragment_uniforms_data_json);
            auto* param_value = new float[size];
            for (int i = 0; i < size; i++) {
                cJSON* data_item = cJSON_GetArrayItem(fragment_uniforms_data_json, i);
                auto value = static_cast<float>(data_item->valuedouble);
                param_value[i] = value;
            }
            char* name = fragment_uniforms_name_json->valuestring;
            if (strcmp(name, "alphaTimeLine") == 0) {
                flash_write->SetAlphaTime(param_value, size);
            }
            delete[] param_value;
        }
        effects_.insert(std::pair<int, FrameBuffer*>(action_id, flash_write));
    } else {
        FrameBuffer* frame_buffer = effects_[action_id];
        frame_buffer->SetStartTime(start_time);
        frame_buffer->SetEndTime(end_time);
    }
}

void ImageProcess::OnSplitScreen(cJSON* fragment_uniforms, int start_time, int end_time, int action_id) {
    auto result = effects_.find(action_id);
    if (result == effects_.end()) {
        int fragment_uniforms_size = 0;
        if (nullptr != fragment_uniforms) {
            fragment_uniforms_size = cJSON_GetArraySize(fragment_uniforms);
        }
        int screen_count = 0;
        for (int index = 0; index < fragment_uniforms_size; ++index) {
            cJSON *fragment_uniforms_item_json = cJSON_GetArrayItem(fragment_uniforms, index);
            if (nullptr == fragment_uniforms_item_json) {
                return;
            }
            cJSON *fragment_uniforms_name_json = cJSON_GetObjectItem(fragment_uniforms_item_json, "name");
            if (nullptr == fragment_uniforms_name_json) {
                return;
            }
            cJSON* fragment_uniforms_data_json = cJSON_GetObjectItem(fragment_uniforms_item_json, "data");
            char* name = fragment_uniforms_name_json->valuestring;
            if (strcmp(name, "splitScreenCount") == 0 && nullptr != fragment_uniforms_data_json) {
                screen_count = fragment_uniforms_data_json->valueint;
            }
        }
        if (screen_count == 0) {
            return;
        }
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

void ImageProcess::OnBlurSplitScreen(cJSON* fragment_uniforms, int start_time, int end_time, int action_id) {
    auto result = effects_.find(action_id);
    if (result == effects_.end()) {
        auto* screen = new BlurSplitScreen(720, 1280);
        screen->SetStartTime(start_time);
        screen->SetEndTime(end_time);
        effects_.insert(std::pair<int, FrameBuffer*>(action_id, screen));
    } else {
        auto* screen = effects_[action_id];
        screen->SetStartTime(start_time);
        screen->SetEndTime(end_time);
    }
}

void ImageProcess::OnSoulScale(cJSON* fragment_uniforms, int start_time, int end_time, int action_id) {
    auto result = effects_.find(action_id);
    if (result == effects_.end()) {
        int fragment_uniforms_size = 0;
        if (nullptr != fragment_uniforms) {
            fragment_uniforms_size = cJSON_GetArraySize(fragment_uniforms);
        }
        auto* soul_scale = new SoulScale(720, 1280);
        soul_scale->SetStartTime(start_time);
        soul_scale->SetEndTime(end_time);
        for (int index = 0; index < fragment_uniforms_size; ++index) {
            cJSON *fragment_uniforms_item_json = cJSON_GetArrayItem(fragment_uniforms, index);
            if (nullptr == fragment_uniforms_item_json) {
                return;
            }
            cJSON *fragment_uniforms_name_json = cJSON_GetObjectItem(fragment_uniforms_item_json, "name");
            cJSON* fragment_uniforms_data_json = cJSON_GetObjectItem(fragment_uniforms_item_json, "data");
            if (nullptr == fragment_uniforms_name_json) {
                return;
            }
            if (nullptr == fragment_uniforms_data_json) {
                return;
            }
            char* name = fragment_uniforms_name_json->valuestring;
            int size = cJSON_GetArraySize(fragment_uniforms_data_json);
            auto* param_value = new float[size];
            for (int i = 0; i < size; i++) {
                cJSON* data_item = cJSON_GetArrayItem(fragment_uniforms_data_json, i);
                auto value = static_cast<float>(data_item->valuedouble);
                param_value[i] = value;
            }
            if (strcmp(name, "mixturePercent") == 0) {
                soul_scale->SetMixPercent(param_value, size);
            } else if (strcmp(name, "scalePercent") == 0) {
                soul_scale->SetScalePercent(param_value, size);
            }
            delete[] param_value;
        }
        effects_.insert(std::pair<int, FrameBuffer*>(action_id, soul_scale));
    } else {
        FrameBuffer* frame_buffer = effects_[action_id];
        frame_buffer->SetStartTime(start_time);
        frame_buffer->SetEndTime(end_time);
    }
}

void ImageProcess::OnShake(cJSON* fragment_uniforms, int start_time, int end_time, int action_id) {
    auto result = effects_.find(action_id);
    if (result == effects_.end()) {
        int fragment_uniforms_size = 0;
        if (nullptr != fragment_uniforms) {
            fragment_uniforms_size = cJSON_GetArraySize(fragment_uniforms);
        }
        auto* shake = new Shake(720, 1280);
        shake->SetStartTime(start_time);
        shake->SetEndTime(end_time);
        for (int index = 0; index < fragment_uniforms_size; ++index) {
            cJSON *fragment_uniforms_item_json = cJSON_GetArrayItem(fragment_uniforms, index);
            if (nullptr == fragment_uniforms_item_json) {
                return;
            }
            cJSON *fragment_uniforms_name_json = cJSON_GetObjectItem(fragment_uniforms_item_json, "name");
            cJSON* fragment_uniforms_data_json = cJSON_GetObjectItem(fragment_uniforms_item_json, "data");
            if (nullptr == fragment_uniforms_name_json) {
                return;
            }
            if (nullptr == fragment_uniforms_data_json) {
                return;
            }
            char* name = fragment_uniforms_name_json->valuestring;
            int size = cJSON_GetArraySize(fragment_uniforms_data_json);
            auto* param_value = new float[size];
            for (int i = 0; i < size; i++) {
                cJSON* data_item = cJSON_GetArrayItem(fragment_uniforms_data_json, i);
                auto value = static_cast<float>(data_item->valuedouble);
                param_value[i] = value;
            }
            if (strcmp(name, "scale") == 0) {
                shake->SetScalePercent(param_value, size);
            }
            delete[] param_value;
        }
        effects_.insert(std::pair<int, FrameBuffer*>(action_id, shake));
    } else {
        FrameBuffer* frame_buffer = effects_[action_id];
        frame_buffer->SetStartTime(start_time);
        frame_buffer->SetEndTime(end_time);
    }
}

void ImageProcess::OnSkinNeedling(cJSON* fragment_uniforms, int start_time, int end_time, int action_id) {
    auto result = effects_.find(action_id);
    if (result == effects_.end()) {
        int fragment_uniforms_size = 0;
        if (nullptr != fragment_uniforms) {
            fragment_uniforms_size = cJSON_GetArraySize(fragment_uniforms);
        }
        auto* skin_needling = new SkinNeedling(720, 1280);
        skin_needling->SetStartTime(start_time);
        skin_needling->SetEndTime(end_time);
        for (int index = 0; index < fragment_uniforms_size; ++index) {
            cJSON *fragment_uniforms_item_json = cJSON_GetArrayItem(fragment_uniforms, index);
            if (nullptr == fragment_uniforms_item_json) {
                return;
            }
            cJSON *fragment_uniforms_name_json = cJSON_GetObjectItem(fragment_uniforms_item_json, "name");
            cJSON* fragment_uniforms_data_json = cJSON_GetObjectItem(fragment_uniforms_item_json, "data");
            if (nullptr == fragment_uniforms_name_json) {
                return;
            }
            char* name = fragment_uniforms_name_json->valuestring;
            if (strcmp(name, "") == 0 && nullptr != fragment_uniforms_data_json) {
                int size = cJSON_GetArraySize(fragment_uniforms_data_json);
                auto* param_value = new float[size];
                for (int i = 0; i < size; i++) {
                    cJSON* data_item = cJSON_GetArrayItem(fragment_uniforms_data_json, i);
                    auto value = static_cast<float>(data_item->valuedouble);
                    param_value[i] = value;
                }
//                skin_needling->(param_value, size);
                delete[] param_value;
            }
        }
        effects_.insert(std::pair<int, FrameBuffer*>(action_id, skin_needling));
    } else {
        FrameBuffer* frame_buffer = effects_[action_id];
        frame_buffer->SetStartTime(start_time);
        frame_buffer->SetEndTime(end_time);
    }
}

}  // namespace trinity
