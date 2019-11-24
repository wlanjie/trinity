//
//  model.c
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#include "model.h"

#include "model_rect.h"

Model *createModel(ModelType mType) {
    switch (mType) {
        case Rect:
            return model_rect_create();
        default:
            LOGE("invalid model type");
           return NULL;
    }
}

void freeModel(Model *model) {
    if(model != NULL){
        glDeleteBuffers(3, model->vbos);
        free(model);
    }
}
