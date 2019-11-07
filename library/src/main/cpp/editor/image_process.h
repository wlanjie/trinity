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
// Created by wlanjie on 2019-06-05.
//

#ifndef TRINITY_IMAGE_PROCESS_H
#define TRINITY_IMAGE_PROCESS_H

#include <cstdint>
#include <vector>
#include <map>
#include "action.h"
#include "flash_white.h"
#include "filter.h"
#include "blur_split_screen.h"
#include "gaussian_blur.h"
#include "soul_scale.h"
#include "shake.h"
#include "skin_needling.h"

extern "C" {
#include "cJSON.h"
};

#define NO_ACTION -1
#define HORIZONTAL 0
#define VERTICAL 1

namespace trinity {

class ImageProcess {
 public:
    ImageProcess();
    ~ImageProcess();

    int Process(int texture_id, uint64_t current_time,
            int width, int height,
            int input_color_type, int output_color_type);

    int Process(uint8_t* frame, uint64_t current_time,
            int width, int height,
            int input_color_type, int output_color_type);

    void OnAction(char* config, int action_id);
    void RemoveAction(int action_id);
    void ClearAction();

 private:
    /**
     * 解析json配置
     * @param config
     * @param action_id
     */
    void ParseConfig(char* config, int action_id = -1);
    /**
     * 执行特效处理
     * @param texture_id
     * @param current_time
     * @param width
     * @param height
     * @return
     */
    int OnProcess(int texture_id, uint64_t current_time, int width, int height);
    /**
     * 旋转
     * @param rotate
     * @param action_id
     */
    void OnRotate(float rotate, int action_id);
    /**
     * 闪白
     * @param time
     * @param start_time
     * @param end_time
     * @param action_id
     */
    void OnFlashWhite(cJSON* fragment_uniforms, int start_time, int end_time, int action_id = NO_ACTION);
    /**
     * 分屏
     * @param screen_count
     * @param start_time
     * @param end_time
     * @param action_id
     */
    void OnSplitScreen(cJSON* fragment_uniforms, int start_time, int end_time, int action_id = NO_ACTION);
    /**
     * 模糊分屏
     * @param start_time
     * @param end_time
     * @param action_id
     */
    void OnBlurSplitScreen(cJSON* fragment_uniforms, int start_time, int end_time, int action_id = NO_ACTION);
    /**
     * 滤镜
     * @param start_time
     * @param end_time
     * @param json
     * @param action_id
     */
    void OnFilter(int start_time, int end_time, cJSON* json, int action_id);
    /**
     * 灵魂出窍
     * @param start_time
     * @param end_time
     * @param action_id
     */
    void OnSoulScale(cJSON* fragment_uniforms, int start_time, int end_time, int action_id = NO_ACTION);
    /**
     * 抖动
     * @param start_time
     * @param end_time
     * @param action_id
     */
    void OnShake(cJSON* fragment_uniforms, int start_time, int end_time, int action_id = NO_ACTION);
    /**
     * 毛刺
     * @param start_time
     * @param end_time
     * @param action_id
     */
    void OnSkinNeedling(cJSON* fragment_uniforms, int start_time, int end_time, int action_id = NO_ACTION);

 private:
    std::map<int, FrameBuffer*> effects_;
};

}  // namespace trinity

#endif  // TRINITY_IMAGE_PROCESS_H
