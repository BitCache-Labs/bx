#include <engine/online.hpp>
#include <engine/guard.hpp>

#ifdef ONLINE_DUMMY_BACKEND
class OnlineDummy final : public Online
{
    //RTTR_ENABLE(Online)
    SINGLETON(OnlineDummy)

public:
    static OnlineDummy& Get();

public:
    bool Initialize() override;
    void Shutdown() override;
};

Online& Online::Get()
{
    return OnlineDummy::Get();
}

OnlineDummy& OnlineDummy::Get()
{
    static OnlineDummy instance;
    return instance;
}

bool OnlineDummy::Initialize()
{
    return true;
}

void OnlineDummy::Shutdown()
{
}
#endif