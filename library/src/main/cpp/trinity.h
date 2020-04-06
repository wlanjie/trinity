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
 *
 */

//
// Created by wlanjie on 2019-08-10.
//

#ifndef TRINITY_TRINITY_H
#define TRINITY_TRINITY_H

#include <stdint.h>

#define IMAGE 10
#define VIDEO 11

typedef struct {
    char* file_name;
    int64_t start_time;
    int64_t end_time;
    int type;
} MediaClip;

typedef struct {
    int action_id;
    char* effect_name;
    char* param_name;
    float value;
} EffectParam;

typedef enum {
    kEffect             = 100,
    kEffectUpdate,
    kEffectParamUpdate,
    kEffectDelete,
    kMusic,
    kMusicUpdate,
    kMusicDelete,
    kFilter,
    kFilterUpdate,
    kFilterDelete,
} EffectMessage;

#endif  // TRINITY_TRINITY_H
