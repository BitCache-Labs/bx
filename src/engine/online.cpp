#include <engine/online.hpp>
#include <engine/module.hpp>

class NoOnline final : public Online
{
    BX_MODULE(NoOnline, Online)

public:
    bool Initialize() override;
    void Shutdown() override;

    void Update() override;
};

BX_MODULE_DEFINE(NoOnline)
BX_MODULE_DEFINE_INTERFACE(Online, NoOnline)

bool NoOnline::Initialize()
{
    return true;
}

void NoOnline::Shutdown()
{
}

void NoOnline::Update()
{
}