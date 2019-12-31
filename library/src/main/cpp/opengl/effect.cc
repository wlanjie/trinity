//
// Created by wlanjie on 2019-12-20.
//

#include "effect.h"

namespace trinity {

GeneralSubEffect::GeneralSubEffect() {

}

GeneralSubEffect::~GeneralSubEffect() {
    printf("~GeneralSubEffect\n");
}

int GeneralSubEffect::OnDrawFrame(std::list<SubEffect*> sub_effects, int texture_id, uint64_t current_time) {
    ProcessBuffer* process_buffer = GetProcessBuffer();
    if (nullptr == process_buffer) {
        return texture_id;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, process_buffer->GetFrameBufferId());
    process_buffer->SetOutput(720, 1280);
    process_buffer->ActiveProgram();
    process_buffer->Clear();
    process_buffer->ActiveAttribute();
    SetUniform(sub_effects, process_buffer, fragment_uniforms, texture_id, current_time);
    SetUniform(sub_effects, process_buffer, vertex_uniforms, texture_id, current_time);
    process_buffer->DrawArrays();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return process_buffer->GetTextureId();
}

void Transform::Center(float aspect) {
    
}

void Transform::ScaleSize(float aspect) {
    
}

// StickerSubEffect
StickerSubEffect::StickerSubEffect() 
    : blendmode(-1)
    , width(0)
    , height(0)
    , fps(0)
    , alpha_factor(1.0F)
    , zorder(0)
    , pic_index(0)
    , face_detect(false)
    , has_face(false)
    , input_aspect(1.0F)
    , blend(nullptr)
    , begin_frame_time(0) {

}

StickerSubEffect::~StickerSubEffect() {
    printf("~StickerSubEffect\n");
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

int StickerSubEffect::OnDrawFrame(std::list<SubEffect *> sub_effects, int texture_id, uint64_t current_time) {
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
        index = (int) (duration * fps) % count;
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
    unsigned char* sample_texture_buffer = stbi_load(image_path, &sample_texture_width, &sample_texture_height, &channels, STBI_rgb_alpha);
    if (nullptr != sample_texture_buffer && sample_texture_width > 0 && sample_texture_height > 0) {
        auto* image_buffer = new ImageBuffer(sample_texture_width, sample_texture_height, sample_texture_buffer);
        stbi_image_free(sample_texture_buffer);
        image_buffers.insert(std::pair<int, ImageBuffer*>(final_index, image_buffer));
        return image_buffer;
    }
    return nullptr;
}

// StickerV3
StickerV3SubEffect::StickerV3SubEffect() : transform(nullptr) {

}

StickerV3SubEffect::~StickerV3SubEffect() {
    printf("~StickerV3SubEffect\n");
    if (nullptr != transform) {
        delete transform;
        transform = nullptr;
    }
}

int StickerV3SubEffect::OnDrawFrame(std::list<SubEffect *> sub_effects, int texture_id, uint64_t current_time) {
    if (nullptr != blend) {
        ImageBuffer* image_buffer = StickerBufferAtFrameTime(current_time);
        if (nullptr != image_buffer) {
            return blend->OnDrawFrame(texture_id, image_buffer->GetTextureId(), nullptr, alpha_factor);
        }
    }
    return texture_id;
}

void StickerV3SubEffect::VertexMatrix(Matrix4x4 **matrix) {
    if (width == 0 || height == 0) {
        return;
    }
    float sticker_aspect = width * 1.0F / height;
    float input_aspect = input_aspect;
    if (nullptr == transform) {
        return;
    }
}

// SubEffect
SubEffect::SubEffect() : process_buffer_(nullptr) {

}

SubEffect::~SubEffect() {
    if (nullptr != type) {
        delete[] type;
        type = nullptr;
    }
    if (nullptr != name) {
        delete[] name;
        name = nullptr;
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
    for (auto& input_effect : input_effect) {
        delete[] input_effect;
    }
    input_effect.clear();
    if (nullptr != process_buffer_) {
        delete process_buffer_;
        process_buffer_ = nullptr;
    }
    printf("~SubEffect\n");
}

void SubEffect::InitProcessBuffer(char *vertex_shader, char *fragment_shader) {
    process_buffer_ = new ProcessBuffer();
    process_buffer_->Init(vertex_shader, fragment_shader);
}

void SubEffect::SetFloat(ShaderUniforms *uniform, ProcessBuffer *process_buffer) {
    if (nullptr == process_buffer) {
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

void SubEffect::SetUniform(std::list<SubEffect*> sub_effects, ProcessBuffer *process_buffer, std::vector<ShaderUniforms*> uniforms, int texture_id, uint64_t current_time) {
    int texture_unit_index = 0;
    for (auto& fragment_uniform : uniforms) {
        int type = fragment_uniform->type;
        bool need_texture_unit = type == UniformTypeSample2D || type == UniformTypeInputTexture || type == UniformTypeInputTextureLast || type == UniformTypeRenderCacheKey || type == UniformTypeMattingTexture || type == UniformTypeInputEffectIndex;
        if (need_texture_unit) {
            fragment_uniform->texture_unit_index = texture_unit_index;
            texture_unit_index += 1;
        }
        switch (fragment_uniform->type) {
            case UniformTypeInputTexture:
                SetTextureUnit(fragment_uniform, process_buffer, texture_id);
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
                process_buffer->SetFloat(fragment_uniform->name, 1.0F / 720);
                break;
            case UniformTypeTexelHeightOffset:
                printf("UniformTypeTexelHeightOffset name; %s\n", fragment_uniform->name);
                process_buffer->SetFloat(fragment_uniform->name, 1.0F / 1280);
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
    , end_time_(INT32_MAX) {

}

Effect::~Effect() {
    for (auto& sub_effect : sub_effects_) {
        delete sub_effect;
    }
    sub_effects_.clear();
}

int Effect::OnDrawFrame(GLuint texture_id, uint64_t current_time) {
    int texture = texture_id;
    for (auto iterator = sub_effects_.begin(); iterator != sub_effects_.end(); iterator++) {
        SubEffect* sub_effect = *iterator;
//        if (!sub_effect->enable) {
//            continue;
//        }
        texture = sub_effect->OnDrawFrame(sub_effects_, texture_id, current_time);
    }
    return texture;
}

void Effect::Update(int start_time, int end_time) {
    start_time_ = start_time;
    end_time_ = end_time;
}

void Effect::ParseConfig(char *config_path) {
    char const* config_name = "config.json";
    std::string config;
    config.append(config_path);
    config.append("/");
    config.append(config_name);
    
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
        auto* sub_effect = new SubEffect();
        cJSON *effect_item_json = cJSON_GetArrayItem(effect_json, i);
        cJSON* type_json = cJSON_GetObjectItem(effect_item_json, "type");
        char* type = nullptr;
        if (nullptr != type_json) {
            char* value = type_json->valuestring;
            type = CopyValue(value);
        }
        sub_effect->type = type;
        // TODO delete sub_effect
        if (nullptr != type) {
            if (strcmp(type, "2DStickerV3") == 0) {
                cJSON* path_json = cJSON_GetObjectItem(effect_item_json, "path");
                if (nullptr == path_json) {
                    return;
                }
                char* path_value = path_json->valuestring;
                std::string path;
                path.append(config_path);
                path.append("/");
                path.append(path_value);
                Parse2DStickerV3(sub_effect, path);
            } else if (strcmp(type, "generalEffect") == 0) {
                auto* general_sub_effect = new GeneralSubEffect();
                ConvertGeneralConfig(effect_item_json, config_path, general_sub_effect);
                general_sub_effect->type = type;
                sub_effects_.push_back(general_sub_effect);
            }
        }
    }
}

char* Effect::CopyValue(char* src) {
    size_t len = strlen(src);
    char* value = new char[len + 1];
    memset(value, 0, len * sizeof(char));
    sprintf(value, "%s", src);
    value[len] = '\0';   
    return value;
}

int Effect::ReadFile(std::string& path, char **buffer) {
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

void Effect::ConvertStickerConfig(cJSON *effect_item_json, char *resource_root_path, SubEffect *sub_effect) {
    
}

void Effect::ConvertGeneralConfig(cJSON *effect_item_json, char* resource_root_path, GeneralSubEffect* general_sub_effect) {
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

void Effect::VertexMatrix(SubEffect* sub_effect, Matrix4x4 **matrix) {
    
}

void Effect::ParseTextureFiles(cJSON *texture_files, StickerSubEffect* sub_effect, std::string& resource_root_path) {
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

std::string& Effect::ReplaceAllDistince(std::string &str, const std::string &old_value, const std::string &new_value) {
    for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()) {
        if ((pos = str.find(old_value, pos)) != std::string::npos) {
            str.replace(pos, old_value.length(), new_value);
        } else {
            break;
        }
    }
    return str;
}

void Effect::ParsePartsItem(cJSON* clip_root_json, std::string& resource_root_path, std::string& type) {
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
        stickerv3->face_detect = false;
        stickerv3->type = CopyValue(const_cast<char*>(type.c_str()));
        stickerv3->blend = new Blend();
        
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
        cJSON* zorder_json = cJSON_GetObjectItem(parts_value_json, "zorder");
        if (nullptr != zorder_json) {
            int zorder = zorder_json->valueint;
            stickerv3->zorder = zorder;
        }
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
                                    point->idx = CopyValue(idx_json->valuestring);
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
                
            }
            stickerv3->transform = transform;
            sub_effects_.push_back(stickerv3);
        }
        printf("");
    }
}

void Effect::Parse2DStickerV3(SubEffect* sub_effect, std::string& resource_root_path) {
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
    ParsePartsItem(clip_root_json, resource_root_path, type_string);
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
                    //TODO 这里一次创建多个图片texture会不会卡顿?
                    for (int data_index = 0; data_index < fragment_uniform_data_size; data_index++) {
                        cJSON* data_item_json = cJSON_GetArrayItem(fragment_uniform_data_json, data_index);
                        char* relative_path = data_item_json->valuestring;
                        auto* sample_texture_path = new char[strlen(config_path) + strlen(relative_path)];
                        sprintf(sample_texture_path, "%s/%s", config_path, relative_path);
                        int sample_texture_width = 0;
                        int sample_texture_height = 0;
                        int channels = 0;
                        unsigned char* sample_texture_buffer = stbi_load(sample_texture_path, &sample_texture_width, &sample_texture_height, &channels, STBI_rgb_alpha);
                        delete[] sample_texture_path;
                        if (nullptr != sample_texture_buffer && sample_texture_width > 0 && sample_texture_height > 0) {
                            auto* image_buffer = new ImageBuffer(sample_texture_width, sample_texture_height, sample_texture_buffer);
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
  
}
