#pragma once

#include <engine/api.hpp>
#include <engine/type.hpp>
#include <engine/scene.hpp>

class BX_API Application
    : public SceneManager
{
    BX_TYPE(Application, SceneManager)

public:
    virtual ~Application() {}
    virtual void Configure() = 0;
    virtual bool Initialize() = 0;
    virtual void Update() = 0;
    virtual void Render() = 0;
    virtual void Shutdown() = 0;
};