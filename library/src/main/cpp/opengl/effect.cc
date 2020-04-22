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
// Created by wlanjie on 2019-12-20.
//

#include "effect.h"
#ifdef __ANDROID__
#include "android_xlog.h"
#else
#define LOGE(format, ...) fprintf(stdout, format, __VA_ARGS__)
#define LOGI(format, ...) fprintf(stdout, format, __VA_ARGS__)
#endif

namespace trinity {

GeneralSubEffect::GeneralSubEffect() {
    LOGI("enter: %s", __func__);
}

GeneralSubEffect::~GeneralSubEffect() {
    LOGI("enter: %s", __func__);
}

int GeneralSubEffect::OnDrawFrame(FaceDetection* face_detection,
        std::list<SubEffect*> sub_effects,
        int origin_texture_id, int texture_id,
        int width, int height, uint64_t current_time) {
    ProcessBuffer* process_buffer = GetProcessBuffer();
    if (nullptr == process_buffer) {
        return texture_id;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, process_buffer->GetFrameBufferId());
    process_buffer->SetOutput(720, 1280);
    process_buffer->ActiveProgram();
    process_buffer->Clear();
    process_buffer->ActiveAttribute();
    if (nullptr != param_name) {
        process_buffer->SetFloat(param_name, param_value);
    }
    // TODO 如果需要传入face坐标,需要根据人脸数量执行多次
    SetUniform(sub_effects, process_buffer, fragment_uniforms, origin_texture_id, texture_id, current_time);
    SetUniform(sub_effects, process_buffer, vertex_uniforms, origin_texture_id, texture_id, current_time);
    process_buffer->DrawArrays();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return process_buffer->GetTextureId();
}

glm::vec2 Transform::Center(float aspect, FaceDetectionReport* face_detection, int source_width, int source_height) {
    if (face_detect && source_width != 0 && source_height != 0) {
        glm::vec2 scale = ScaleSize(aspect, face_detection, source_width, source_height);
        float* land_marks = face_detection->key_points;
        int land_mark_size = face_detection->key_point_size;
        if (nullptr == land_marks || land_mark_size == 0) {
            return glm::vec2(0, 0);
        }
        if (positions.empty()) {
            return glm::vec2(0, 0);
        }
        Position* position = positions[0];
        glm::vec2 anchor_point = glm::vec2(position->anchor[0], position->anchor[1]);
        glm::vec2 anchor_face_point = glm::vec2(0, 0);
        for (auto point : position->points) {
            int idx = point->idx;
            float weight = point->weight;
            #ifdef __APPLE__
            glm::vec2 face_point = glm::vec2(land_marks[idx * 2] / source_width, 1.0F - land_marks[idx * 2 + 1] / source_height);
            #else
            glm::vec2 face_point = glm::vec2(land_marks[idx * 2] / source_width, 1.0F - land_marks[idx * 2 + 1] / source_height);
            #endif
            anchor_face_point = anchor_face_point + (face_point * weight);
        }
        glm::vec2 center = glm::vec2(0.5F, 0.5F);
        center = glm::vec2(anchor_face_point.x + (center.x - anchor_point.x) * scale.x,
                anchor_face_point.y + (center.y - anchor_point.y) * scale.y);
        center = glm::vec2(center.x * 2 - 1, center.y * 2 - 1);
        return center;
    }
    return glm::vec2(0., 0.);
}

glm::vec2 Transform::ScaleSize(float aspect, FaceDetectionReport* face_detection, int source_width, int source_height) {
    if (face_detect && nullptr != scale && source_width != 0 && source_height != 0) {
        if (nullptr != scale->scale_x) {
            float* land_marks = face_detection->key_points;
            int land_mark_size = face_detection->key_point_size;
            if (nullptr == land_marks || land_mark_size == 0) {
                return glm::vec2(1.F, 1.F / aspect);
            }
            double scale_x = scale->scale_x->factor;
            glm::vec2 anchor_face_point1 = glm::vec2(land_marks[4 * 2] / source_width, land_marks[4 * 2 + 1] / source_height);
            glm::vec2 anchor_face_point2 = glm::vec2(land_marks[28 * 2] / source_width, land_marks[28 * 2 + 1] / source_height);
            float distance = glm::distance(anchor_face_point1, anchor_face_point2);
            return glm::vec2(scale_x * distance, scale_x * distance / aspect);
        }
    }
    return glm::vec2(1.F, 1.F / aspect);
}

// StickerSubEffect
StickerSubEffect::StickerSubEffect()
    : blendmode(-1)
    , width(0)
    , height(0)
    , fps(0)
    , alpha_factor(1.0F)
    , pic_index(0)
    , face_detect(false)
    , input_aspect(1.0F)
    , blend(nullptr)
    , begin_frame_time(0) {
    LOGI("enter: %s", __func__);
}

StickerSubEffect::~StickerSubEffect() {
    LOGI("enter: %s", __func__);
    sticker_idxs.clear();
    for (auto& path : sticker_paths) {
        delete[] path;
    }
    sticker_paths.clear();
    auto image_buffer_iterator = image_buffers.begin();
    while (image_buffer_iterator != image_buffers.end()) {
        delete image_buffer_iterator->second;
        image_buffer_iterator++;
    }
    image_buffers.clear();
    if (nullptr != blend) {
        delete blend;
        blend = nullptr;
    }
}

int StickerSubEffect::OnDrawFrame(FaceDetection* face_detection,
        std::list<SubEffect *> sub_effects,
        int origin_texture_id, int texture_id,
        int width, int height, uint64_t current_time) {
    return texture_id;
}

ImageBuffer* StickerSubEffect::StickerBufferAtFrameTime(float time) {
    auto count = sticker_idxs.size();
    int index;
    if (fps == 0) {
        index = pic_index % count;
        pic_index++;
    } else {
        float duration = 0;
        if (begin_frame_time == 0) {
            begin_frame_time = time;
        } else {
            duration = (time - begin_frame_time) * 1.0F / 1000;
        }
        index = static_cast<int>(duration * fps) % count;
    }
    int final_index = sticker_idxs.at(index);
    auto image_buffer_iterator = image_buffers.find(final_index);
    if (image_buffer_iterator != image_buffers.end()) {
        return image_buffer_iterator->second;
    }
    char* image_path = sticker_paths.at(final_index);
    int sample_texture_width = 0;
    int sample_texture_height = 0;
    int channels = 0;
    unsigned char* sample_texture_buffer = stbi_load(image_path,
            &sample_texture_width, &sample_texture_height, &channels, STBI_rgb_alpha);
    if (nullptr != sample_texture_buffer && sample_texture_width > 0 && sample_texture_height > 0) {
//        stbi_write_png("/Users/wlanjie/Desktop/offscreen.png",
//                       sample_texture_width, sample_texture_height, 4,
//                       sample_texture_buffer + (sample_texture_width * 4 * (sample_texture_height - 1)),
//                       -sample_texture_width * 4);
        auto* image_buffer = new ImageBuffer(sample_texture_width, sample_texture_height, sample_texture_buffer);
        stbi_image_free(sample_texture_buffer);
        image_buffers.insert(std::pair<int, ImageBuffer*>(final_index, image_buffer));
        return image_buffer;
    }
    return nullptr;
}

glm::mat4 StickerSubEffect::VertexMatrix(FaceDetectionReport* face_detection, int source_width, int source_height) {
    return glm::mat4();
}

// StickerFace
StickerFace::StickerFace() : frame_count(0) {
    LOGI("enter: %s", __func__);
}

StickerFace::~StickerFace() {
    LOGI("enter: %s", __func__);
    for (auto position : positions) {
        delete position;
    }
    positions.clear();
    for (auto scale : scales) {
        delete scale;
    }
    scales.clear();
    for (auto rotate : rotates) {
        delete rotate;
    }
    rotates.clear();
}

glm::mat4 StickerFace::VertexMatrix(FaceDetectionReport *face_detection, int source_width, int source_height) {
    glm::mat4 model_matrix = glm::mat4(1.0);
    bool is_invalid = face_detection->HasFace() || face_detect
        || positions.empty() || scales.empty();
    if (!is_invalid) {
        return model_matrix;
    }
    float* land_marks = face_detection->key_points;
    float aspect = source_width * 1.0F / source_height;
    float sticker_aspect = width * 1.0F / height;
    StickerFace::Position* position1 = positions[0];
    StickerFace::Position* position2 = positions[positions.size() - 1];
    glm::vec2 anchor_face_point1 = glm::vec2(land_marks[position1->index * 2] / source_width,
        land_marks[position1->index * 2 + 1] / source_height);
    glm::vec2 anchor_face_point2 = glm::vec2(land_marks[position2->index * 2] / source_width,
        land_marks[position2->index * 2 + 1] / source_height);    
    float distance1 = glm::distance(anchor_face_point1, anchor_face_point2);
    float distance2 = fabs(position1->x - position2->x);
    float ratio = positions.size() == 1 ? sticker_aspect : distance1 / distance2;
    glm::vec2 sticker_center = glm::vec2(0.5, 0.5);
    sticker_center = glm::vec2(anchor_face_point2.x + (sticker_center.x - position2->x) * ratio,
        anchor_face_point2.y + (sticker_center.y - position2->y) / sticker_aspect * ratio);
    sticker_center = glm::vec2(sticker_center.x * 2 - 1, sticker_center.y * 2 - 1);
    // 贴纸长宽
    StickerFace::Scale* scale1 = scales[0];
    StickerFace::Scale* scale2 = scales[scales.size() - 1];
    glm::vec2 scale_face_point1 = glm::vec2(land_marks[scale1->index * 2] / source_width,
        land_marks[scale1->index * 2 + 1] / source_height);
    glm::vec2 scale_face_point2 = glm::vec2(land_marks[scale2->index * 2] / source_width,
        land_marks[scale2->index * 2 + 1] / source_height);
    distance1 = glm::distance(scale_face_point1, scale_face_point2);
    distance2 = fabs(scale1->x - scale2->x);
    float ndc_sticker_width = distance1 / distance2;
    float ndc_sticker_height = ndc_sticker_width / sticker_aspect;
    glm::vec2 rotation_center = glm::vec2(land_marks[43 * 2] / source_width * 2 - 1,
        land_marks[43 * 2 + 1] / source_height * 2 - 1);
    float pitch_angle = face_detection->pitch;
    float yaw_angle = face_detection->yaw;
    float roll_angle = face_detection->roll;
    if (fabs(yaw_angle) > M_PI / 180.0 * 50.0) {
        yaw_angle = (yaw_angle / fabs(yaw_angle)) * M_PI / 180.0 * 50.0;
    }
    if (fabs(pitch_angle) > M_PI / 180.0 * 30.0) {
        pitch_angle = (pitch_angle / fabs(pitch_angle)) * M_PI / 180.0 * 30.0;
    }
    // 围绕rotationCenter旋转
    float projeciton_scale = 2;
    glm::mat4 projection = glm::frustum(-1 / projeciton_scale, 1 / projeciton_scale, 
        -1 / aspect / projeciton_scale, 1 / aspect / projeciton_scale, 5.0F, 100.0F);
    glm::mat4 view_matrix = glm::lookAt(glm::vec3(0, 0, 10), 
        glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
    model_matrix = glm::translate(model_matrix, glm::vec3(rotation_center.x, rotation_center.y, 0));
    // rotate z
    float z_cos = cosf(roll_angle);
    float z_sin = sinf(roll_angle);
    glm::mat4 rotation_z = { z_cos, z_sin, 0.0F, 0.0F,
                     -z_sin, z_cos, 0.0F, 0.0F,
                     0.0F, 0.0F, 1.0F, 0.0F,
                     0.0F, 0.0F, 0.0F, 1.0F};
    model_matrix = model_matrix * rotation_z;
    // rotate y
    float y_cos = cosf(yaw_angle);
    float y_sin = sinf(yaw_angle);
    glm::mat4 rotation_y = {
        y_cos, 0.0F, -y_sin, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        y_sin, 0.0F, y_cos, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F
    };
    model_matrix = model_matrix * rotation_y;
    // rotate x
    float x_cos = cosf(pitch_angle);
    float x_sin = sinf(pitch_angle);
    glm::mat4 rotation_x = {
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, x_cos, x_sin, 0.0F,
        0.0F, -x_sin, x_cos, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F
    };
    model_matrix = model_matrix * rotation_x;
    model_matrix = glm::translate(model_matrix, glm::vec3(-rotation_center.x, -rotation_center.y, 0));
    // 移动贴图到目标位置
    model_matrix = glm::translate(model_matrix, glm::vec3(sticker_center.x, -sticker_center.y, 0));
    model_matrix = glm::scale(model_matrix, glm::vec3(ndc_sticker_width, ndc_sticker_height, 1.0F));
    glm::mat4 model_view_matrix = view_matrix * model_matrix;
    glm::mat4 mvp = projection * model_view_matrix;
    return mvp;
}

int StickerFace::OnDrawFrame(FaceDetection* face_detection,
    std::list<SubEffect *> sub_effects,
    int origin_texture_id, int texture_id,
    int width, int height, uint64_t current_time) {
    int sticker_texture_id = texture_id;
    if (nullptr != blend) {
        std::vector<FaceDetectionReport*> face_detections;
        face_detection->FaceDetector(face_detections);
        ImageBuffer* image_buffer = StickerBufferAtFrameTime(current_time);
        if (nullptr != image_buffer) {
            for (auto face_detection_report : face_detections) {
                glm::mat4 matrix = VertexMatrix(face_detection_report, width, height);
                float* m = const_cast<float*>(glm::value_ptr(matrix));
                sticker_texture_id = blend->OnDrawFrame(sticker_texture_id, image_buffer->GetTextureId(), width, height, m, alpha_factor);
            }
        }
    }
    return sticker_texture_id;
}

// StickerV3
StickerV3SubEffect::StickerV3SubEffect()
    : transform(nullptr) {
    LOGI("enter: %s", __func__);
}

StickerV3SubEffect::~StickerV3SubEffect() {
    LOGI("enter: %s", __func__);
    if (nullptr != transform) {
        delete transform;
        transform = nullptr;
    }
}

int StickerV3SubEffect::OnDrawFrame(FaceDetection* face_detection,
        std::list<SubEffect *> sub_effects,
        int origin_texture_id, int texture_id,
        int width, int height, uint64_t current_time) {
    int sticker_texture_id = texture_id;
    if (nullptr != blend) {
        ImageBuffer* image_buffer = StickerBufferAtFrameTime(current_time);
        if (nullptr != image_buffer) {
            float* matrix = nullptr;
            if (face_detect && nullptr != face_detection) {
                std::vector<FaceDetectionReport*> face_reports;
                face_detection->FaceDetector(face_reports);
                if (!face_reports.empty()) {
                    for (auto& report : face_reports) {
                        bool has_face = report->HasFace();
                        if (has_face) {
                            glm::mat4 m = VertexMatrix(report, width, height);
                            matrix = const_cast<float*>(glm::value_ptr(m));
                            sticker_texture_id = blend->OnDrawFrame(sticker_texture_id, image_buffer->GetTextureId(), 
                                width, height, matrix, alpha_factor);
                        }
                        delete report;
                    }
                }
            } else {
                sticker_texture_id = blend->OnDrawFrame(sticker_texture_id, image_buffer->GetTextureId(), 
                    width, height, matrix, alpha_factor);
            }
        }
    }
    return sticker_texture_id;
}

glm::mat4 StickerV3SubEffect::VertexMatrix(FaceDetectionReport* face_detection, int source_width, int source_height) {
    glm::mat4 model_matrix = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    if (width == 0 || height == 0 || !face_detection->HasFace()) {
        return model_matrix;
    }
    if (nullptr == transform) {
        return model_matrix;
    }
    if (width == 0 || height == 0) {
        return model_matrix;
    }
    float sticker_aspect = width * 1.0F / height;
    float input_aspect = source_width * 1.0F / source_height;
    glm::vec2 center = transform->Center(sticker_aspect, face_detection, source_width, source_height);
    glm::vec2 scale = transform->ScaleSize(sticker_aspect, face_detection, source_width, source_height);
    float roll_angle = 0;
    glm::vec2 rotation_center = glm::vec2(0, 0);
    if (face_detect) {
        float* land_marks = face_detection->key_points;
        rotation_center = glm::vec2(land_marks[16 * 2] / WIDTH * 2 - 1, land_marks[16 * 2 + 1] / HEIGHT * 2 - 1);
        roll_angle = face_detection->roll;
    }
    glm::mat4 projection = glm::ortho(-1.0F, 1.0F, -1.0F / input_aspect, 1.0F / input_aspect, 0.F, 100.F);
    model_matrix = glm::translate(model_matrix, glm::vec3(center.x, center.y, 0));
    float cos = cosf(roll_angle);
    float sin = sinf(roll_angle);
    glm::mat4 rotation_z = { cos, sin, 0.0F, 0.0F,
                    -sin, cos, 0.0F, 0.0F,
                    0.0F, 0.0F, 1.0F, 0.0F,
                    0.0F, 0.0F, 0.0F, 1.0F};
    model_matrix = model_matrix * rotation_z;
    model_matrix = glm::scale(model_matrix, glm::vec3(scale.x, scale.y, 1.0F));
    glm::mat4 mvp = projection * model_matrix;
    return mvp;
}

FaceMakeupV2SubEffect::FaceMakeupV2SubEffect()
    : face_makeup_v2_(nullptr)
    , face_markup_render_(nullptr) {
    face_markup_render_ = new FaceMarkupRender();
    LOGI("enter: %s", __func__);
}

FaceMakeupV2SubEffect::~FaceMakeupV2SubEffect() {
    LOGI("enter: %s", __func__);
    delete face_makeup_v2_;
    delete face_markup_render_;
}

int FaceMakeupV2SubEffect::OnDrawFrame(FaceDetection* face_detection, std::list<trinity::SubEffect *> sub_effects,
                                       int origin_texture_id, int texture_id,
                                       int width, int height, uint64_t current_time) {
    if (nullptr == face_markup_render_) {
        return origin_texture_id;
    }
    std::vector<FaceDetectionReport*> face_reports;
    face_detection->FaceDetector(face_reports);
    auto face_markup_filters = face_makeup_v2_->filters;
    int prop_texture_id = texture_id;
    for (auto& face_markup_filter : face_markup_filters) {
        for (auto &face_detection_report : face_reports) {
            if (face_detection_report->HasFace()) {
                float *texture_coordinate = face_detection_report->TextureCoordinates(
                        face_markup_filter->x,
                        face_markup_filter->y, face_markup_filter->width,
                        face_markup_filter->height);
                prop_texture_id = face_markup_filter->face_markup_render->OnDrawFrame(prop_texture_id,
                                                        face_markup_filter->image_buffer->GetTextureId(),
                                                        width, height,
                                                        face_markup_filter->blend_mode,
                                                        face_markup_filter->intensity,
                                                        texture_coordinate,
                                                        face_detection_report);
            }
//            delete face_detection_report;
        }
    }
    return prop_texture_id;
}

glm::mat4 FaceMakeupV2SubEffect::VertexMatrix(int source_width, int source_height) {
    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };;
}

// SubEffect
SubEffect::SubEffect()
    : process_buffer_(nullptr)
    , enable(true)
    , name(nullptr)
    , type(nullptr)
    , zorder(0)
    , param_name(nullptr)
    , param_value(0) {}

SubEffect::~SubEffect() {
    LOGI("enter: %s", __func__);
    if (nullptr != type) {
        delete[] type;
        type = nullptr;
    }
    if (nullptr != name) {
        delete[] name;
        name = nullptr;
    }
    if (nullptr != param_name) {
        delete[] param_name;
        param_name = nullptr;
    }
    for (auto& fragment_uniform : fragment_uniforms) {
            delete[] fragment_uniform->name;
            for (auto& image_buffer : fragment_uniform->image_buffer_values) {
                delete image_buffer;
            }
            fragment_uniform->image_buffer_values.clear();
            fragment_uniform->float_values.clear();
    }
    fragment_uniforms.clear();
    for (auto& vertex_uniform : vertex_uniforms) {
            delete[] vertex_uniform->name;
            for (auto& image_buffer : vertex_uniform->image_buffer_values) {
                delete image_buffer;
            }
            vertex_uniform->image_buffer_values.clear();
            vertex_uniform->float_values.clear();
    }
    vertex_uniforms.clear();
    for (auto& input_effect_name : input_effect) {
        delete[] input_effect_name;
    }
    input_effect.clear();
    if (nullptr != process_buffer_) {
        process_buffer_->Destroy();
        delete process_buffer_;
        process_buffer_ = nullptr;
    }
}

void SubEffect::InitProcessBuffer(char *vertex_shader, char *fragment_shader) {
    process_buffer_ = new ProcessBuffer();
    process_buffer_->Init(vertex_shader, fragment_shader);
}

void SubEffect::SetFloat(ShaderUniforms *uniform, ProcessBuffer *process_buffer) {
    if (nullptr == process_buffer) {
        return;
    }
    if (uniform->float_values.empty()) {
        return;
    }
    int index = uniform->data_index % uniform->float_values.size();
    float value = uniform->float_values.at(index);
    uniform->data_index++;
    process_buffer->SetFloat(uniform->name, value);
}

void SubEffect::SetSample2D(ShaderUniforms *uniform, ProcessBuffer *process_buffer) {
    if (nullptr == process_buffer) {
        return;
    }
    if (uniform->image_buffer_values.empty()) {
        return;
    }
    int index = uniform->data_index % uniform->image_buffer_values.size();
    ImageBuffer* image_buffer = uniform->image_buffer_values.at(index);
    SetTextureUnit(uniform, process_buffer, image_buffer->GetTextureId());
}

void SubEffect::SetTextureUnit(ShaderUniforms *uniform, ProcessBuffer *process_buffer, GLuint texture) {
    if (nullptr == process_buffer) {
        return;
    }
    GLenum target = GL_TEXTURE3 + uniform->texture_unit_index;
    glActiveTexture(target);
    glBindTexture(GL_TEXTURE_2D, texture);
    process_buffer->SetInt(uniform->name, uniform->texture_unit_index + 3);
}

void SubEffect::SetUniform(std::list<SubEffect*> sub_effects, ProcessBuffer *process_buffer,
        std::vector<ShaderUniforms*> uniforms, int origin_texture_id, int texture_id, uint64_t current_time) {
    int texture_unit_index = 0;
    for (auto& fragment_uniform : uniforms) {
        int type = fragment_uniform->type;
        bool need_texture_unit = type == UniformTypeSample2D || type == UniformTypeInputTexture
                || type == UniformTypeInputTextureLast || type == UniformTypeRenderCacheKey
                || type == UniformTypeMattingTexture || type == UniformTypeInputEffectIndex;
        if (need_texture_unit) {
            fragment_uniform->texture_unit_index = texture_unit_index;
            texture_unit_index += 1;
        }
        switch (fragment_uniform->type) {
            case UniformTypeInputTexture:
                SetTextureUnit(fragment_uniform, process_buffer, origin_texture_id);
                break;
            case UniformTypeMattingTexture:
            case UniformTypeInputTextureLast:
                break;
            case UniformTypeInputEffectIndex: {
                if (!input_effect.empty()) {
                    char* name = input_effect.at(fragment_uniform->input_effect_index);
                    for (auto iterator = sub_effects.begin(); iterator != sub_effects.end(); iterator++) {
                        SubEffect* sub_effect = *iterator;
                        if (nullptr != sub_effect->name) {
                            ProcessBuffer* process_buffer = sub_effect->GetProcessBuffer();
                            if (strcmp(sub_effect->name, name) == 0 && nullptr != process_buffer) {
                                SetTextureUnit(fragment_uniform, process_buffer_, process_buffer->GetTextureId());
                            }
                        }
                    }
                }
                break;
            }
            case UniformTypeRenderCacheKey:
                break;
            case UniformTypeSample2D:
                SetSample2D(fragment_uniform, process_buffer);
                break;
            case UniformTypeBoolean:
                break;
            case UniformTypeFloat:
                SetFloat(fragment_uniform, process_buffer);
                break;
            case UniformTypePoint:
                break;
            case UniformTypeVec3:
                break;
            case UniformTypeMatrix4x4:
                break;
            case UniformTypeImageWidth:
                process_buffer->SetInt(fragment_uniform->name, 720);
                break;
            case UniformTypeImageHeight:
                process_buffer->SetInt(fragment_uniform->name, 1280);
                break;
            case UniformTypeTexelWidthOffset:
                printf("UniformTypeTexelWidthOffset name; %s\n", fragment_uniform->name);
                process_buffer->SetFloat(fragment_uniform->name, 1.0F / 360);
                break;
            case UniformTypeTexelHeightOffset:
                printf("UniformTypeTexelHeightOffset name; %s\n", fragment_uniform->name);
                process_buffer->SetFloat(fragment_uniform->name, 1.0F / 640);
                break;
            case UniformTypeFrameTime:
                printf("UniformTypeFrameTime name: %s time: %f\n", fragment_uniform->name, 1.0F * current_time / 1000);
                process_buffer->SetFloat(fragment_uniform->name, 1.0F * current_time / 1000);
                break;
            default:
                break;
        }
    }
}

// Effect
Effect::Effect()
    : start_time_(0)
    , end_time_(INT32_MAX)
    , face_detection_(nullptr) {}

Effect::~Effect() {
    for (auto& sub_effect : sub_effects_) {
        delete sub_effect;
    }
    sub_effects_.clear();
}

void Effect::SetFaceDetection(trinity::FaceDetection *face_detection) {
    face_detection_ = face_detection;
}

int Effect::OnDrawFrame(GLuint texture_id, int width, int height, uint64_t current_time) {
    int origin_texture_id = texture_id;
    int texture = texture_id;
    for (auto& sub_effect : sub_effects_) {
        if (!sub_effect->enable) {
            continue;
        }
        if (current_time >= start_time_ && current_time <= end_time_) {
            texture = sub_effect->OnDrawFrame(face_detection_, sub_effects_, origin_texture_id, 
                texture, width, height, current_time);
        }
    }
    return texture;
}

void Effect::UpdateTime(int start_time, int end_time) {
    start_time_ = start_time;
    end_time_ = end_time;
}

void Effect::UpdateParam(const char *effect_name, const char *param_name, float value) {
    for (auto iterator = sub_effects_.begin(); iterator != sub_effects_.end(); iterator++) {
        SubEffect* sub_effect = *iterator;
        if (strcmp(effect_name, sub_effect->name) == 0) {
            if (nullptr != sub_effect->param_name) {
                delete[] sub_effect->param_name;
            }
            size_t len = strlen(param_name) + 1;
            sub_effect->param_name = new char[len];
            memcpy(sub_effect->param_name, param_name, len);
            sub_effect->param_value = value;
//            ProcessBuffer* process_buffer = sub_effect->GetProcessBuffer();
//            if (nullptr != process_buffer) {
//                process_buffer->SetFloat(param_name, value);
//            }
        }
    }
}

bool SortSubEffect(SubEffect* s1, SubEffect* s2) {
    return s1->zorder < s2->zorder;
}

void Effect::ParseConfig(char *config_path) {
    char const* config_name = "config.json";
    std::string config;
    config.append(config_path);
    config.append("/");
    config.append(config_name);
    LOGE("%s config: %s", __func__, config.c_str());
    char* buffer = nullptr;
    int ret = ReadFile(config, &buffer);
    if (ret != 0 || buffer == nullptr) {
        return;
    }
    cJSON* json = cJSON_Parse(buffer);
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
    for (int i = 0; i < effect_size; ++i) {
        cJSON *effect_item_json = cJSON_GetArrayItem(effect_json, i);
        cJSON* type_json = cJSON_GetObjectItem(effect_item_json, "type");
        char* type = nullptr;
        if (nullptr != type_json) {
            char* value = type_json->valuestring;
            type = CopyValue(value);
        }
        bool enable = true;
        cJSON* enable_json = cJSON_GetObjectItem(effect_item_json, "enable");
        if (nullptr != enable_json) {
            enable = enable_json->valueint == 1;
        }
        cJSON* zorder_json = cJSON_GetObjectItem(effect_item_json, "zorder");
        int zorder = 0;
        if (nullptr != zorder_json) {
            zorder = zorder_json->valueint;
        }
            // TODO delete sub_effect
        if (nullptr != type) {
            if (strcasecmp(type, "2DStickerV3") == 0) {
                cJSON* path_json = cJSON_GetObjectItem(effect_item_json, "path");
                if (nullptr == path_json) {
                    return;
                }
                char* path_value = path_json->valuestring;
                std::string path;
                path.append(config_path);
                path.append("/");
                path.append(path_value);
                Parse2DStickerV3(path, zorder);
            } else if (strcasecmp(type, "generalEffect") == 0) {
                auto* general_sub_effect = new GeneralSubEffect();
                ConvertGeneralConfig(effect_item_json, config_path, general_sub_effect);
                general_sub_effect->type = type;
                general_sub_effect->enable = enable;
                sub_effects_.push_back(general_sub_effect);
            } else if (strcasecmp(type, "faceMakeupV2") == 0) {
                auto* face_makeup_v2_effect = new FaceMakeupV2SubEffect();
                ConvertFaceMakeupV2(effect_item_json, config_path, face_makeup_v2_effect);
                face_makeup_v2_effect->type = type;
                face_makeup_v2_effect->enable = enable;
                face_makeup_v2_effect->zorder = zorder;
                sub_effects_.push_back(face_makeup_v2_effect);
            } else if (strcasecmp(type, "2DSticker") == 0) {
//                auto* sticker_face = new StickerFace();
//                sticker_face->zorder = zorder;
//                sticker_face->enable = enable;
                std::list<SubEffect*> sticker_face_effect;
                ConvertStickerFaceConfig(effect_item_json, config_path, sticker_face_effect);
                for (auto& sub_effect : sticker_face_effect) {
                    sub_effects_.push_back(sub_effect);
                }
                sticker_face_effect.clear();
            } else if (strcasecmp(type, "filter") == 0) {
                #ifndef __APPLE__
                auto* filter_effect = new FilterSubEffect();
                filter_effect->zorder = zorder;
                ConvertFilter(effect_item_json, config_path, filter_effect);
                sub_effects_.push_back(filter_effect);
                #endif
            }
        }
    }
    sub_effects_.sort(SortSubEffect);
}

char* Effect::CopyValue(char* src) {
    size_t len = strlen(src);
    char* value = new char[len + 1];
    memset(value, 0, len * sizeof(char));
    sprintf(value, "%s", src);
    value[len] = '\0';
    return value;
}

int Effect::ReadFile(const std::string& path, char **buffer) {
    FILE *file = fopen(path.c_str(), "r");
    printf("path: %s\n", path.c_str());
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
    printf("%s\n", data);
    *buffer = data;
    return 0;
}

void Effect::ConvertStickerConfig(cJSON *effect_item_json, char *resource_root_path, SubEffect *sub_effect) {}

void Effect::ConvertGeneralConfig(cJSON *effect_item_json, char* resource_root_path,
        GeneralSubEffect* general_sub_effect) {
    int ret = 0;
    cJSON *vertex_shader_json = cJSON_GetObjectItem(effect_item_json, "vertexShader");
    cJSON *fragment_shader_json = cJSON_GetObjectItem(effect_item_json, "fragmentShader");
    cJSON *vertex_uniforms_json = cJSON_GetObjectItem(effect_item_json, "vertexUniforms");
    cJSON *fragment_uniforms_json = cJSON_GetObjectItem(effect_item_json, "fragmentUniforms");
    cJSON* name_json = cJSON_GetObjectItem(effect_item_json, "name");
    if (nullptr != name_json) {
        char* value = name_json->valuestring;
        general_sub_effect->name = CopyValue(value);
    }
    cJSON* zorder_json = cJSON_GetObjectItem(effect_item_json, "zorder");
    if (nullptr != zorder_json) {
        general_sub_effect->zorder = zorder_json->valueint;
    }
    cJSON* input_effect_json = cJSON_GetObjectItem(effect_item_json, "inputEffect");
    if (nullptr != input_effect_json) {
        int input_effect_size = cJSON_GetArraySize(input_effect_json);
        for (int input_effect_index = 0; input_effect_index < input_effect_size; input_effect_index++) {
            cJSON* input_effect_value = cJSON_GetArrayItem(input_effect_json, input_effect_index);
            if (input_effect_value->type == cJSON_String) {
                char* value = input_effect_value->valuestring;
                general_sub_effect->input_effect.push_back(CopyValue(value));
            }
        }
    }
    if (nullptr != vertex_shader_json && nullptr != fragment_shader_json) {
        char* vertex_shader = vertex_shader_json->valuestring;
        char* fragment_shader = fragment_shader_json->valuestring;
        std::string vertex_shader_path;
        vertex_shader_path.append(resource_root_path);
        vertex_shader_path.append("/");
        vertex_shader_path.append(vertex_shader);
        std::string fragment_shader_path;
        fragment_shader_path.append(resource_root_path);
        fragment_shader_path.append("/");
        fragment_shader_path.append(fragment_shader);
        char* vertex_shader_buffer = nullptr;
        ret = ReadFile(vertex_shader_path, &vertex_shader_buffer);
        printf("vertex ret: %d\n", ret);
        char* fragment_shader_buffer = nullptr;
        ret = ReadFile(fragment_shader_path, &fragment_shader_buffer);
        printf("fragment ret: %d\n", ret);
        if (vertex_shader_buffer != nullptr && fragment_shader_buffer != nullptr) {
            general_sub_effect->InitProcessBuffer(vertex_shader_buffer, fragment_shader_buffer);
            delete[] vertex_shader_buffer;
            delete[] fragment_shader_buffer;
        }
    }
    if (nullptr != fragment_uniforms_json) {
        ParseUniform(general_sub_effect, resource_root_path, fragment_uniforms_json, Fragment);
    }
    if (nullptr != vertex_uniforms_json) {
        ParseUniform(general_sub_effect, resource_root_path, vertex_uniforms_json, Vertex);
    }
}

void Effect::ParseFaceMakeupV2(cJSON *makeup_root_json, const std::string &resource_root_path, FaceMakeupV2* face_makeup) {
    cJSON* face_id_json = cJSON_GetObjectItem(makeup_root_json, "faceID");
    int face_id_size = cJSON_GetArraySize(face_id_json);
    cJSON* filter_json_array = cJSON_GetObjectItem(makeup_root_json, "filters");
    int filter_size = cJSON_GetArraySize(filter_json_array);
    for (int i = 0; i < filter_size; ++i) {
        cJSON* filter_item_json = cJSON_GetArrayItem(filter_json_array, i);
        cJSON* filter_type_json = cJSON_GetObjectItem(filter_item_json, "filterType");
        char* filter_type = filter_type_json->valuestring;
        if (strcmp(filter_type, "pupil") == 0 || strcmp(filter_type, "brow") == 0) {
            continue;
        }
        auto* face_makeup_filter = new FaceMakeupV2Filter;
        face_makeup_filter->filter_type = CopyValue(filter_type_json->valuestring);
        cJSON* sequence_resource_json = cJSON_GetObjectItem(filter_item_json, "2dSequenceResources");
//        cJSON* image_count_json = cJSON_GetObjectItem(sequence_resource_json, "imageCount");
//        face_makeup_filter->image_count = image_count_json->valueint;
        cJSON* name_json = cJSON_GetObjectItem(sequence_resource_json, "name");
        cJSON* path_json = cJSON_GetObjectItem(sequence_resource_json, "path");
        if (name_json == nullptr || path_json == nullptr) {
            continue;
        }
        cJSON* image_interval_json = cJSON_GetObjectItem(sequence_resource_json, "imageInterval");
        face_makeup_filter->image_interval = image_interval_json->valuedouble;
        cJSON* blend_mode_json = cJSON_GetObjectItem(filter_item_json, "blendMode");
        face_makeup_filter->blend_mode = blend_mode_json->valueint;
        cJSON* intensity_json = cJSON_GetObjectItem(filter_item_json, "intensity");
        face_makeup_filter->intensity = intensity_json->valuedouble;
        cJSON* rect_json = cJSON_GetObjectItem(filter_item_json, "rect");
        if (nullptr != rect_json) {
            cJSON* x_json = cJSON_GetObjectItem(rect_json, "x");
            face_makeup_filter->x = x_json->valuedouble;
            cJSON* y_json = cJSON_GetObjectItem(rect_json, "y");
            face_makeup_filter->y = y_json->valuedouble;
            cJSON* width_json = cJSON_GetObjectItem(rect_json, "width");
            face_makeup_filter->width = width_json->valuedouble;
            cJSON* height_json = cJSON_GetObjectItem(rect_json, "height");
            face_makeup_filter->height = height_json->valuedouble;
        }
        std::string image_path_string;
        image_path_string.append(resource_root_path);
        image_path_string.append(path_json->valuestring);
        image_path_string.append(name_json->valuestring);
        image_path_string.append("000.png");

        int sample_texture_width = 0;
        int sample_texture_height = 0;
        int channels = 0;
        unsigned char* sample_texture_buffer = stbi_load(image_path_string.c_str(), &sample_texture_width,
                                                         &sample_texture_height, &channels, STBI_rgb_alpha);
        if (nullptr != sample_texture_buffer && sample_texture_width > 0 && sample_texture_height > 0) {
            auto* image_buffer = new ImageBuffer(sample_texture_width, sample_texture_height,
                                                 sample_texture_buffer);
            stbi_image_free(sample_texture_buffer);
            face_makeup_filter->image_buffer = image_buffer;
        }

        cJSON* z_position_json = cJSON_GetObjectItem(filter_item_json, "zPosition");
        face_makeup_filter->z_position = z_position_json->valueint;
        face_makeup_filter->face_markup_render = new FaceMarkupRender();
        face_makeup->filters.push_back(face_makeup_filter);
    }
}

void Effect::ConvertFaceMakeupV2(cJSON *effect_item_json, char *resource_root_path, FaceMakeupV2SubEffect* face_makeup_v2_sub_effect) {
    cJSON* path_json = cJSON_GetObjectItem(effect_item_json, "path");
    if (nullptr == path_json) {
        return;
    }
    char* path = path_json->valuestring;
    char const* content_name = "content.json";
    std::string content_root_path;
    content_root_path.append(resource_root_path);
    content_root_path.append("/");
    content_root_path.append(path);

    std::string content_path;
    content_path.append(content_root_path);
    content_path.append(content_name);
    char* content_buffer = nullptr;
    LOGE("%s content_path: %s", __func__, content_path.c_str());
    int ret = ReadFile(content_path, &content_buffer);
    if (ret != 0 || nullptr == content_buffer) {
        return;
    }
    cJSON* content_root_json = cJSON_Parse(content_buffer);
    if (nullptr == content_root_json) {
        return;
    }
    cJSON* requirement_json = cJSON_GetObjectItem(content_root_json, "requirement");
    if (nullptr != requirement_json) {
        cJSON* face_detect_json = cJSON_GetObjectItem(requirement_json, "faceDetect");
        face_makeup_v2_sub_effect->face_detect = face_detect_json->valueint == 1;
    }
    cJSON* content_json = cJSON_GetObjectItem(content_root_json, "content");
    if (nullptr == content_json) {
        return;
    }
    cJSON* content_path_json = cJSON_GetObjectItem(content_json, "path");
    if (nullptr == content_path_json) {
        return;
    }
    char* path_value = content_path_json->valuestring;
    std::string clip_path;
    clip_path.append(resource_root_path);
    clip_path.append("/");
    clip_path.append(path);
    clip_path.append(path_value);
    char* makeup_buffer = nullptr;
    LOGE("%s clip_path: %s", __func__, clip_path.c_str());
    ret = ReadFile(clip_path, &makeup_buffer);
    if (ret != 0 || nullptr == makeup_buffer) {
        return;
    }
    cJSON* makeup_root_json = cJSON_Parse(makeup_buffer);
    if (nullptr == makeup_root_json) {
        return;
    }
    auto* face_makeup = new FaceMakeupV2();
    ParseFaceMakeupV2(makeup_root_json, content_root_path, face_makeup);
    face_makeup_v2_sub_effect->face_makeup_v2_ = face_makeup;
}

// convertStickerFace
int Effect::ConvertStickerFaceConfig(cJSON* effect_item_json, char* resource_root_path, std::list<SubEffect*>& sub_effects) {
    cJSON* path_json = cJSON_GetObjectItem(effect_item_json, "path");
    if (nullptr == path_json) {
        return -1;
    }
    char* path = path_json->valuestring;
    char const* content_name = "content.json";
    std::string content_root_path;
    content_root_path.append(resource_root_path);
    content_root_path.append("/");
    content_root_path.append(path);

    std::string content_path;
    content_path.append(content_root_path);
    content_path.append(content_name);
    char* content_buffer = nullptr;
    LOGE("%s content_path: %s", __func__, content_path.c_str());
    int ret = ReadFile(content_path, &content_buffer);
    if (ret != 0 || nullptr == content_buffer) {
        return -1;
    }
    cJSON* content_root_json = cJSON_Parse(content_buffer);
    if (nullptr == content_root_json) {
        return -1;
    }
    bool face_detect = false;
    cJSON* requirement_json = cJSON_GetObjectItem(content_root_json, "requirement");
    if (nullptr != requirement_json) {
        cJSON* face_detect_json = cJSON_GetObjectItem(requirement_json, "faceDetect");
        face_detect = face_detect_json->valueint == 1;
    }
    cJSON* content_json = cJSON_GetObjectItem(content_root_json, "content");
    if (nullptr == content_json) {
        return -1;
    }
    cJSON* content_path_json = cJSON_GetObjectItem(content_json, "path");
    if (nullptr == content_path_json) {
        return -1;
    }
    char* sticker_path = content_path_json->valuestring;
    std::string sticker_config_path;
    sticker_config_path.append(content_root_path);
    sticker_config_path.append(sticker_path);
    char* sticker_config_buffer = nullptr;
    ret = ReadFile(sticker_config_path, &sticker_config_buffer);
    if (ret != 0 || nullptr == content_buffer) {
        return -1;
    }
    cJSON* sticker_config_root_json = cJSON_Parse(sticker_config_buffer);
    if (nullptr == sticker_config_root_json) {
        return -1;
    }
    cJSON* parts_json = cJSON_GetObjectItem(sticker_config_root_json, "parts");
    if (nullptr == parts_json) {
        return -1;
    }
    int parts_size = cJSON_GetArraySize(parts_json);
    for (int i = 0; i < parts_size; i++) {
        auto* sticker_face = new StickerFace();
        sticker_face->face_detect = face_detect;
        
        cJSON* parts_item_json = cJSON_GetArrayItem(parts_json, i);
        char* parts_item_path = parts_item_json->string;
        cJSON* parts_sub_item_json = cJSON_GetObjectItem(parts_json, parts_item_path);
        cJSON* blend_mode_json = cJSON_GetObjectItem(parts_sub_item_json, "blendMode");
        if (nullptr != blend_mode_json) {
            sticker_face->blendmode = blend_mode_json->valueint;
            sticker_face->blend = BlendFactory::CreateBlend(sticker_face->blendmode);
        }
        cJSON* enable_json = cJSON_GetObjectItem(parts_sub_item_json, "enable");
        if (nullptr != enable_json) {
            sticker_face->enable = enable_json->valueint == 1;
        }
        cJSON* frame_count_json = cJSON_GetObjectItem(parts_sub_item_json, "frameCount");
        if (nullptr != frame_count_json) {
            sticker_face->frame_count = frame_count_json->valueint;
        }        cJSON* width_json = cJSON_GetObjectItem(parts_sub_item_json, "width");
        if (nullptr != width_json) {
            sticker_face->width = width_json->valueint;
        }
        if (sticker_face->width == 0) {
            continue;
        }
        cJSON* height_json = cJSON_GetObjectItem(parts_sub_item_json, "height");
        if (nullptr != height_json) {
            sticker_face->height = height_json->valueint;
        }
        if (sticker_face->height == 0) {
            continue;
        }
        cJSON* alpha_factor_json = cJSON_GetObjectItem(parts_sub_item_json, "alphaFactor");
        if (nullptr != alpha_factor_json) {
            sticker_face->alpha_factor = alpha_factor_json->valuedouble;
        }
        cJSON* zorder_json = cJSON_GetObjectItem(parts_sub_item_json, "zorder");
        if (nullptr != zorder_json) {
            sticker_face->zorder = zorder_json->valueint;
        }
        cJSON* fps_json = cJSON_GetObjectItem(parts_sub_item_json, "fps");
        if (nullptr != fps_json) {
            sticker_face->fps = fps_json->valuedouble;
        }
        sticker_face->name = CopyValue(parts_item_path);
        cJSON* position_json = cJSON_GetObjectItem(parts_sub_item_json, "position");
        if (nullptr != position_json) {
            cJSON* position_x_json = cJSON_GetObjectItem(position_json, "positionX");
            int position_x_size = cJSON_GetArraySize(position_x_json);
            for (int i = 0; i < position_x_size; i++) {
                StickerFace::Position* position = new StickerFace::Position();
                cJSON* position_x_item_json = cJSON_GetArrayItem(position_x_json, i);
                cJSON* x_json = cJSON_GetObjectItem(position_x_item_json, "x");
                if (nullptr != x_json) {
                    position->x = x_json->valuedouble / sticker_face->width;
                }
                cJSON* y_json = cJSON_GetObjectItem(position_x_item_json, "y");
                if (nullptr != y_json) {
                    position->y = y_json->valuedouble / sticker_face->height;
                }
                cJSON* index_json = cJSON_GetObjectItem(position_x_item_json, "index");
                if (nullptr != index_json) {
                    position->index = index_json->valueint;
                }
                sticker_face->positions.push_back(position);
            }
        }
        cJSON* scale_json = cJSON_GetObjectItem(parts_sub_item_json, "scale");
        if (nullptr != scale_json) {
            cJSON* scale_x_json = cJSON_GetObjectItem(scale_json, "scaleX");
            if (nullptr != scale_x_json) {
                int scale_x_size = cJSON_GetArraySize(scale_x_json);
                for (int i = 0; i < scale_x_size; i++) {
                    cJSON* scale_x_key_json = cJSON_GetArrayItem(scale_x_json, i);
                    cJSON* scale_x_item_json = cJSON_GetObjectItem(scale_x_json, scale_x_key_json->string);
                    int point_size = cJSON_GetArraySize(scale_x_item_json);
                    for (int point_index = 0; point_index < point_size; point_index++) {
                        cJSON* point_json = cJSON_GetArrayItem(scale_x_item_json, point_index);
                        StickerFace::Scale* scale = new StickerFace::Scale();
                        cJSON* x_json = cJSON_GetObjectItem(point_json, "x");
                        if (nullptr != x_json) {
                            scale->x = x_json->valuedouble / sticker_face->width;
                        }
                        cJSON* y_json = cJSON_GetObjectItem(point_json, "y");
                        if (nullptr != y_json) {
                            scale->y = y_json->valuedouble / sticker_face->height;
                        }
                        cJSON* index_json = cJSON_GetObjectItem(point_json, "index");
                        if (nullptr != index_json) {
                            scale->index = index_json->valueint;
                        }
                        sticker_face->scales.push_back(scale); 
                    }
                }
            }
        }
        cJSON* rotate_center_json = cJSON_GetObjectItem(parts_sub_item_json, "rotateCenter");
        if (nullptr != rotate_center_json) {
            int size = cJSON_GetArraySize(rotate_center_json);
            for (int i = 0; i < size; i++) {
                StickerFace::Rotate* rotate = new StickerFace::Rotate();
                cJSON* rotate_item_json = cJSON_GetArrayItem(rotate_center_json, i);
                cJSON* x_json = cJSON_GetObjectItem(rotate_item_json, "x");
                if (nullptr != x_json) {
                    rotate->x = x_json->valuedouble / sticker_face->width;
                }
                cJSON* y_json = cJSON_GetObjectItem(rotate_item_json, "y");
                if (nullptr != y_json) {
                    rotate->y = y_json->valuedouble / sticker_face->height;
                }
                sticker_face->rotates.push_back(rotate);
            }
        }
        for (int i = 0; i < sticker_face->frame_count; i++) {
            std::string sticker_path;
            sticker_path.append(content_root_path);
            sticker_path.append(parts_item_path);
            sticker_path.append("/");
            sticker_path.append(parts_item_path);
            sticker_path.append("_");
            if (i < 10) {
                sticker_path.append("00");
            } else if (i < 100) {
                sticker_path.append("0");
            }
            sticker_path.append(std::to_string(i));
            sticker_path.append(".png");
            sticker_face->sticker_paths.push_back(CopyValue(const_cast<char*>(sticker_path.c_str())));
            sticker_face->sticker_idxs.push_back(i);
        }
        sub_effects.push_back(sticker_face);
    }
    return 0;
}

// convertFilterSubEffect
void Effect::ConvertFilter(cJSON* effect_item_json, char *resource_root_path, FilterSubEffect* filter_sub_effect) {
    cJSON* path_json = cJSON_GetObjectItem(effect_item_json, "path");
    if (nullptr == path_json) {
        return;
    }
    char* path = path_json->valuestring;
    char const* content_name = "config.json";
    std::string content_root_path;
    content_root_path.append(resource_root_path);
    content_root_path.append("/");
    content_root_path.append(path);

    std::string config_path;
    config_path.append(content_root_path);
    config_path.append(content_name);
    char* content_buffer = nullptr;
    LOGE("%s content_path: %s", __func__, config_path.c_str());
    int ret = ReadFile(config_path, &content_buffer);
    if (ret != 0 || nullptr == content_buffer) {
        return;
    }
    cJSON* content_root_json = cJSON_Parse(content_buffer);
    if (nullptr == content_root_json) {
        return;
    }
    cJSON* content_json = cJSON_GetObjectItem(content_root_json, "content");
    if (nullptr == content_json) {
        return;
    }
    cJSON* filter_json = cJSON_GetObjectItem(content_json, "filter");
    if (nullptr == filter_json) {
        return;
    }
    cJSON* file_type_json = cJSON_GetObjectItem(filter_json, "fileType");
    cJSON* intensity_json = cJSON_GetObjectItem(filter_json, "intensity");
    cJSON* shader_type_json = cJSON_GetObjectItem(filter_json, "shaderType");
    cJSON* type_json = cJSON_GetObjectItem(filter_json, "type");
    if (nullptr != file_type_json) {
        filter_sub_effect->file_type = file_type_json->valueint;
    }
    if (nullptr != intensity_json) {
        filter_sub_effect->intensity = static_cast<float>(intensity_json->valuedouble);
    }
    if (nullptr != shader_type_json) {
        filter_sub_effect->shader_type = shader_type_json->valueint;
    }
    if (nullptr != type_json) {
        filter_sub_effect->type = type_json->valueint;
    }
    std::string filter_root_path;
    filter_root_path.append(resource_root_path);
    filter_root_path.append("/");
    filter_root_path.append(path);
    filter_root_path.append("filter/filter.png");
    int filter_texture_width = 0;
    int filter_texture_height = 0;
    int channels = 0;
    unsigned char* filter_texture_buffer = stbi_load(filter_root_path.c_str(), &filter_texture_width,
                                                     &filter_texture_height, &channels, STBI_rgb_alpha);
    if (filter_texture_width > 0 && filter_texture_height > 0) {
        auto* filter = new Filter(filter_texture_buffer, filter_texture_width, filter_texture_height);
        filter->SetIntensity(filter_sub_effect->intensity);
        stbi_image_free(filter_texture_buffer);
        filter_sub_effect->filter = filter;
    }                                                    
}

glm::mat4 Effect::VertexMatrix(SubEffect* sub_effect, int source_width, int source_height) {
    return glm::mat4();
}

void Effect::ParseTextureFiles(cJSON *texture_files, StickerSubEffect* sub_effect,
        const std::string& resource_root_path) {
    int texture_file_size = cJSON_GetArraySize(texture_files);
    for (int i = 0; i < texture_file_size; i++) {
        cJSON* texture_file_item = cJSON_GetArrayItem(texture_files, i);
        if (nullptr != texture_file_item && texture_file_item->type == cJSON_String) {
            char* texture_file = texture_file_item->valuestring;
//            auto* texture_path = new char[strlen(resource_root_path) + strlen(texture_file)];
//            sprintf(texture_path, "%s/%s", resource_root_path, texture_file);
            std::string sticker_path;
            sticker_path.append(resource_root_path);
            sticker_path.append("/");
            sticker_path.append(texture_file);
            sub_effect->sticker_paths.push_back(CopyValue(const_cast<char*>(sticker_path.c_str())));
        }
    }
}

std::string& Effect::ReplaceAllDistince(std::string &str, const std::string &old_value,
        const std::string &new_value) {
    for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()) {
        if ((pos = str.find(old_value, pos)) != std::string::npos) {
            str.replace(pos, old_value.length(), new_value);
        } else {
            break;
        }
    }
    return str;
}

void Effect::ParsePartsItem(cJSON* clip_root_json, const std::string& resource_root_path, const std::string& type, bool face_detect, int zorder) {
    cJSON* parts_json = cJSON_GetObjectItem(clip_root_json, "parts");
    if (nullptr == parts_json) {
        return;
    }
    int parts_size = cJSON_GetArraySize(parts_json);
    if (parts_size <= 0) {
        return;
    }
    std::vector<std::string> used_keys;
    for (int i = 0; i < parts_size; i++) {
        cJSON* parts_item_json = cJSON_GetArrayItem(parts_json, i);
        const char* parts_item_value = parts_item_json->string;
        std::string key_value(parts_item_value);
        std::string value_names[3] = { "_widthAlign", "_heightAlign", "_Vertical" };
        std::string key = key_value;
        for (int value_index = 0; value_index < 3; value_index++) {
            std::string value = value_names[value_index];
            auto find = key_value.find(value);
            if (find != std::string::npos) {
                key = ReplaceAllDistince(key_value, value, "");
            }
        }
        if (std::find(used_keys.begin(), used_keys.end(), key) != used_keys.end()) {
            continue;
        }
        used_keys.push_back(key_value);
        cJSON* parts_value_json = cJSON_GetObjectItem(parts_json, parts_item_value);
        if (nullptr == parts_value_json) {
            return;
        }
        auto* stickerv3 = new StickerV3SubEffect();
        stickerv3->name = CopyValue(const_cast<char*>(key.c_str()));
        stickerv3->face_detect = face_detect;
        stickerv3->type = CopyValue(const_cast<char*>(type.c_str()));
        cJSON* alpha_factor_json = cJSON_GetObjectItem(parts_value_json, "alphaFactor");
        if (nullptr != alpha_factor_json) {
            double alpha_factor = alpha_factor_json->valuedouble;
            stickerv3->alpha_factor = static_cast<float>(alpha_factor);
        }
        cJSON* blend_mode_json = cJSON_GetObjectItem(parts_value_json, "blendmode");
        if (nullptr != blend_mode_json) {
            int blend_mode = blend_mode_json->valueint;
            stickerv3->blendmode = blend_mode;
        }
        stickerv3->blend = BlendFactory::CreateBlend(stickerv3->blendmode);
        cJSON* fps_json = cJSON_GetObjectItem(parts_value_json, "fps");
        if (nullptr != fps_json) {
            int fps = fps_json->valueint;
            stickerv3->fps = fps;
        }
        cJSON* width_json = cJSON_GetObjectItem(parts_value_json, "width");
        if (nullptr != width_json) {
            int width = width_json->valueint;
            stickerv3->width = width;
        }
        cJSON* height_json = cJSON_GetObjectItem(parts_value_json, "height");
        if (nullptr != height_json) {
            int height = height_json->valueint;
            stickerv3->height = height;
        }
        stickerv3->zorder = zorder;
//        cJSON* zorder_json = cJSON_GetObjectItem(parts_value_json, "zorder");
//        if (nullptr != zorder_json) {
//            int zorder = zorder_json->valueint;
//            stickerv3->zorder = zorder;
//        }
        cJSON* transform_type_json = cJSON_GetObjectItem(parts_value_json, "transformType");
        if (nullptr != transform_type_json) {
            char* transform_type = transform_type_json->valuestring;
//            stickerv3->transform_type = CopyValue(transform_type);
        }
        cJSON* texture_idx_json = cJSON_GetObjectItem(parts_value_json, "textureIdx");
        if (nullptr != texture_idx_json) {
            cJSON* idx_json = cJSON_GetObjectItem(texture_idx_json, "idx");
            if (nullptr != idx_json) {
                int idx_size = cJSON_GetArraySize(idx_json);
                for (int idx_index = 0; idx_index < idx_size; idx_index++) {
                    cJSON* idx_item_json = cJSON_GetArrayItem(idx_json, idx_index);
                    int idx = idx_item_json->valueint;
                    stickerv3->sticker_idxs.push_back(idx);
                }
            }
//            cJSON* type_json = cJSON_GetObjectItem(texture_idx_json, "type");
//            if (nullptr != type_json) {
//                texture_idx->type = CopyValue(type_json->valuestring);
//            }
        }
        cJSON* texture_files_json = cJSON_GetObjectItem(clip_root_json, "texturefiles");
        if (nullptr != texture_files_json) {
            // get sticker file path list
            ParseTextureFiles(texture_files_json, stickerv3, resource_root_path);
        }
        cJSON* transform_params_json = cJSON_GetObjectItem(parts_value_json, "transformParams");
        if (nullptr != transform_params_json) {
            auto* transform = new Transform();
            cJSON* position_json = cJSON_GetObjectItem(transform_params_json, "position");
            if (nullptr != position_json) {
                int position_size = cJSON_GetArraySize(position_json);
                for (int position_index = 0; position_index < position_size; position_index++) {
                    auto* position = new Position();
                    cJSON* position_item_json = cJSON_GetArrayItem(position_json, position_index);
                    char* position_value = position_item_json->string;
                    cJSON* position_value_json = cJSON_GetObjectItem(position_json, position_value);
                    if (nullptr != position_value_json) {
                        cJSON* anchor_json = cJSON_GetObjectItem(position_value_json, "anchor");
                        if (nullptr != anchor_json) {
                            int anchor_size = cJSON_GetArraySize(anchor_json);
                            for (int anchor_index = 0; anchor_index < anchor_size; anchor_index++) {
                                cJSON* anchor_item_json = cJSON_GetArrayItem(anchor_json, anchor_index);
                                if (nullptr != anchor_item_json) {
                                    position->anchor.push_back(static_cast<float>(anchor_item_json->valuedouble));
                                }
                            }
                        }
                        cJSON* point_json = cJSON_GetObjectItem(position_value_json, "point");
                        if (nullptr != point_json) {
                            int point_size = cJSON_GetArraySize(point_json);
                            for (int point_index = 0; point_index < point_size; point_index++) {
                                cJSON* point_item_json = cJSON_GetArrayItem(point_json, point_index);
                                auto* point = new Point();
                                cJSON* idx_json = cJSON_GetObjectItem(point_item_json, "idx");
                                if (nullptr != idx_json && idx_json->type == cJSON_String) {
                                    point->idx = strtol(idx_json->valuestring, nullptr, 10);
                                }
                                cJSON* relation_ref_json = cJSON_GetObjectItem(point_item_json, "relationRef");
                                if (nullptr != relation_ref_json && relation_ref_json->type == cJSON_Number) {
                                    point->relation_ref = relation_ref_json->valueint;
                                }
                                cJSON* relation_type_json = cJSON_GetObjectItem(point_item_json, "relationType");
                                if (nullptr != relation_type_json && relation_type_json->type == cJSON_String) {
                                    point->relation_type = CopyValue(relation_type_json->valuestring);
                                }
                                cJSON* weight_json = cJSON_GetObjectItem(point_item_json, "weight");
                                if (nullptr != weight_json) {
                                    point->weight = static_cast<float>(weight_json->valuedouble);
                                }
                                position->points.push_back(point);
                            }
                        }
                    }
                    transform->positions.push_back(position);
                }
            }
            cJSON* relation_json = cJSON_GetObjectItem(transform_params_json, "relation");
            if (nullptr != relation_json) {
                cJSON* foreground_json = cJSON_GetObjectItem(relation_json, "foreground");
                if (nullptr != foreground_json && foreground_json->type == cJSON_Number) {
                    auto* relation = new Relation();
                    relation->forground = foreground_json->valueint;
                    transform->relation = relation;
                }
                cJSON* face106_json = cJSON_GetObjectItem(relation_json, "face106");
                if (nullptr != face106_json) {
                    transform->face_detect = face106_json->valueint == 1;
                }
            }
            cJSON* relation_index_json = cJSON_GetObjectItem(transform_params_json, "relationIndex");
            if (nullptr != relation_index_json) {
                int relation_size = cJSON_GetArraySize(relation_index_json);
                for (int relation_index = 0; relation_index < relation_size; relation_index++) {
                    cJSON* relation_item_json = cJSON_GetArrayItem(relation_index_json, relation_index);
                    if (nullptr != relation_item_json && relation_item_json->type == cJSON_Number) {
                        transform->relation_index.push_back(relation_item_json->valueint);
                    }
                }
            }
            cJSON* relation_ref_order_json = cJSON_GetObjectItem(transform_params_json, "relationRefOrder");
            if (nullptr != relation_ref_order_json && relation_ref_order_json->type == cJSON_Number) {
                transform->relation_ref_order = relation_ref_order_json->valueint;
            }
            cJSON* rotation_type_json = cJSON_GetObjectItem(transform_params_json, "rotationtype");
            if (nullptr != rotation_type_json && rotation_type_json->type == cJSON_Number) {
                transform->rotation_type = rotation_type_json->valueint;
            }
            cJSON* scale_json = cJSON_GetObjectItem(transform_params_json, "scale");
            if (nullptr != scale_json) {
                auto* scale = new Scale();
                cJSON* scale_y_json = cJSON_GetObjectItem(scale_json, "scaleY");
                if (nullptr != scale_y_json) {
                    auto* scale_params = new ScaleParams();
                    cJSON* factor_json = cJSON_GetObjectItem(scale_y_json, "factor");
                    if (nullptr != factor_json) {
                        scale_params->factor = factor_json->valuedouble;
                    }
                    scale->scale_y = scale_params;
                }
                cJSON* scale_x_json = cJSON_GetObjectItem(scale_json, "scaleX");
                if (nullptr != scale_x_json) {
                    auto* scale_params = new ScaleParams();
                    cJSON* factor_json = cJSON_GetObjectItem(scale_x_json, "factor");
                    if (nullptr != factor_json) {
                        scale_params->factor = factor_json->valuedouble;
                    }
                    scale->scale_x = scale_params;
                }
                transform->scale = scale;
            }
            stickerv3->transform = transform;
            sub_effects_.push_back(stickerv3);
        }
        printf("");
    }
}

void Effect::Parse2DStickerV3(const std::string& resource_root_path, int zorder) {
    char const* content_name = "content.json";
    std::string content_path;
    content_path.append(resource_root_path);
    content_path.append("/");
    content_path.append(content_name);
    char* content_buffer = nullptr;
    int ret = ReadFile(content_path, &content_buffer);
    if (ret != 0 || nullptr == content_buffer) {
        return;
    }
    cJSON* content_root_json = cJSON_Parse(content_buffer);
    if (nullptr == content_root_json) {
        return;
    }
    cJSON* content_json = cJSON_GetObjectItem(content_root_json, "content");
    if (nullptr == content_json) {
        return;
    }
    cJSON* path_json = cJSON_GetObjectItem(content_json, "path");
    if (nullptr == path_json) {
        return;
    }
    bool face_detect = false;
    cJSON* requirement_json = cJSON_GetObjectItem(content_root_json, "requirement");
    if (nullptr != requirement_json) {
        cJSON* face_detect_json = cJSON_GetObjectItem(requirement_json, "faceDetect");
        if (nullptr != face_detect_json) {
            face_detect = face_detect_json->valueint;
        }
    }
    std::string type_string;
    cJSON* type_json = cJSON_GetObjectItem(content_json, "type");
    if (nullptr != type_json) {
        type_string.append(type_json->valuestring);
    }
    char* path_value = path_json->valuestring;
    std::string clip_path;
    clip_path.append(resource_root_path);
    clip_path.append("/");
    clip_path.append(path_value);
    char* clip_buffer = nullptr;
    ret = ReadFile(clip_path, &clip_buffer);
    if (ret != 0 || nullptr == clip_buffer) {
        return;
    }
    cJSON* clip_root_json = cJSON_Parse(clip_buffer);
    if (nullptr == clip_root_json) {
        return;
    }
    // convert sticker config
    ParsePartsItem(clip_root_json, resource_root_path, type_string, face_detect, zorder);
    printf("");
}

void Effect::ParseUniform(SubEffect *sub_effect, char *config_path, cJSON *uniforms_json, ShaderUniformType type) {
    int fragment_uniforms_size = cJSON_GetArraySize(uniforms_json);
    for (int uniform_index = 0; uniform_index < fragment_uniforms_size; uniform_index++) {
        cJSON* fragment_uniform_item_json = cJSON_GetArrayItem(uniforms_json, uniform_index);
        auto* fragment_uniform = new ShaderUniforms();
        cJSON* fragment_uniform_type_json = cJSON_GetObjectItem(fragment_uniform_item_json, "type");
        if (nullptr != fragment_uniform_type_json) {
            fragment_uniform->type = fragment_uniform_type_json->valueint;
        }
        cJSON* name_json = cJSON_GetObjectItem(fragment_uniform_item_json, "name");
        if (nullptr != name_json) {
            fragment_uniform->name = name_json->valuestring;
        }
        cJSON* input_effect_index_json = cJSON_GetObjectItem(fragment_uniform_item_json, "inputEffectIndex");
        if (nullptr != input_effect_index_json && input_effect_index_json->type == cJSON_Number) {
            fragment_uniform->input_effect_index = input_effect_index_json->valueint;
        }
        cJSON* input_effect_json = cJSON_GetObjectItem(fragment_uniform_item_json, "inputEffect");
        if (nullptr != input_effect_json) {
            int input_effect_size = cJSON_GetArraySize(input_effect_json);
            for (int input_effect_index = 0; input_effect_index < input_effect_size; input_effect_index++) {
                cJSON* input_effect_item_json = cJSON_GetArrayItem(input_effect_json, input_effect_index);
                if (nullptr != input_effect_item_json && input_effect_item_json->type == cJSON_String) {
                    char* value = input_effect_item_json->valuestring;
                    size_t len = strlen(value);
                    char* input_effect_value = new char[len + 1];
                    memcpy(input_effect_value, value, len);
                    input_effect_value[len] = '\0';
                    sub_effect->input_effect.push_back(input_effect_value);
                }
            }
        }
        cJSON* fragment_uniform_data_json = cJSON_GetObjectItem(fragment_uniform_item_json, "data");
        if (nullptr != fragment_uniform_data_json) {
            int fragment_uniform_data_size = cJSON_GetArraySize(fragment_uniform_data_json);
            fragment_uniform->data_size = fragment_uniform_data_size;
            switch (fragment_uniform->type) {
                case UniformTypeInputTexture:
                    break;

                case UniformTypeMattingTexture:
                case UniformTypeInputTextureLast:
                    break;

                case UniformTypeInputEffectIndex:
                    break;

                case UniformTypeRenderCacheKey:
                    break;

                case UniformTypeSample2D: {
                    // TODO 这里一次创建多个图片texture会不会卡顿?
                    for (int data_index = 0; data_index < fragment_uniform_data_size; data_index++) {
                        cJSON* data_item_json = cJSON_GetArrayItem(fragment_uniform_data_json, data_index);
                        char* relative_path = data_item_json->valuestring;
                        auto* sample_texture_path = new char[strlen(config_path) + strlen(relative_path)];
                        sprintf(sample_texture_path, "%s/%s", config_path, relative_path);
                        int sample_texture_width = 0;
                        int sample_texture_height = 0;
                        int channels = 0;
                        unsigned char* sample_texture_buffer = stbi_load(sample_texture_path, &sample_texture_width,
                                &sample_texture_height, &channels, STBI_rgb_alpha);
                        delete[] sample_texture_path;
                        if (nullptr != sample_texture_buffer && sample_texture_width > 0 && sample_texture_height > 0) {
                            auto* image_buffer = new ImageBuffer(sample_texture_width, sample_texture_height,
                                    sample_texture_buffer);
                            stbi_image_free(sample_texture_buffer);
                            fragment_uniform->image_buffer_values.push_back(image_buffer);
                        }
                    }
                    break;
                }

                case UniformTypeBoolean:
                    break;

                case UniformTypeFloat: {
                    for (int data_index = 0; data_index < fragment_uniform_data_size; data_index++) {
                        cJSON *data_item_json = cJSON_GetArrayItem(fragment_uniform_data_json, data_index);
                        fragment_uniform->float_values.push_back(static_cast<float>(data_item_json->valuedouble));
                    }
                    break;
                }

                case UniformTypePoint:
                    break;

                case UniformTypeVec3:
                    break;

                case UniformTypeMatrix4x4:
                    break;
                    
                case UniformTypeFace:
                    break;

                case UniformTypeImageWidth:
                    break;

                case UniformTypeImageHeight:
                    break;

                case UniformTypeTexelWidthOffset:
                    break;

                case UniformTypeTexelHeightOffset:
                    break;

                case UniformTypeFrameTime:
                    break;
                default:
                    break;
            }
        }
        if (type == Fragment) {
            sub_effect->fragment_uniforms.push_back(fragment_uniform);
        } else if (type == Vertex) {
            sub_effect->vertex_uniforms.push_back(fragment_uniform);
        }
    }
}

}  // namespace trinity
