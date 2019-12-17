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

#if __ANDROID__
#include "android_xlog.h"
#elif __APPLE__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#define LOGI
#define LOGE 
#endif

enum UniformType {
    UniformTypeSample2D = 1,
    UniformTypeBoolean = 2,
    UniformTypeFloat = 3,
    UniformTypePoint = 4,
    UniformTypeVec3 = 5,
    UniformTypeMatrix4x4 = 8,
    UniformTypeInputTexture = 100,
    UniformTypeInputTextureLast = 101,
    UniformTypeImageWidth = 200,
    UniformTypeImageHeight = 201,
    UniformTypeTexelWidthOffset = 300,
    UniformTypeTexelHeightOffset = 301,
    UniformTypeFrameTime = 302,
    UniformTypeInputEffectIndex = 1000,
    UniformTypeMattingTexture = 2000,
    UniformTypeRenderCacheKey = 3000,
};

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
    // 执行滤镜操作
    for (auto& filter : filters_) {
        Filter* f = filter.second;
        int process_texture = f->OnDrawFrame(texture, current_time);
        texture = process_texture;
    }
    for (auto& effect : effects_) {
        if (effect->preview) {
            std::vector<FrameBuffer*> frame_buffers = effect->frame_buffers;
            std::vector<FragmentUniforms*> fragment_uniforms = effect->fragment_uniforms;
            for (FrameBuffer* buffer : frame_buffers) {
                buffer->ActiveProgram();
                for (FragmentUniforms *uniforms : fragment_uniforms) {
                    if (uniforms->data_size > 0 && uniforms->data != nullptr &&
                        uniforms->type == UniformTypeFloat) {
                        double data = uniforms->data[uniforms->data_index % uniforms->data_size];
                        buffer->SetFloat(uniforms->name, data);
                        uniforms->data_index++;
                    }
                    if (uniforms->type == UniformTypeInputTexture) {
                        buffer->SetInt(uniforms->name, 0);
                    }
                }
                int process_texture = buffer->OnDrawFrame(texture, current_time);
                texture = process_texture;
            }
            return texture;
        }
    }
    for (auto& effect : effects_) {
        LOGE("effect start_time: %d end_time: %d", effect->start_time, effect->end_time);
        std::vector<FrameBuffer*> frame_buffers = effect->frame_buffers;
        std::vector<FragmentUniforms*> fragment_uniforms = effect->fragment_uniforms;
        for (FrameBuffer* buffer : frame_buffers) {
            buffer->ActiveProgram();
            for (FragmentUniforms* uniforms : fragment_uniforms) {
                if (uniforms->data_size > 0 && uniforms->data != nullptr && uniforms->type == UniformTypeFloat) {
                    double data = uniforms->data[uniforms->data_index % uniforms->data_size];
                    buffer->SetFloat(uniforms->name, data);
                    uniforms->data_index++;
                }
                if (uniforms->type == UniformTypeInputTexture) {
                    buffer->SetInt(uniforms->name, 0);
                }
            }
            int process_texture = buffer->OnDrawFrame(texture, current_time);
            texture = process_texture;
        }
    }
    return texture;
}

void ImageProcess::OnAction(char* config_path, int action_id) {
    ParseConfig(config_path, action_id);
}

void ImageProcess::OnUpdateAction(int start_time, int end_time, int action_id) {
    LOGI("enter %s start_time: %d end_time: %d action_id: %d", __func__, start_time, end_time, action_id);
    auto it = std::find_if(effects_.begin(), effects_.end(), [action_id](const Effect* effect) -> bool {
         return action_id == effect->action_id;
     });
     if (it == effects_.end()) {
         return;
     }
     for (FrameBuffer* buffer : (*it)->frame_buffers) {
        buffer->SetStartTime(start_time);
        buffer->SetEndTime(end_time);
     }
     (*it)->start_time = start_time;
     (*it)->end_time = end_time;
     (*it)->preview = false;
     LOGI("leave %s", __func__);
}

void ImageProcess::ParseConfig(char *config_path, int action_id) {
    char const* config_name = "/config.json";
    auto* config = new char[strlen(config_path) + strlen(config_name)];
    sprintf(config, "%s%s", config_path, config_name);
    
    char* buffer = nullptr;
    int ret = ReadFile(config, &buffer);
    if (ret != 0 || buffer == nullptr) {
        return;
    }
    cJSON* json = cJSON_Parse(buffer);
    delete[] config;
    delete[] buffer;
    if (nullptr == json) {
        return;
    }
    bool face_detect = false;
    bool matting = false;
    cJSON* requirement = cJSON_GetObjectItem(json, "requirement");
    if (nullptr != requirement) {
        cJSON* face_detect_json = cJSON_GetObjectItem(requirement, "faceDetect");
        if (nullptr != face_detect_json && cJSON_IsBool(face_detect_json)) {
            face_detect = face_detect_json->valueint == 1;
        }
    }
    cJSON* effect_json = cJSON_GetObjectItem(json, "effect");
    if (nullptr == effect_json) {
        cJSON_Delete(json);
        return;
    }
    int effect_size = cJSON_GetArraySize(effect_json);
    auto* effect = new Effect();
    effect->action_id = action_id;
    effect->preview = true;

    for (int i = 0; i < effect_size; ++i) {
        cJSON *effect_item_json = cJSON_GetArrayItem(effect_json, i);
        cJSON *name_json = cJSON_GetObjectItem(effect_item_json, "name");
        cJSON *vertex_shader_json = cJSON_GetObjectItem(effect_item_json, "vertexShader");
        cJSON *fragment_shader_json = cJSON_GetObjectItem(effect_item_json, "fragmentShader");
        cJSON *start_time_json = cJSON_GetObjectItem(effect_item_json, "startTime");
        cJSON *end_time_json = cJSON_GetObjectItem(effect_item_json, "endTime");
        cJSON *vertex_uniforms_json = cJSON_GetObjectItem(effect_item_json, "vertexUniforms");
        cJSON *fragment_uniforms_json = cJSON_GetObjectItem(effect_item_json, "fragmentUniforms");
        if (nullptr != vertex_shader_json && nullptr != fragment_shader_json) {
            char* vertex_shader = vertex_shader_json->valuestring;
            char* fragment_shader = fragment_shader_json->valuestring;
            
            auto* vertex_shader_path = new char[strlen(config_path) + strlen(vertex_shader)];
            sprintf(vertex_shader_path, "%s/%s", config_path, vertex_shader);
            auto* fragment_shader_path = new char[strlen(config_path) + strlen(fragment_shader)];
            sprintf(fragment_shader_path, "%s/%s", config_path, fragment_shader);
            
            char* vertex_shader_buffer = nullptr;
            ret = ReadFile(vertex_shader_path, &vertex_shader_buffer);
            printf("vertex ret: %d\n", ret);
            char* fragment_shader_buffer = nullptr;
            ret = ReadFile(fragment_shader_path, &fragment_shader_buffer);
            printf("fragment ret: %d\n", ret);
            
            if (vertex_shader_buffer != nullptr && fragment_shader_buffer != nullptr) {
                auto* frame_buffer = new FrameBuffer(720, 1280, vertex_shader_buffer, fragment_shader_buffer);
                effect->frame_buffers.push_back(frame_buffer);
                delete[] vertex_shader_buffer;
                delete[] fragment_shader_buffer;
            }
            delete[] fragment_shader_path;
            delete[] vertex_shader_path;
        }
        if (nullptr != fragment_uniforms_json) {
            int fragment_uniforms_size = cJSON_GetArraySize(fragment_uniforms_json);
            for (int uniform_index = 0; uniform_index < fragment_uniforms_size; uniform_index++) {
                auto* fragment_uniform = new FragmentUniforms();
//                memset(fragment_uniform, 0, sizeof(fragment_uniform));
                cJSON* fragment_uniform_item_json = cJSON_GetArrayItem(fragment_uniforms_json, uniform_index);
                cJSON* fragment_uniform_name_json = cJSON_GetObjectItem(fragment_uniform_item_json, "name");
                if (nullptr != fragment_uniform_name_json) {
                    fragment_uniform->name = fragment_uniform_name_json->valuestring;
                }
                cJSON* fragment_uniform_type_json = cJSON_GetObjectItem(fragment_uniform_item_json, "type");
                if (nullptr != fragment_uniform_type_json) {
                    fragment_uniform->type = fragment_uniform_type_json->valueint;
                }
                cJSON* fragment_uniform_data_json = cJSON_GetObjectItem(fragment_uniform_item_json, "data");
                if (nullptr != fragment_uniform_data_json) {
                    int fragment_uniform_data_size = cJSON_GetArraySize(fragment_uniform_data_json);
                    fragment_uniform->data_size = fragment_uniform_data_size;
                    fragment_uniform->data = new double[fragment_uniform_data_size];
                    for (int data_index = 0; data_index < fragment_uniform_data_size; data_index++) {
                        cJSON* data_item_json = cJSON_GetArrayItem(fragment_uniform_data_json, data_index);
                        fragment_uniform->data[data_index] = data_item_json->valuedouble;
                    }
                }
                effect->fragment_uniforms.push_back(fragment_uniform);
            }
        }
    }
    effects_.push_back(effect);
}

int ImageProcess::ReadFile(char *path, char **buffer) {
    FILE *file = fopen(path, "r");
    printf("path: %s\n", path);
    if (file == nullptr) {
        return -1;
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    char* data = new char[sizeof(char) * file_size + 1];
    if (nullptr == data) {
        fclose(file);
        return -2;
    }
    memset(data, 0, sizeof(char) * file_size);
    data[file_size] = '\0';
    size_t read_size = fread(data, 1, file_size, file);
    if (read_size != file_size) {
        fclose(file);
        delete[] data;
        return -3;
    }
    fclose(file);
    *buffer = data;
    return 0;
}

void ImageProcess::RemoveAction(int action_id) {
    auto it = std::find_if(effects_.begin(), effects_.end(), [action_id](const Effect* effect) -> bool {
        return action_id == effect->action_id;
    });
    if (it == effects_.end()) {
        return;
    }
    std::vector<FrameBuffer*> frame_buffers = (*it)->frame_buffers;
    for (FrameBuffer* buffer : frame_buffers) {
        delete buffer;
    }
    frame_buffers.clear();
    effects_.erase(it);
}

void ImageProcess::ClearAction() {
    for (Effect* effect : effects_) {
        for (FrameBuffer* buffer : effect->frame_buffers) {
            delete buffer;
        }
        effect->frame_buffers.clear();

        for (FragmentUniforms* uniforms : effect->fragment_uniforms) {
            if (uniforms->data != nullptr) {
                delete[] uniforms->data;
            }
            uniforms->data_size = 0;
            uniforms->data_index = 0;
            delete uniforms;
        }
    }
    effects_.clear();

    for (auto& filter : filters_) {
        delete filter.second;
    }
    filters_.clear();
}

void ImageProcess::OnFilter(char* config_path, int action_id, int start_time, int end_time) {
    char const* config_name = "/config.json";
    auto* config = new char[strlen(config_path) + strlen(config_name)];
    sprintf(config, "%s%s", config_path, config_name);
    
    char* buffer = nullptr;
    int ret = ReadFile(config, &buffer);
    if (ret != 0 || buffer == nullptr) {
        return;
    }
    cJSON* json = cJSON_Parse(buffer);
    delete[] config;
    delete[] buffer;
    if (nullptr == json) {
        return;
    }
    float intensity = 1.0F;
    cJSON* intensity_json = cJSON_GetObjectItem(json, "intensity");
    if (nullptr != intensity_json) {
        intensity = static_cast<float>(intensity_json->valuedouble);
    }
    cJSON* lut_json = cJSON_GetObjectItem(json, "lut");
    if (lut_json != nullptr) {
        char* lut_value = lut_json->valuestring;
        auto* lut_path = new char[strlen(config_path) + strlen(lut_value)];
        sprintf(lut_path, "%s/%s", config_path, lut_value);

        int lut_width = 0;
        int lut_height = 0;
        int channels = 0;
        unsigned char* lut_buffer = stbi_load(lut_path, &lut_width, &lut_height, &channels, STBI_rgb_alpha);
        delete[] lut_path;

        if (nullptr == lut_buffer) {
            cJSON_Delete(json);
            return;
        }
        if ((lut_width == 512 && lut_height == 512) || (lut_width == 64 && lut_height == 64)) {
            auto result = filters_.find(action_id);
            if (result == filters_.end()) {
                // 添加filter
                auto* filter = new Filter(lut_buffer, 720, 1280);
                filter->SetStartTime(start_time);
                filter->SetEndTime(end_time);
                filter->SetIntensity(intensity);
                filters_.insert(std::pair<int, Filter*>(action_id, filter));
            } else {
                // 更新filter
                Filter* filter = filters_[action_id];
                filter->SetStartTime(start_time);
                filter->SetEndTime(end_time);
                filter->SetIntensity(intensity);
                filter->UpdateLut(lut_buffer, 720, 1280);
            }
        }
        stbi_image_free(lut_buffer);
    }
    cJSON_Delete(json);
}

void ImageProcess::OnDeleteFilter(int action_id) {
    auto result = filters_.find(action_id);
    if (result != filters_.end()) {
        Filter* filter = filters_[action_id];
        delete filter;
        filters_.erase(action_id);
    }
}

}  // namespace trinity
