#pragma once

#include <engine/api.hpp>
#include <engine/module.hpp>
#include <engine/application.hpp>
#include <engine/log.hpp>

LOG_CHANNEL(Engine)

class BX_API Engine
{
    BX_MODULE(Engine)

public:
    bool IsRunning() const noexcept;
    void Close() noexcept;

    int Run(int argc, char** args, Application& app);

private:
    bool Initialize(Application& app) noexcept;
    void Shutdown(Application& app) noexcept;

#ifdef EDITOR_BUILD
    void OnGui(Application& app) noexcept;
#endif

private:
    bool m_isRunning = true;
};