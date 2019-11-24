//
//  mesh_factory.h
//  player
//
//  Created by wlanjie on 2019/11/20.
//  Copyright © 2019 com.trinity.player. All rights reserved.
//

#ifndef mesh_factory_h
#define mesh_factory_h

typedef struct {
    float* pp;
    float* tt;
    unsigned int * index;
    int ppLen, ttLen, indexLen;
} Mesh;


Mesh *get_ball_mesh();
Mesh *get_rect_mesh();
Mesh *get_planet_mesh();
Mesh *get_distortion_mesh(int eye);

void free_mesh(Mesh *p);

#endif  // mesh_factory_h
