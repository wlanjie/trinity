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

#ifndef TRINITY_EFFECT_H
#define TRINITY_EFFECT_H

#include <string>
#include <vector>
#include <list>
#include <map>
#include "image_buffer.h"
#include "process_buffer.h"
#include "frame_buffer.h"
#include "stb_image.h"
#include "blend.h"
#include "face_markup_render.h"
#include "face_detection.h"

// glm
#include "gtx/norm.hpp"
#include "ext.hpp"
#include "geometric.hpp"
#include "trigonometric.hpp"
#include "ext/vector_float2.hpp"
#include "common.hpp"
#include "vec2.hpp"

extern "C" {
#include "cJSON.h"
};

namespace trinity {

enum ShaderUniformType {
    Fragment = 1,
    Vertex
};

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

class ShaderUniforms {
 public:
    char* name;
    int type;
    int data_size;
    std::vector<float> float_values;
    std::vector<ImageBuffer*> image_buffer_values;
    int data_index;
    int texture_index;
    int texture_unit_index;
    int input_effect_index;
};

class Point {
 public:
    int idx;
    int relation_ref;
    char* relation_type;
    float weight;
};

class Position {
 public:
    std::vector<float> anchor;
    std::vector<Point*> points;
};

class Relation {
 public:
    int forground;
};

class ScaleParams {
 public:
    ScaleParams()
        : start_idx(nullptr)
        , start_relation_ref(0)
        , start_relation_type(nullptr)
        , end_idx(nullptr)
        , end_relation_ref(0)
        , end_relation_type(nullptr)
        , factor(0) {}

    ~ScaleParams() {
        if (nullptr != start_idx) {
            delete[] start_idx;
            start_idx = nullptr;
        }
        if (nullptr != start_relation_type) {
            delete[] start_relation_type;
            start_relation_type = nullptr;
        }
        if (nullptr != end_idx) {
            delete[] end_idx;
            end_idx = nullptr;
        }
        if (nullptr != end_relation_type) {
            delete[] end_relation_type;
            end_relation_type = nullptr;
        }
    }
    char* start_idx;
    int start_relation_ref;
    char* start_relation_type;
    char* end_idx;
    int end_relation_ref;
    char* end_relation_type;
    double factor;
};

class Scale {
 public:
    Scale() : scale_x(nullptr), scale_y(nullptr) {}
    ~Scale() {
        if (nullptr != scale_x) {
            delete scale_x;
            scale_x = nullptr;
        }
        if (nullptr != scale_y) {
            delete scale_y;
            scale_y = nullptr;
        }
    }
    ScaleParams* scale_x;
    ScaleParams* scale_y;
};

class Transform {
 public:
    Transform()
        : relation(nullptr)
        , scale(nullptr)
        , relation_ref_order(0)
        , rotation_type(0)
        , face_detect(false) {}

    ~Transform() {

    }
    std::vector<Position*> positions;
    Relation* relation;
    std::vector<int> relation_index;
    int relation_ref_order;
    int rotation_type;
    Scale* scale;
    bool face_detect;
 public:
    glm::vec2 Center(float aspect, FaceDetectionReport* face_detection);
    glm::vec2 ScaleSize(float aspect, FaceDetectionReport* face_detection);
};

class FaceMakeupV2Filter {
 public:
    int image_count;
    double image_interval;
    int blend_mode;
    char* filter_type;
    double intensity;
    double x;
    double y;
    double width;
    double height;
    ImageBuffer* image_buffer;
    int z_position;
};

class FaceMakeupV2 {
 public:
    std::vector<int> face_ids;
    std::vector<FaceMakeupV2Filter*> filters;
    int standard_face_width;
    int standard_face_height;
};

class TextureIdx {
 public:
    char* type;
    std::vector<int> idx;
};

// SubEffect
class SubEffect {
 public:
    SubEffect();
    virtual ~SubEffect();

 public:
    char* type;
    char* name;
    int zorder;
    bool enable;
    std::vector<ShaderUniforms*> fragment_uniforms;
    std::vector<ShaderUniforms*> vertex_uniforms;
    std::vector<char*> input_effect;
    char* param_name;
    float param_value;

 public:
    ProcessBuffer* GetProcessBuffer() {
        return process_buffer_;
    }
    void InitProcessBuffer(char* vertex_shader, char* fragment_shader);
    void SetFloat(ShaderUniforms* uniform, ProcessBuffer* process_buffer);
    void SetSample2D(ShaderUniforms* uniform, ProcessBuffer* process_buffer);
    void SetTextureUnit(ShaderUniforms* uniform, ProcessBuffer* process_buffer, GLuint texture);
    void SetUniform(std::list<SubEffect*> sub_effects, ProcessBuffer* process_buffer,
            std::vector<ShaderUniforms*> uniforms, int origin_texture_id, int texture_id, uint64_t current_time);
    virtual int OnDrawFrame(FaceDetection* face_detection, std::list<SubEffect*> sub_effects, int origin_texture_id, int texture_id, uint64_t current_time) {
        return texture_id;
    }

 private:
    ProcessBuffer* process_buffer_;
};

// GeneralSubEffect
class GeneralSubEffect : public SubEffect {
 public:
    GeneralSubEffect();
    ~GeneralSubEffect();
    virtual int OnDrawFrame(FaceDetection* face_detection, std::list<SubEffect*> sub_effects, int origin_texture_id, int texture_id, uint64_t current_time);
};

// Sticker
class StickerSubEffect : public SubEffect {
 public:
    StickerSubEffect();
    ~StickerSubEffect();

    int OnDrawFrame(FaceDetection* face_detection,
            std::list<SubEffect*> sub_effects,
            int origin_texture_id, int texture_id, uint64_t current_time) override;
    ImageBuffer* StickerBufferAtFrameTime(float time);
 public:
    int blendmode;
    int width;
    int height;
    int fps;
    float alpha_factor;
    std::vector<int> sticker_idxs;
    std::vector<char*> sticker_paths;
    std::map<int, ImageBuffer*> image_buffers;
    int pic_index;
    bool face_detect;
    bool has_face;
    float input_aspect;
    Blend* blend;
    int begin_frame_time;
 protected:
    virtual glm::mat4 VertexMatrix(FaceDetectionReport* face_detection);
};

// StickerV3
class StickerV3SubEffect : public StickerSubEffect {
 public:
    StickerV3SubEffect();
    ~StickerV3SubEffect();

    int OnDrawFrame(FaceDetection* face_detection, std::list<SubEffect*> sub_effects, int origin_texture_id, int texture_id, uint64_t current_time) override;
 public:
    Transform* transform;

 protected:
    glm::mat4 VertexMatrix(FaceDetectionReport* face_detection);
};

// faceMakeupV2
class FaceMakeupV2SubEffect : public StickerSubEffect {
 public:
    FaceMakeupV2SubEffect();
    ~FaceMakeupV2SubEffect();
    int OnDrawFrame(FaceDetection* face_detection, std::list<SubEffect*> sub_effects, int origin_texture_id, int texture_id, uint64_t current_time) override;
 protected:
    glm::mat4 VertexMatrix();
 public:
    FaceMakeupV2* face_makeup_v2_;
    FaceMarkupRender* face_markup_render_;
    NormalBlend* blend;
};

class Effect {
 public:
    Effect();
    ~Effect();
    void SetFaceDetection(FaceDetection* face_detection);
    void ParseConfig(char* config_path);
    int OnDrawFrame(GLuint texture_id, uint64_t current_time);
    void UpdateTime(int start_time, int end_time);
    void UpdateParam(const char* effect_name, const char* param_name, float value);
 private:
    char* CopyValue(char* src);
    int ReadFile(const std::string& path, char** buffer);
    void ConvertStickerConfig(cJSON* effect_item_json, char* resource_root_path, SubEffect* sub_effect);
    void ConvertGeneralConfig(cJSON* effect_item_json, char* resource_root_path, GeneralSubEffect* general_sub_effect);
    void ParseFaceMakeupV2(cJSON* makeup_root_json, const std::string& resource_root_path, FaceMakeupV2* face_makeup_v2);
    void ConvertFaceMakeupV2(cJSON* effect_item_json, char* resource_root_path, FaceMakeupV2SubEffect* face_makeup_v2_sub_effect);
    glm::mat4 VertexMatrix(SubEffect* sub_effect);
    void ParseTextureFiles(cJSON* texture_files, StickerSubEffect* sub_effect, const std::string& resource_root_path);
    std::string& ReplaceAllDistince(std::string& str, const std::string& old_value, const std::string& new_value);
    void ParsePartsItem(cJSON* clip_root_json, const std::string& resource_root_path, const std::string& type);
    void Parse2DStickerV3(SubEffect* sub_effect, const std::string& resource_root_path);
    void ParseUniform(SubEffect *sub_effect, char *config_path, cJSON *uniforms_json, ShaderUniformType type);
 private:
    FaceDetection* face_detection_;
    std::list<SubEffect*> sub_effects_;
    int start_time_;
    int end_time_;
};

}  // namespace trinity

#endif  // TRINITY_EFFECT_H
