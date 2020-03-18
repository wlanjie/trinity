//
//  macro.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef macro_h
#define macro_h

#include "android_xlog.h"

#define NDK_MEDIACODEC_VERSION 29

#define AUDIO_FLAG 0x1
#define VIDEO_FLAG 0x2

#define message_stop 1
#define message_buffer_empty 2
#define message_buffer_full 3
#define message_error 999

#define default_buffer_size 1024*1024*5
#define default_buffer_time 5.0f
#define default_read_timeout 3.0f

// 100 ms
#define NULL_LOOP_SLEEP_US 100000
// 10 ms
#define BUFFER_EMPTY_SLEEP_US 10000
// 30 fps
#define WAIT_FRAME_SLEEP_US 33333

#define PIX_FMT_EGL_EXT 10000

#define ROTATION_0 0
#define ROTATION_90 1
#define ROTATION_180 2
#define ROTATION_270 3

//////// rename avframe fields
#define HW_BUFFER_ID pkt_pos
// 0 : 0
// 1 : 90
// 2 : 180
// 3 : 270
#define FRAME_ROTATION sample_rate
// error code
#define ERROR_AUDIO_DECODE_SEND_PACKET 3001
#define ERROR_AUDIO_DECODE_CODEC_NOT_OPENED 3002
#define ERROR_AUDIO_DECODE_RECIVE_FRAME 3003

#define ERROR_VIDEO_SW_DECODE_SEND_PACKET 4101
#define ERROR_VIDEO_SW_DECODE_CODEC_NOT_OPENED 4102
#define ERROR_VIDEO_SW_DECODE_RECIVE_FRAME 4103

#define ERROR_VIDEO_HW_MEDIACODEC_RECEIVE_FRAME 501

#endif  // macro_h
