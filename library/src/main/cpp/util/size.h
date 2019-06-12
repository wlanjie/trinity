//
// Created by wlanjie on 2019/4/23.
//

#ifndef TRINITY_SIZE_H
#define TRINITY_SIZE_H

namespace trinity {

class Size {
public:
    Size(float width, float height) {
        width_ = width;
        height_ = height;
    }

    float GetWidth() { return width_; }
    float GetHeight() { return height_; }

private:
    float width_;
    float height_;
};

class Rect {
public:
    Rect(float min_x, float min_y, float width, float height) {
        min_x_ = min_x;
        min_y_ = min_y;
        width_ = width;
        height_ = height;
    }

    Rect* GetRectWithAspectRatio(Size& aspect_ratio) {
        float aspect = aspect_ratio.GetWidth() / aspect_ratio.GetHeight();
        float bounds_aspect = width_ / height_;
        float x = 0;
        float y = 0;
        float width;
        float height;
        if (aspect > bounds_aspect) {
            x = min_x_;
            width = width_;
            height = width_ / aspect;
            y = min_y_ + (height_ - height) / 2;
        } else {
            y = min_y_;
            height = height_;
            width = height_ * aspect;
            x = min_x_ + (width_ - width) / 2;
        }
        return new Rect(x, y, width, height);
    }

    float GetWidth() { return width_; }
    float GetHeight() { return height_; }

private:
    float min_x_;
    float min_y_;
    float width_;
    float height_;
};

}

#endif //TRINITY_SIZE_H
