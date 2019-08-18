//
// Created by wlanjie on 2019-07-30.
//

#ifndef TRINITY_EDTIOR_RESOURCE_H
#define TRINITY_EDTIOR_RESOURCE_H

#include <cstdio>
#include <cstdlib>

#include "trinity.h"

extern "C" {
#include "cJSON.h"
};

namespace trinity {

class EditorResource {
public:
    EditorResource(const char* resource_path);
    ~EditorResource();

    void InsertClip(MediaClip* clip);
    void RemoveClip(int index);
    void ReplaceClip(int index, MediaClip* clip);
    void AddFilter(const char* lut_path, uint64_t start_time, uint64_t end_time, int action_id);
    void AddMusic(const char* path, uint64_t start_time, uint64_t end_time);
    void AddAction(int effect_type, uint64_t start_time, uint64_t end_time);

private:
    FILE* resource_file_;
    cJSON* root_json_;
    cJSON* media_json_;
    cJSON* effect_json_;
};

}

#endif //TRINITY_EDTIOR_RESOURCE_H
