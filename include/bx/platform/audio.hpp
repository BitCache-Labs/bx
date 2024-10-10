#pragma once

#ifdef AUDIO_IMPL
#include <bx/containers/list.hpp>
#include <bx/math/math.hpp>

using AudioHandle = u64;
constexpr AudioHandle AUDIO_INVALID_HANDLE = -1;

struct ChannelInfo
{
};

struct AudioInfo
{
	List<f32> samples;
};

class Audio
{
public:
	static bool Initialize();
	static void Reload();
	static void Shutdown();

	static AudioHandle GetDefaultChannel();

	static void CreateChannel(const ChannelInfo& info);
	static void DestroyChannel(const AudioHandle channel);
	static void SetChannelVolume(const AudioHandle channel, f32 volume);

	static void CreateAudio(const AudioInfo& info);
	static void DestroyAudio(const AudioHandle audio);
	static void PlayAudio(const AudioHandle channel, const AudioHandle audio);
	static void StopAudio(const AudioHandle channel, const AudioHandle audio);
};
#endif