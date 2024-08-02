#pragma once

#include "bx/engine/core/math.hpp"

namespace Vk
{
    // TODO: maybe this can go into math.hpp?
    struct Rect2D {
        Rect2D() = default;
        Rect2D(float width, float height) : offset{}, extent(width, height) {
        }

        Vec2 offset;
        Vec2 extent;
    };
}