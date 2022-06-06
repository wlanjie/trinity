/*
 * Copyright (C) 2020 Trinity. All rights reserved.
 * Copyright (C) 2020 Wang LianJie <wlanjie888@gmail.com>
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
// Created by wlanjie on 2020/6/15.
//

#ifndef TRINITY_ROTATE_COORDINATE_H
#define TRINITY_ROTATE_COORDINATE_H

/**
 * 获取导出软编码需要的纹理坐标
 * 因为软编码时,需要调用glReadPixel方法读取yuv数据
 * 面glReadPixel读取的数据是上下反转的, 所以在设置给
 * 软编码时纹理坐标是反转过来的,确保读取的方向是正确的
 * @param rotate 需要旋转的角度 0 90 180 270
 * @return 旋转过的纹理坐标
 */
static float* rotate_mirror_coordinate(int rotate) {
    // 软编时硬解码需要做镜像处理
    if (rotate == 90) {
        static float TEXTURE_90_MIRROR[8] = {
                0.f, 0.f,
                0.f, 1.f,
                1.f, 0.f,
                1.f, 1.f
        };
        return TEXTURE_90_MIRROR;
    } else if (rotate == 180) {
        static float TEXTURE_180_MIRROR[8] = {
                0.0f, 0.0f,
                1.0f, 0.0f,
                0.0f, 1.0f,
                1.0f, 1.0f
        };
        return TEXTURE_180_MIRROR;
    } else if (rotate == 270) {
        static float TEXTURE_270_MIRROR[8] = {
                1.f, 1.f,
                1.f, 0.f,
                0.f, 1.f,
                0.f, 0.f
        };
        return TEXTURE_270_MIRROR;
    } else {
        static float TEXTURE_MIRROR[8] = {
                0.0f, 1.0f,
                1.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 0.0f,
        };
        return TEXTURE_MIRROR;
    }
}

/**
 * 获取旋转纹理坐标
 * @param rotate 0 90 180 270
 * @return 旋转过的纹理坐标
 */
static float* rotate_coordinate(int rotate) {
    if (rotate == 90) {
        static float TEXTURE_COORDINATE_ROTATED_90[8] = {
                1.0f, 0.0f,
                1.0f, 1.0f,
                0.0f, 0.0f,
                0.0f, 1.0f
        };
        return TEXTURE_COORDINATE_ROTATED_90;
    } else if (rotate == 180) {
        static float TEXTURE_COORDINATE_ROTATED_180[8] = {
                0.0f, 1.0f,
                1.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 0.0f,
        };
        return TEXTURE_COORDINATE_ROTATED_180;
    } else if (rotate == 270) {
        static float TEXTURE_COORDINATE_ROTATED_270[8] = {
                0.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 1.0f,
                1.0f, 0.0f
        };
        return TEXTURE_COORDINATE_ROTATED_270;
    } else {
        static float TEXTURE_COORDINATE_NO_ROTATION[8] = {
                0.0f, 0.0f,
                1.0f, 0.0f,
                0.0f, 1.0f,
                1.0f, 1.0f
        };
        return TEXTURE_COORDINATE_NO_ROTATION;
    }
}

/**
 * 获取导出时软编码软解码需要的旋转纹理坐标
 * @param rotate 0 90 180 270
 * @return 旋转过的纹理坐标
 */
static float* rotate_soft_decode_encode_coordinate(int rotate) {
    if (rotate == 90) {
        static float TEXTURE_COORDINATE_ROTATED_90[8] = {
                0.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 1.0f,
                1.0f, 0.0f
        };
        return TEXTURE_COORDINATE_ROTATED_90;
    } else if (rotate == 180) {
        static float TEXTURE_COORDINATE_ROTATED_180[8] = {
                0.0f, 1.0f,
                1.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 0.0f,
        };
        return TEXTURE_COORDINATE_ROTATED_180;
    } else if (rotate == 270) {
        static float TEXTURE_COORDINATE_ROTATED_270[8] = {
                1.0f, 0.0f,
                1.0f, 1.0f,
                0.0f, 0.0f,
                0.0f, 1.0f
        };
        return TEXTURE_COORDINATE_ROTATED_270;
    } else {
        static float TEXTURE_COORDINATE_NO_ROTATION[8] = {
                0.0f, 0.0f,
                1.0f, 0.0f,
                0.0f, 1.0f,
                1.0f, 1.0f
        };
        return TEXTURE_COORDINATE_NO_ROTATION;
    }
}

/**
 * 获取导出时软解码硬解码需要的旋转纹理坐标
 * @param rotate 0 90 180 270
 * @return 旋转过的纹理坐标
 */
static float* rotate_soft_decode_media_encode_coordinate(int rotate) {
    if (rotate == 90) {
        static float TEXTURE_90_MIRROR[8] = {
                1.f, 1.f,
                1.f, 0.f,
                0.f, 1.f,
                0.f, 0.f
        };
        return TEXTURE_90_MIRROR;
    } else if (rotate == 180) {
        static float TEXTURE_180_MIRROR[8] = {
                0.0f, 0.0f,
                1.0f, 0.0f,
                0.0f, 1.0f,
                1.0f, 1.0f
        };
        return TEXTURE_180_MIRROR;
    } else if (rotate == 270) {
        static float TEXTURE_270_MIRROR[8] = {
                0.f, 0.f,
                0.f, 1.f,
                1.f, 0.f,
                1.f, 1.f
        };
        return TEXTURE_270_MIRROR;
    } else {
        static float TEXTURE_MIRROR[8] = {
                0.0f, 1.0f,
                1.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 0.0f,
        };
        return TEXTURE_MIRROR;
    }
}

#endif //TRINITY_ROTATE_COORDINATE_H
