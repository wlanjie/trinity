//
//  model.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef model_h
#define model_h

#include "video_render_types.h"

Model *createModel(ModelType mType);

void freeModel(Model *model);

#endif  // model_h
