#pragma once

#include <engine/api.hpp>
#include <engine/module.hpp>
#include <engine/list.hpp>
#include <engine/math.hpp>
#include <engine/log.hpp>

using AudioHandle = u64;
constexpr AudioHandle AUDIO_INVALID_HANDLE = -1;

struct BX_API ChannelInfo
{
};

struct BX_API AudioInfo
{
	List<f32> samples;
};

class BX_API Audio
{
	BX_MODULE_INTERFACE(Audio)

public:
	virtual bool Initialize() = 0;
	virtual void Reload() = 0;
	virtual void Shutdown() = 0;

	virtual AudioHandle GetDefaultChannel() = 0;

	void Test() { BX_LOGI(Audio, "HEEEEEEEELLLLLOOOOOO"); }

	virtual void CreateChannel(const ChannelInfo& info) = 0;
	virtual void DestroyChannel(const AudioHandle channel) = 0;
	virtual void SetChannelVolume(const AudioHandle channel, f32 volume) = 0;

	virtual void CreateAudio(const AudioInfo& info) = 0;
	virtual void DestroyAudio(const AudioHandle audio) = 0;
	virtual void PlayAudio(const AudioHandle channel, const AudioHandle audio) = 0;
	virtual void StopAudio(const AudioHandle channel, const AudioHandle audio) = 0;
};