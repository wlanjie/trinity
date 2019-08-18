//
// Created by wlanjie on 2019-07-30.
//

#include "edtior_resource.h"
#include "android_xlog.h"

namespace trinity {

EditorResource::EditorResource(const char *resource_path) {
    resource_file_ = fopen(resource_path, "w+");
    root_json_ = cJSON_CreateObject();
    media_json_ = cJSON_CreateArray();
    cJSON_AddItemToObject(root_json_, "clips", media_json_);
    effect_json_ = cJSON_CreateArray();
    cJSON_AddItemToObject(root_json_, "effects", effect_json_);
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

void EditorResource::AddFilter(const char *lut_path, uint64_t start_time, uint64_t end_time, int action_id) {
    fseek(resource_file_, 0, SEEK_SET);
    int effect_size = cJSON_GetArraySize(effect_json_);
    if (effect_size > 0) {
        cJSON* effect_item = effect_json_->child;
        int index = -1;
        for (int i = 0; i < effect_size; ++i) {
            cJSON* action_id_json = cJSON_GetObjectItem(effect_item, "action_id");
            if (action_id == action_id_json->valueint) {
                index = i;
                break;
            }
            effect_item = effect_item->next;
        }
        cJSON_DeleteItemFromArray(effect_json_, index);
    }

    cJSON *item = cJSON_CreateObject();
    cJSON_AddItemToArray(effect_json_, item);
    cJSON_AddStringToObject(item, "type", "lut_filter");
    cJSON_AddStringToObject(item, "lut_path", lut_path);
    cJSON_AddNumberToObject(item, "start_time", start_time);
    cJSON_AddNumberToObject(item, "end_time", end_time);
    cJSON_AddNumberToObject(item, "action_id", action_id);
    char *content = cJSON_Print(root_json_);
    fprintf(resource_file_, "%s", content);
    fflush(resource_file_);
    free(content);
}

void EditorResource::AddMusic(const char *path, uint64_t start_time, uint64_t end_time) {

}

void EditorResource::AddAction(int effect_type, uint64_t start_time, uint64_t end_time) {

}

}