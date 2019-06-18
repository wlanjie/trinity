//
// Created by wlanjie on 2019-06-05.
//

#include "image_process.h"
#include <algorithm>

namespace trinity {

ImageProcess::ImageProcess() {
    current_action_id_ = 0;
    flash_white_ = nullptr;
    filter_ = nullptr;
    screen_two_ = nullptr;
    screen_three_ = nullptr;
    screen_four_ = nullptr;
    screen_six_ = nullptr;
    screen_nine_ = nullptr;
}

ImageProcess::~ImageProcess() {
    ClearAction();
}

int ImageProcess::Process(int texture_id, uint64_t current_time, int width, int height, int input_color_type, int output_color_type) {
    return OnProcess(texture_id, current_time, width, height);
}

int ImageProcess::Process(uint8_t *frame, uint64_t current_time, int width, int height, int input_color_type, int output_color_type) {
    return OnProcess(0, current_time, width, height);
}

int ImageProcess::OnProcess(int texture_id, uint64_t current_time, int width, int height) {
    int texture = texture_id;
    for (auto it = actions_.begin(); it != actions_.end(); ++it) {
        if (it->type == ROTATE) {
        } else if (it->type == FLASH_WHITE) {
            if (nullptr == flash_white_) {
                flash_white_ = new FlashWhite(width, height);
            }
            auto *action = reinterpret_cast<FlashWhiteAction*>(it->args);
            if (current_time >= action->start_time && current_time <= action->end_time) {
                texture = flash_white_->OnDrawFrame(texture_id);
            }
        } else if (it->type == FILTER) {
            auto* action = reinterpret_cast<FilterAction*>(it->args);
            if (nullptr == filter_) {
                filter_ = new Filter(action->lut, width, height);
            }
            if (action->update_lut == 1) {
                filter_->UpdateLut(action->lut, width, height);
            }
            if (current_time >= action->start_time && current_time <= action->end_time) {
                texture = filter_->OnDrawFrame(texture_id);
            }
        } else if (it->type == SPLIT_SCREEN) {
            auto* action = reinterpret_cast<SplitScreenAction*>(it->args);
            if (action->split_screen_count == 2) {
                if (nullptr == screen_two_) {
                    screen_two_ = new FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, SCREEN_TWO_FRAGMENT_SHADER);
                }
                if (current_time >= action->start_time && current_time <= action->end_time) {
                    texture = screen_two_->OnDrawFrame(texture_id);
                }
            } else if (action->split_screen_count == 3) {
                if (nullptr == screen_three_) {
                    screen_three_ = new FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, SCREEN_THREE_FRAGMENT_SHADER);
                }
                if (current_time >= action->start_time && current_time <= action->end_time) {
                    texture = screen_three_->OnDrawFrame(texture_id);
                }
            } else if (action->split_screen_count == 4) {
                if (nullptr == screen_four_) {
                    screen_four_ = new FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, SCREEN_FOUR_FRAGMENT_SHADER);
                }
                if (current_time >= action->start_time && current_time <= action->end_time) {
                    texture = screen_four_->OnDrawFrame(texture_id);
                }
            } else if (action->split_screen_count == 6) {
                if (nullptr == screen_six_) {
                    screen_six_ = new FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, SCREEN_SIX_FRAGMENT_SHADER);
                }
                if (current_time >= action->start_time && current_time <= action->end_time) {
                    texture = screen_six_->OnDrawFrame(texture_id);
                }
            } else if (action->split_screen_count == 9) {
                if (nullptr == screen_nine_) {
                    screen_nine_ = new FrameBuffer(width, height, DEFAULT_VERTEX_SHADER, SCREEN_NINE_FRAGMENT_SHADER);
                }
                if (current_time >= action->start_time && current_time <= action->end_time) {
                    texture = screen_nine_->OnDrawFrame(texture_id);
                }
            }
        }
    }
    return texture;
}

void ImageProcess::RemoveAction(int action_id) {

}

void ImageProcess::ClearAction() {
    for (auto it = actions_.begin(); it != actions_.end(); ++it) {
        if (it->type == ROTATE) {
            delete reinterpret_cast<RotateAction*>(it->args);
        } else if (it->type == FLASH_WHITE) {
            delete reinterpret_cast<FlashWhiteAction*>(it->args);
        } else if (it->type == FILTER) {
            FilterAction* action = reinterpret_cast<FilterAction*>(it->args);
            delete[] action->lut;
            delete action;
            if (nullptr != filter_) {
                delete filter_;
                filter_ = nullptr;
            }
        } else if (it->type == SPLIT_SCREEN) {
            SplitScreenAction* action = reinterpret_cast<SplitScreenAction*>(it->args);
            delete action;
            if (nullptr != screen_two_) {
                delete screen_two_;
                screen_two_ = nullptr;
            }
            if (nullptr != screen_three_) {
                delete screen_three_;
                screen_three_ = nullptr;
            }
            if (nullptr != screen_four_) {
                delete screen_four_;
                screen_four_ = nullptr;
            }
            if (nullptr != screen_six_) {
                delete screen_six_;
                screen_six_ = nullptr;
            }
            if (nullptr != screen_nine_) {
                delete screen_nine_;
                screen_nine_ = nullptr;
            }
        }
    }
}

int ImageProcess::AddRotateAction(float rotate, int action_id) {
   int actId = action_id;
   if (action_id == NO_ACTION) {
       auto* r = new RotateAction();
       r->rotate = rotate;
       actId = AddAction(ROTATE, r);
   } else {
       auto it = std::find_if(actions_.begin(), actions_.end(), [action_id](const Action& action) -> bool {
           return action.action_id == action_id;
       });
       if (it == actions_.end()) {
           return NO_ACTION;
       }
       if ((*it).type == ROTATE) {
           auto *rotateInfo = static_cast<RotateAction *>((*it).args);
           rotateInfo->rotate = rotate;
       }
   }
   return actId;
}

int ImageProcess::AddFlashWhiteAction(int time, uint64_t start_time, uint64_t end_time, int action_id) {
    int actId = action_id;
    if (action_id == NO_ACTION) {
        auto* action = new FlashWhiteAction();
        action->flash_time = time;
        action->start_time = start_time;
        action->end_time = end_time;
        actId = AddAction(FLASH_WHITE, action);
    } else {
        auto it = std::find_if(actions_.begin(), actions_.end(), [action_id](const Action& action) -> bool {
            return action.action_id == action_id;
        });
        if (it == actions_.end()) {
            return NO_ACTION;
        }
        if ((*it).type == FLASH_WHITE) {
            auto *flashWhiteInfo = static_cast<FlashWhiteAction *>((*it).args);
            flashWhiteInfo->flash_time = time;
            flashWhiteInfo->start_time = start_time;
            flashWhiteInfo->end_time = end_time;
        }
    }
    return actId;
}

int ImageProcess::AddFilterAction(uint8_t *lut, int lut_size, uint64_t start_time, uint64_t end_time, int action_id) {
    int actId = action_id;
    if (action_id == NO_ACTION) {
        auto* action = new FilterAction();
        uint8_t* buffer = new uint8_t[lut_size];
        memcpy(buffer, lut, lut_size);
        action->lut = buffer;
        action->start_time = start_time;
        action->end_time = end_time;
        action->update_lut = 0;
        actId = AddAction(FILTER, action);
    } else {
        auto it = std::find_if(actions_.begin(), actions_.end(), [action_id](const Action& action) -> bool {
            return action.action_id == action_id;
        });
        if (it == actions_.end()) {
            return NO_ACTION;
        }
        if ((*it).type == FILTER) {
            auto *filterInfo = static_cast<FilterAction *>((*it).args);
            if (nullptr != filterInfo->lut) {
                delete[] filterInfo->lut;
            }
            uint8_t* buffer = new uint8_t[lut_size];
            memcpy(buffer, lut, lut_size);
            filterInfo->lut = buffer;
            filterInfo->start_time = start_time;
            filterInfo->end_time = end_time;
            filterInfo->update_lut = 1;
        }
    }
    return actId;
}

int ImageProcess::AddSplitScreenAction(int screen_count, uint64_t start_time, uint64_t end_time, int action_id) {
    int actId = action_id;
    if (action_id == NO_ACTION) {
        auto* action = new SplitScreenAction();
        action->start_time = start_time;
        action->end_time = end_time;
        action->split_screen_count = screen_count;
        actId = AddAction(SPLIT_SCREEN, action);
    } else {
        auto it = std::find_if(actions_.begin(), actions_.end(), [action_id](const Action& action) -> bool {
            return action.action_id == action_id;
        });
        if (it == actions_.end()) {
            return NO_ACTION;
        }
        if ((*it).type == SPLIT_SCREEN) {
            auto *action = static_cast<SplitScreenAction *>((*it).args);
            action->start_time = start_time;
            action->end_time = end_time;
            action->split_screen_count = screen_count;
        }
    }
    return actId;
}

int ImageProcess::AddAction(ActionType type, void *args) {
    Action action;
    action.type = type;
    action.args = args;
    current_action_id_++;
    action.action_id = current_action_id_;
    auto it = actions_.begin();
    if (it == actions_.end()) {
        actions_.push_back(action);
    } else {
        actions_.insert(it, action);
    }
    return action.action_id;
}

}