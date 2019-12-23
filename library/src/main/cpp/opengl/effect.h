//
// Created by wlanjie on 2019-12-20.
//

#ifndef TRINITY_EFFECT_H
#define TRINITY_EFFECT_H

#include <cstdint>
#include <vector>
#include <map>
#include "image_buffer.h"
#include "process_buffer.h"
#include "frame_buffer.h"
#include "stb_image.h"

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

class SubEffect {
 public:
   char* name;
   bool enable;
   std::vector<ShaderUniforms*> fragment_uniforms;
   std::vector<ShaderUniforms*> vertex_uniforms;
   std::vector<char*> input_effect;
};

class Effect {
 public:
    Effect();
    ~Effect();
    
    void ParseConfig(char* config_path);
    int OnDrawFrame(GLuint texture_id, uint64_t current_time);
    void Update(int start_time, int end_time);
 private:
    void SetFloat(ShaderUniforms* fragment_uniform, ProcessBuffer* process_buffer);
    void SetSample2D(ShaderUniforms* fragment_uniform, ProcessBuffer* process_buffer);
    void SetTextureUnit(ShaderUniforms* fragment_uniform, ProcessBuffer* process_buffer, GLuint texture);
    bool NeedTextureUnit(ShaderUniforms* fragment_uniform);
    void SetUniform(SubEffect* sub_effect, ProcessBuffer* process_buffer, std::vector<ShaderUniforms*> uniforms, int texture_id, uint64_t current_time);
    char* CopyValue(char* src);
    int ReadFile(char* path, char** buffer);
    void ParseUniform(SubEffect *sub_effect, char *config_path, cJSON *uniforms_json, ShaderUniformType type);
 private:
    std::vector<SubEffect*> sub_effects_;
    std::map<char*, ProcessBuffer*> process_buffers_;
    std::map<char*, FrameBuffer*> frame_buffers_;
    int start_time_;
    int end_time_;
};

}

#endif  // TRINITY_EFFECT_H
