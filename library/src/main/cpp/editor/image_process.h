//
// Created by wlanjie on 2019-06-05.
//

#ifndef TRINITY_IMAGE_PROCESS_H
#define TRINITY_IMAGE_PROCESS_H

#include <cstdint>
#include <vector>
#include "action.h"
#include "flash_white.h"
#include "filter.h"

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

    void RemoveAction(int action_id);
    void ClearAction();
    int AddRotateAction(float rotate, int action_id);
    int AddFlashWhiteAction(int time, uint64_t start_time, uint64_t end_time, int action_id = NO_ACTION);
    int AddFilterAction(uint8_t* lut, int lut_size, uint64_t start_time, uint64_t end_time, int action_id = NO_ACTION);
private:
    int AddAction(ActionType type, void* args);
    int OnProcess(int texture_id, uint64_t current_time, int width, int height);

private:
    std::vector<Action> actions_;
    uint32_t current_action_id_;

    FlashWhite* flash_white_;
    Filter* filter_;
};

}

#endif //TRINITY_IMAGE_PROCESS_H
