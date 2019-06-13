//
// Created by wlanjie on 2019-06-05.
//

#ifndef TRINITY_ACTION_H
#define TRINITY_ACTION_H

enum ActionType {
    NONE,
    ROTATE,
    FLASH_WHITE,
    FILTER
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
} FlashWhiteAction;

typedef struct {
    uint64_t start_time;
    uint64_t end_time;
    uint8_t* lut;
    int update_lut;
} FilterAction;

#endif //TRINITY_ACTION_H
