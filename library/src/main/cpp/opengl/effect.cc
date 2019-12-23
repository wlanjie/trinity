//
// Created by wlanjie on 2019-12-20.
//

#include "effect.h"

namespace trinity {

Effect::Effect()
    : start_time_(0)
    , end_time_(INT32_MAX) {

}

Effect::~Effect() {
    for (auto& sub_effect : sub_effects_) {
        delete[] sub_effect->name;
        for (auto& fragment_uniform : sub_effect->fragment_uniforms) {
            delete[] fragment_uniform->name;
            for (auto& image_buffer : fragment_uniform->image_buffer_values) {
                delete image_buffer;
            }
            fragment_uniform->image_buffer_values.clear();
            fragment_uniform->float_values.clear();
        }
        sub_effect->fragment_uniforms.clear();

        for (auto& input_effect : sub_effect->input_effect) {
            delete[] input_effect;
        }
        sub_effect->input_effect.clear();
    }
    sub_effects_.clear();
    if (!process_buffers_.empty()) {
        auto process_buffer_iterator = process_buffers_.begin();
        while (process_buffer_iterator != process_buffers_.end()) {
//            delete[] process_buffer_iterator->first;
            delete process_buffer_iterator->second;
            process_buffer_iterator++;
        }
    }
    process_buffers_.clear();
}

int Effect::OnDrawFrame(GLuint texture_id, uint64_t current_time) {
    if (process_buffers_.empty()) {
        return texture_id;
    }
    int texture = texture_id;
    for (size_t i = 0; i < sub_effects_.size(); i++) {
        SubEffect* sub_effect = sub_effects_.at(i);
//        if (!sub_effect->enable) {
//            continue;
//        }
        ProcessBuffer* process_buffer = nullptr;
        auto process_buffer_iterator = process_buffers_.find(sub_effect->name);
        if (process_buffer_iterator == process_buffers_.end()) {
            return texture_id;
        } else {
            process_buffer = process_buffer_iterator->second;
        }
        if (nullptr == process_buffer) {
            continue;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, process_buffer->GetFrameBufferId());
        process_buffer->SetOutput(720, 1280);
        process_buffer->ActiveProgram();
        process_buffer->Clear();
        process_buffer->ActiveAttribute();
        SetUniform(sub_effect, process_buffer, sub_effect->fragment_uniforms, texture_id, current_time);
        SetUniform(sub_effect, process_buffer, sub_effect->vertex_uniforms, texture_id, current_time);
        process_buffer->DrawArrays();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        texture = process_buffer->GetTextureId();
        process_buffers_.insert(std::pair<char*, ProcessBuffer*>(sub_effect->name, process_buffer));
    }
    return texture;
}

void Effect::Update(int start_time, int end_time) {
    start_time_ = start_time;
    end_time_ = end_time;
}

void Effect::SetFloat(ShaderUniforms *fragment_uniform, ProcessBuffer *process_buffer) {
    int index = fragment_uniform->data_index % fragment_uniform->float_values.size();
    float value = fragment_uniform->float_values.at(index);
    fragment_uniform->data_index++;
    printf("SetFloat %f\n", value);
    process_buffer->SetFloat(fragment_uniform->name, value);
}

void Effect::SetSample2D(ShaderUniforms *fragment_uniform, ProcessBuffer *process_buffer) {
    int index = fragment_uniform->data_index % fragment_uniform->image_buffer_values.size();
    ImageBuffer* image_buffer = fragment_uniform->image_buffer_values.at(index);
    SetTextureUnit(fragment_uniform, process_buffer, image_buffer->GetTextureId());
}

void Effect::SetTextureUnit(ShaderUniforms* fragment_uniform, ProcessBuffer* process_buffer, GLuint texture) {
    GLenum target = GL_TEXTURE3 + fragment_uniform->texture_unit_index;
    glActiveTexture(target);
    glBindTexture(GL_TEXTURE_2D, texture);
//    printf("name: %s index: %d texture: %d\n", fragment_uniform->name, fragment_uniform->texture_unit_index + 3, texture);
    process_buffer->SetInt(fragment_uniform->name, fragment_uniform->texture_unit_index + 3);
}

bool Effect::NeedTextureUnit(ShaderUniforms *fragment_uniform) {
    int type = fragment_uniform->type;
    return type == UniformTypeSample2D || type == UniformTypeInputTexture || type == UniformTypeInputTextureLast || type == UniformTypeRenderCacheKey || type == UniformTypeMattingTexture || type == UniformTypeInputEffectIndex;
}

void Effect::SetUniform(SubEffect* sub_effect, ProcessBuffer* process_buffer, std::vector<ShaderUniforms*> uniforms, int texture_id, uint64_t current_time) {
    int texture_unit_index = 0;
    for (auto& fragment_uniform : uniforms) {
        if (NeedTextureUnit(fragment_uniform)) {
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
                if (!sub_effect->input_effect.empty()) {
                    char* name = sub_effect->input_effect.at(fragment_uniform->input_effect_index);
                    auto input_effect_iterator = process_buffers_.begin();
                    while (input_effect_iterator != process_buffers_.end()) {
                        auto key = input_effect_iterator->first;
                        if (strcmp(key, name) == 0) {
                            printf("key: %s\n", key);
                            ProcessBuffer* input_effect_buffer = input_effect_iterator->second;
                            SetTextureUnit(fragment_uniform, process_buffer, input_effect_buffer->GetTextureId());
                        }
                        input_effect_iterator++;
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

void Effect::ParseConfig(char *config_path) {
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

    for (int i = 0; i < effect_size; ++i) {
        auto* sub_effect = new SubEffect();
        cJSON *effect_item_json = cJSON_GetArrayItem(effect_json, i);
        cJSON *vertex_shader_json = cJSON_GetObjectItem(effect_item_json, "vertexShader");
        cJSON *fragment_shader_json = cJSON_GetObjectItem(effect_item_json, "fragmentShader");
        cJSON *vertex_uniforms_json = cJSON_GetObjectItem(effect_item_json, "vertexUniforms");
        cJSON *fragment_uniforms_json = cJSON_GetObjectItem(effect_item_json, "fragmentUniforms");
        cJSON* name_json = cJSON_GetObjectItem(effect_item_json, "name");
        if (nullptr != name_json) {
            char* value = name_json->valuestring;
            sub_effect->name = CopyValue(value);
        }
        cJSON* input_effect_json = cJSON_GetObjectItem(effect_item_json, "inputEffect");
        if (nullptr != input_effect_json) {
            int input_effect_size = cJSON_GetArraySize(input_effect_json);
            for (int input_effect_index = 0; input_effect_index < input_effect_size; input_effect_index++) {
                cJSON* input_effect_value = cJSON_GetArrayItem(input_effect_json, input_effect_index);
                if (input_effect_value->type == cJSON_String) {
                    char* value = input_effect_value->valuestring;
                    sub_effect->input_effect.push_back(CopyValue(value));
                }
            }
        }
        
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
                auto* process_buffer = new ProcessBuffer();
                process_buffer->Init(vertex_shader_buffer, fragment_shader_buffer);
                process_buffers_.insert(std::pair<char*, ProcessBuffer*>(sub_effect->name, process_buffer));
                delete[] vertex_shader_buffer;
                delete[] fragment_shader_buffer;
            }
            delete[] fragment_shader_path;
            delete[] vertex_shader_path;
        }
        sub_effects_.push_back(sub_effect);
        if (nullptr != fragment_uniforms_json) {
            ParseUniform(sub_effect, config_path, fragment_uniforms_json, Fragment);
        }
        if (nullptr != vertex_uniforms_json) {
            ParseUniform(sub_effect, config_path, vertex_uniforms_json, Vertex);
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

int Effect::ReadFile(char *path, char **buffer) {
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
    printf("%s\n", data);
    *buffer = data;
    return 0;    
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
