//
// Created by wlanjie on 2019-11-28.
//

#ifndef TRINITY_PLAYER_EVENT_OBSERVER_H
#define TRINITY_PLAYER_EVENT_OBSERVER_H

namespace trinity {

class PlayerEventObserver {

public:
    virtual void OnComplete() = 0;
};

}

#endif //TRINITY_PLAYER_EVENT_OBSERVER_H
