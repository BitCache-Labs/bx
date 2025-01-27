#pragma once

#include <engine/application.hpp>

class Engine
{
public:
    static Engine& Get() noexcept;

    bool IsRunning() const noexcept;
    void Close() noexcept;

    int Run(int argc, char** args, Application& app);

private:
    Engine() = default;
    ~Engine() = default;

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    bool Initialize() noexcept;
    void Shutdown() noexcept;

#ifdef EDITOR_BUILD
    void OnGui(Application& app) noexcept;
#endif

private:
    bool m_isRunning = true;
};