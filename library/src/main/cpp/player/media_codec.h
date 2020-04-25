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
//  media_codec.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef media_codec_h
#define media_codec_h

#include "play_types.h"
#include "endian.h"

#define BUFFER_FLAG_END_OF_STREAM 4

/* Inspired by libavcodec/hevc.c */
static int convert_hevc_nal_units(const uint8_t *p_buf, size_t i_buf_size,
                           uint8_t *p_out_buf, size_t i_out_buf_size,
                           size_t *p_sps_pps_size, size_t *p_nal_size) {
    int i, num_arrays;
    const uint8_t *p_end = p_buf + i_buf_size;
    uint32_t i_sps_pps_size = 0;

    if (i_buf_size <= 3 || (!p_buf[0] && !p_buf[1] && p_buf[2] <= 1)) {
        return -1;
    }

    if (p_end - p_buf < 23) {
        LOGE("Input Metadata too small");
        return -1;
    }

    p_buf += 21;

    if (p_nal_size) {
        *p_nal_size = (size_t) ((*p_buf & 0x03) + 1);
    }
    p_buf++;

    num_arrays = *p_buf++;

    for (i = 0; i < num_arrays; i++) {
        int type, cnt, j;

        if (p_end - p_buf < 3) {
            LOGE("Input Metadata too small");
            return -1;
        }
        type = *(p_buf++) & 0x3f;
        (void)(type);

        cnt = p_buf[0] << 8 | p_buf[1];
        p_buf += 2;

        for (j = 0; j < cnt; j++) {
            int i_nal_size;

            if (p_end - p_buf < 2) {
                LOGE("Input Metadata too small");
                return -1;
            }

            i_nal_size = p_buf[0] << 8 | p_buf[1];
            p_buf += 2;

            if (i_nal_size < 0 || p_end - p_buf < i_nal_size) {
                LOGE("NAL unit size does not match Input Metadata size");
                return -1;
            }

            if (i_sps_pps_size + 4 + i_nal_size > i_out_buf_size) {
                LOGE("Output buffer too small");
                return -1;
            }

            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 1;

            memcpy(p_out_buf + i_sps_pps_size, p_buf, (size_t) i_nal_size);
            p_buf += i_nal_size;

            i_sps_pps_size += i_nal_size;
        }
    }

    *p_sps_pps_size = i_sps_pps_size;

    return 0;
}
/*
static void restore_mpeg4_esds(AVCodecParameters *codecpar,
                                      uint8_t *p_buf, size_t i_buf_size,
                                      size_t i_es_dscr_length, size_t i_dec_dscr_length,
                                      uint8_t *p_esds_buf)
{
    p_esds_buf[0] = 0x03;
    p_esds_buf[1] = 0x80;
    p_esds_buf[2] = 0x80;
    p_esds_buf[3] = 0x80;
    p_esds_buf[4] = (uint8_t) i_es_dscr_length;
    uint16_t *es_id = (uint16_t *)&p_esds_buf[5];
    *es_id = htobe16(1);

    p_esds_buf[8] = 0x04;
    p_esds_buf[9] = 0x80;
    p_esds_buf[10] = 0x80;
    p_esds_buf[11] = 0x80;
    p_esds_buf[12] = (uint8_t) i_dec_dscr_length;
    p_esds_buf[13] = 0x20;
    p_esds_buf[14] = 0x11;
    uint32_t *max_br = (uint32_t *)&p_esds_buf[18];
    uint32_t *avg_br = (uint32_t *)&p_esds_buf[22];
    *max_br = *avg_br = htobe32(codecpar->bit_rate);

    p_esds_buf[26] = 0x05;
    p_esds_buf[27] = 0x80;
    p_esds_buf[28] = 0x80;
    p_esds_buf[29] = 0x80;
    p_esds_buf[30] = (uint8_t) i_buf_size;
    memcpy(&p_esds_buf[31], p_buf, i_buf_size);

    p_esds_buf[31+i_buf_size] = 0x06;
    p_esds_buf[31+i_buf_size+1] = 0x80;
    p_esds_buf[31+i_buf_size+2] = 0x80;
    p_esds_buf[31+i_buf_size+3] = 0x80;
    p_esds_buf[31+i_buf_size+4] = 0x01;
    p_esds_buf[31+i_buf_size+5] = 0x02;
}

static int convert_sps_pps(const uint8_t *p_buf, size_t i_buf_size,
                           uint8_t *p_out_buf, size_t i_out_buf_size,
                           size_t *p_sps_pps_size, size_t *p_nal_size) {
    // int i_profile;
    uint32_t i_data_size = i_buf_size, i_nal_size, i_sps_pps_size = 0;
    unsigned int i_loop_end;

    if (i_data_size < 7) {
        LOGE("Input Metadata too small");
        return -1;
    }

    // i_profile    = (p_buf[1] << 16) | (p_buf[2] << 8) | p_buf[3];
    if (p_nal_size)
        *p_nal_size = (size_t) ((p_buf[4] & 0x03) + 1);
    p_buf += 5;
    i_data_size -= 5;

    for (unsigned int j = 0; j < 2; j++) {
        if (i_data_size < 1) {
            LOGE("PPS too small after processing SPS/PPS %u",
                 i_data_size);
            return -1;
        }
        i_loop_end = (unsigned int) (p_buf[0] & (j == 0 ? 0x1f : 0xff));
        p_buf++;
        i_data_size--;

        for (unsigned int i = 0; i < i_loop_end; i++) {
            if (i_data_size < 2) {
                LOGE("SPS is too small %u", i_data_size);
                return -1;
            }
            i_nal_size = (p_buf[0] << 8) | p_buf[1];
            p_buf += 2;
            i_data_size -= 2;
            if (i_data_size < i_nal_size) {
                LOGE("SPS size does not match NAL specified size %u",
                     i_data_size);
                return -1;
            }
            if (i_sps_pps_size + 4 + i_nal_size > i_out_buf_size) {
                LOGE("Output SPS/PPS buffer too small");
                return -1;
            }
            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 1;
            memcpy(p_out_buf + i_sps_pps_size, p_buf, i_nal_size);
            i_sps_pps_size += i_nal_size;
            p_buf += i_nal_size;
            i_data_size -= i_nal_size;
        }
    }
    *p_sps_pps_size = i_sps_pps_size;
    return 0;
}
*/
static int convert_sps_pps2(const uint8_t *p_buf, size_t i_buf_size,
                            uint8_t * out_sps_buf, size_t * out_sps_buf_size,
                            uint8_t * out_pps_buf, size_t * out_pps_buf_size,
//                            uint8_t *p_out_buf, size_t i_out_buf_size,
//                            size_t *p_sps_pps_size,
                            size_t *p_nal_size
) {
    // int i_profile;
    uint32_t i_data_size = (uint32_t) i_buf_size, i_nal_size;
    unsigned int i_loop_end;

    /* */
    if (i_data_size < 7) {
        LOGE("Input Metadata too small");
        return -1;
    }

    /* Read infos in first 6 bytes */
    // i_profile    = (p_buf[1] << 16) | (p_buf[2] << 8) | p_buf[3];
    if (p_nal_size)
        *p_nal_size = (size_t) ((p_buf[4] & 0x03) + 1);
    p_buf += 5;
    i_data_size -= 5;

    for (unsigned int j = 0; j < 2; j++) {
        /* First time is SPS, Second is PPS */
        if (i_data_size < 1) {
            LOGE("PPS too small after processing SPS/PPS %u",
                 i_data_size);
            return -1;
        }
        i_loop_end = (unsigned int) (p_buf[0] & (j == 0 ? 0x1f : 0xff));
        p_buf++;
        i_data_size--;

        for (unsigned int i = 0; i < i_loop_end; i++) {
            if (i_data_size < 2) {
                LOGE("SPS is too small %u", i_data_size);
                return -1;
            }

            i_nal_size = (p_buf[0] << 8) | p_buf[1];
            p_buf += 2;
            i_data_size -= 2;

            if (i_data_size < i_nal_size) {
                LOGE("SPS size does not match NAL specified size %u",
                     i_data_size);
                return -1;
            }
//            if (i_sps_pps_size + 4 + i_nal_size > i_out_buf_size) {
//                LOGE("Output SPS/PPS buffer too small");
//                return -1;
//            }
//
//            p_out_buf[i_sps_pps_size++] = 0;
//            p_out_buf[i_sps_pps_size++] = 0;
//            p_out_buf[i_sps_pps_size++] = 0;
//            p_out_buf[i_sps_pps_size++] = 1;
//
//            memcpy(p_out_buf + i_sps_pps_size, p_buf, i_nal_size);
//            i_sps_pps_size += i_nal_size;
            if (j == 0) {
                out_sps_buf[0] = 0;
                out_sps_buf[1] = 0;
                out_sps_buf[2] = 0;
                out_sps_buf[3] = 1;
                memcpy(out_sps_buf + 4, p_buf, i_nal_size);
                * out_sps_buf_size = i_nal_size + 4;
            } else {
                out_pps_buf[0] = 0;
                out_pps_buf[1] = 0;
                out_pps_buf[2] = 0;
                out_pps_buf[3] = 1;
                memcpy(out_pps_buf + 4, p_buf, i_nal_size);
                * out_pps_buf_size = i_nal_size + 4;
            }

            p_buf += i_nal_size;
            i_data_size -= i_nal_size;
        }
    }

//    *p_sps_pps_size = i_sps_pps_size;

    return 0;
}

typedef struct H264ConvertState {
    uint32_t nal_len;
    uint32_t nal_pos;
} H264ConvertState;

static void convert_h264_to_annexb(uint8_t *p_buf, size_t i_len,
                                    size_t i_nal_size,
                                    H264ConvertState *state ) {
    if (i_nal_size < 3 || i_nal_size > 4) {
        return;
    }

    /* This only works for NAL sizes 3-4 */
    while (i_len > 0) {
        if (state->nal_pos < i_nal_size) {
            unsigned int i;
            for (i = 0; state->nal_pos < i_nal_size && i < i_len; i++, state->nal_pos++) {
                state->nal_len = (state->nal_len << 8) | p_buf[i];
                p_buf[i] = 0;
            }
            if (state->nal_pos < i_nal_size) {
                return;
            }
            p_buf[i - 1] = 1;
            p_buf += i;
            i_len -= i;
        }
        if (state->nal_len > INT_MAX) {
            return;
        }
        if (state->nal_len > i_len) {
            state->nal_len -= i_len;
            return;
        } else {
            p_buf += state->nal_len;
            i_len -= state->nal_len;
            state->nal_len = 0;
            state->nal_pos = 0;
        }
    }
}

MediaCodecContext * create_mediacodec_context(AVPlayContext *context);
void mediacodec_release_buffer(AVPlayContext *context, AVFrame *frame);
int mediacodec_receive_frame(AVPlayContext *context, AVFrame *frame);
int mediacodec_dequeue_input_buffer_index(AVPlayContext* context);
int mediacodec_send_packet(AVPlayContext *context, AVPacket *packet, int buffer_index);
void mediacodec_end_of_stream(AVPlayContext* context, int buffer_index);
void mediacodec_flush(AVPlayContext *context);
void mediacodec_seek(AVPlayContext* context);
int mediacodec_start(AVPlayContext *context);
void mediacodec_stop(AVPlayContext *context);
void mediacodec_update_image(AVPlayContext* context);
int mediacodec_get_texture_matrix(AVPlayContext* context, float* texture_matrix);
bool mediacodec_frame_available(AVPlayContext* context);
#endif  // media_codec_h
