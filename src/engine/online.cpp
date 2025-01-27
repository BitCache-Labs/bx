#include <engine/online.hpp>
#include <engine/module.hpp>

class OnlineDummy final : public Online
{
    BX_MODULE(OnlineDummy, Online)

public:
    bool Initialize() override;
    void Shutdown() override;

    void Update() override;
};

BX_MODULE_DEFINE(OnlineDummy)
BX_MODULE_DEFINE_INTERFACE(Online, OnlineDummy)

bool OnlineDummy::Initialize()
{
    return true;
}

void OnlineDummy::Shutdown()
{
}

void OnlineDummy::Update()
{
}