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
// Created by wlanjie on 2019-07-30.
//

#ifndef TRINITY_EDITOR_RESOURCE_H
#define TRINITY_EDITOR_RESOURCE_H

#include <cstdio>
#include <cstdlib>

#include "trinity.h"

extern "C" {
#include "cJSON.h"
};

namespace trinity {

class EditorResource {
 public:
    explicit EditorResource(const char* resource_path);
    ~EditorResource();

    void InsertClip(MediaClip* clip);
    void RemoveClip(int index);
    void ReplaceClip(int index, MediaClip* clip);
    void AddAction(const char* config, int action_id);
    void UpdateAction(int start_time, int end_time, int action_id);
    void DeleteAction(int action_id);
    void AddMusic(const char* config, int action_id);
    void UpdateMusic(const char* config, int action_id);
    void DeleteMusic(int action_id);
    void AddFilter(const char* config, int action_id);
    void UpdateFilter(const char* config, int start_time, int end_time, int action_id);
    void DeleteFilter(int action_id);

 private:
    FILE* resource_file_;
    cJSON* root_json_;
    cJSON* media_json_;
    cJSON* effect_json_;
    cJSON* music_json_;
    cJSON* filter_json_;
};

}  // namespace trinity

#endif  // TRINITY_EDITOR_RESOURCE_H
