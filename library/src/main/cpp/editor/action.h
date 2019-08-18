//
// Created by wlanjie on 2019-06-05.
//

#ifndef TRINITY_ACTION_H
#define TRINITY_ACTION_H

enum ActionType {
    NONE = 0,
    ROTATE,
    FLASH_WHITE,
    FILTER,
    SPLIT_SCREEN
};

enum RotateType {
    ROTATE_90,
    ROTATE_180,
    ROTATE_270,
    ROTATE_FREE
};

typedef struct {
    uint64_t start_time;
    uint64_t end_time;
    uint32_t action_id;
    ActionType type;
    void* args;
} Action;

typedef struct {
    float rotate;
} RotateAction;

typedef struct {
    uint64_t start_time;
    uint64_t end_time;
    int flash_time;
    int action_id;
} FlashWhiteAction;

typedef struct {
    uint64_t start_time;
    uint64_t end_time;
    uint8_t* lut;
    int update_lut;
    int action_id;
    char* lut_path;
} FilterAction;

typedef struct {
    uint64_t start_time;
    uint64_t end_time;
    int split_screen_count;
} SplitScreenAction;

#endif //TRINITY_ACTION_H
