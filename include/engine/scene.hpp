#pragma once

#include <engine/api.hpp>
#include <engine/type.hpp>

class BX_API Scene
{
    BX_TYPE(Scene)

public:
    virtual ~Scene() = default;

    virtual void Play() = 0;
    virtual void Pause() = 0;
    virtual void Stop() = 0;
    virtual void Update() = 0;
    virtual void Render() = 0;
};