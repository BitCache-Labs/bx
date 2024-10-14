#include <bx/platform/audio.hpp>

#include <bx/core/module.hpp>

#include <rttr/registration.h>

Audio& Audio::Get()
{
    return Module::GetFirstDerived<Audio>();
}

RTTR_PLUGIN_REGISTRATION
{
    rttr::registration::class_<Audio>("Audio")
    .method("Initialize", &Audio::Initialize)
    .method("Reload", &Audio::Reload)
    .method("Shutdown", &Audio::Shutdown)
    .method("GetDefaultChannel", &Audio::GetDefaultChannel)
    .method("CreateChannel", &Audio::CreateChannel)
    .method("DestroyChannel", &Audio::DestroyChannel)
    .method("SetChannelVolume", &Audio::SetChannelVolume)
    .method("CreateAudio", &Audio::CreateAudio)
    .method("DestroyAudio", &Audio::DestroyAudio)
    .method("PlayAudio", &Audio::PlayAudio)
    .method("StopAudio", &Audio::StopAudio);
}