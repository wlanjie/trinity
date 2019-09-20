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

#include "editor_resource.h"
#include "android_xlog.h"

namespace trinity {

EditorResource::EditorResource(const char *resource_path) {
    resource_file_ = fopen(resource_path, "w+");
    root_json_ = cJSON_CreateObject();
    media_json_ = cJSON_CreateArray();
    cJSON_AddItemToObject(root_json_, "clips", media_json_);
    effect_json_ = cJSON_CreateArray();
    cJSON_AddItemToObject(root_json_, "effects", effect_json_);
    music_json_ = cJSON_CreateArray();
    cJSON_AddItemToObject(root_json_, "musics", music_json_);
}

EditorResource::~EditorResource() {
    if (nullptr != resource_file_) {
        fclose(resource_file_);
        resource_file_ = nullptr;
    }
    if (nullptr != root_json_) {
        cJSON_Delete(root_json_);
        root_json_ = nullptr;
    }
}

void EditorResource::InsertClip(MediaClip *clip) {
    if (nullptr == media_json_) {
        return;
    }
    fseek(resource_file_, 0, SEEK_SET);
    cJSON* item = cJSON_CreateObject();
    cJSON_AddItemToArray(media_json_, item);
    cJSON* path_json = cJSON_CreateString(clip->file_name);
    cJSON* start_time_json = cJSON_CreateNumber(clip->start_time);
    cJSON* end_time_json = cJSON_CreateNumber(clip->end_time);
    cJSON_AddItemToObject(item, "path", path_json);
    cJSON_AddItemToObject(item, "start_time", start_time_json);
    cJSON_AddItemToObject(item, "end_time", end_time_json);
    char* content = cJSON_Print(root_json_);
    fprintf(resource_file_, "%s", content);
    fflush(resource_file_);
    free(content);
}

void EditorResource::RemoveClip(int index) {
    if (media_json_ != nullptr) {
        cJSON_DeleteItemFromArray(media_json_, index);
    }
}

void EditorResource::ReplaceClip(int index, MediaClip *clip) {

}

void EditorResource::AddAction(const char *config, int action_id) {
    fseek(resource_file_, 0, SEEK_SET);
    cJSON* item = cJSON_CreateObject();
    cJSON_AddItemToArray(effect_json_, item);
    cJSON_AddNumberToObject(item, "actionId", action_id);
    cJSON_AddStringToObject(item, "config", config);
    char *content = cJSON_Print(root_json_);
    fprintf(resource_file_, "%s", content);
    fflush(resource_file_);
    free(content);
}

void EditorResource::UpdateAction(const char *config, int action_id) {
    fseek(resource_file_, 0, SEEK_SET);
    int effect_size = cJSON_GetArraySize(effect_json_);
    if (effect_size > 0) {
        cJSON *effect_item = effect_json_->child;
        int index = -1;
        for (int i = 0; i < effect_size; ++i) {
            cJSON* action_id_json = cJSON_GetObjectItem(effect_item, "actionId");
            if (action_id == action_id_json->valueint) {
                index = i;
                break;
            }
            effect_item = effect_item->next;
        }
        cJSON_DeleteItemFromArray(effect_json_, index);
    }
    cJSON* item = cJSON_CreateObject();
    cJSON_AddItemToArray(effect_json_, item);
    cJSON_AddNumberToObject(item, "actionId", action_id);
    cJSON_AddStringToObject(item, "config", config);
    char *content = cJSON_Print(root_json_);
    fprintf(resource_file_, "%s", content);
    fflush(resource_file_);
    free(content);
}

void EditorResource::DeleteAction(int action_id) {
    fseek(resource_file_, 0, SEEK_SET);
    int effect_size = cJSON_GetArraySize(effect_json_);
    if (effect_size > 0) {
        cJSON *effect_item = effect_json_->child;
        int index = -1;
        for (int i = 0; i < effect_size; ++i) {
            cJSON* action_id_json = cJSON_GetObjectItem(effect_item, "actionId");
            if (action_id == action_id_json->valueint) {
                index = i;
                break;
            }
            effect_item = effect_item->next;
        }
        cJSON_DeleteItemFromArray(effect_json_, index);
    }
    char *content = cJSON_Print(root_json_);
    fprintf(resource_file_, "%s", content);
    fflush(resource_file_);
    free(content);
}

void EditorResource::AddMusic(const char* config, int action_id) {
    fseek(resource_file_, 0, SEEK_SET);
    cJSON* item = cJSON_CreateObject();
    cJSON_AddNumberToObject(item, "actionId", action_id);
    cJSON_AddStringToObject(item, "config", config);
    cJSON_AddItemToArray(music_json_, item);
    char* content = cJSON_Print(root_json_);
    fprintf(resource_file_, "%s", content);
    fflush(resource_file_);
    free(content);
}

void EditorResource::UpdateMusic(const char* config, int action_id) {
    fseek(resource_file_, 0, SEEK_SET);
    int effect_size = cJSON_GetArraySize(effect_json_);
    if (effect_size > 0) {
        cJSON *effect_item = effect_json_->child;
        int index = -1;
        for (int i = 0; i < effect_size; ++i) {
            cJSON* action_id_json = cJSON_GetObjectItem(effect_item, "actionId");
            if (action_id == action_id_json->valueint) {
                index = i;
                break;
            }
            effect_item = effect_item->next;
        }
        cJSON_DeleteItemFromArray(effect_json_, index);
    }
    cJSON* item = cJSON_CreateObject();
    cJSON_AddItemToArray(effect_json_, item);
    cJSON_AddNumberToObject(item, "actionId", action_id);
    cJSON_AddStringToObject(item, "config", config);
    char *content = cJSON_Print(root_json_);
    fprintf(resource_file_, "%s", content);
    fflush(resource_file_);
    free(content);
}

void EditorResource::DeleteMusic(int action_id) {
    fseek(resource_file_, 0, SEEK_SET);
    int effect_size = cJSON_GetArraySize(effect_json_);
    if (effect_size > 0) {
        cJSON *effect_item = effect_json_->child;
        int index = -1;
        for (int i = 0; i < effect_size; ++i) {
            cJSON* action_id_json = cJSON_GetObjectItem(effect_item, "actionId");
            if (action_id == action_id_json->valueint) {
                index = i;
                break;
            }
            effect_item = effect_item->next;
        }
        cJSON_DeleteItemFromArray(effect_json_, index);
    }
    char *content = cJSON_Print(root_json_);
    fprintf(resource_file_, "%s", content);
    fflush(resource_file_);
    free(content);
}

}  // namespace trinity
