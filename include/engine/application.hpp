#pragma once

class Application
{
public:
    virtual ~Application() {}
    virtual void Configure() = 0;
    virtual bool Initialize() = 0;
    virtual void Update() = 0;
    virtual void Render() = 0;
    virtual void Shutdown() = 0;
};