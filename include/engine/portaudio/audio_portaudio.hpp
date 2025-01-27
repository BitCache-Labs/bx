#pragma once

#include <engine/api.hpp>
#include <engine/audio.hpp>
#include <engine/macros.hpp>

#include <stdio.h>
#include <math.h>
#include <portaudio.h>

#define AUDIO_MAX_CLIPS_PER_CHANNEL (100)

struct BX_API ChannelImpl
{
    f32 volume = 1.0f;
    AudioHandle audios[AUDIO_MAX_CLIPS_PER_CHANNEL];
};

struct BX_API AudioImpl
{
    List<f32> samples;
    SizeType currentIndex = 0;
};

class BX_API AudioPortAudio final : public Audio
{
    BX_MODULE(AudioPortAudio, Audio)
    friend class AudioPortAudioEditor;

public:
    bool Initialize() override;
    void Reload() override;
    void Shutdown() override;

    AudioHandle GetDefaultChannel() override;

    void CreateChannel(const ChannelInfo& info) override;
    void DestroyChannel(const AudioHandle channel) override;
    void SetChannelVolume(const AudioHandle channel, f32 volume) override;

    void CreateAudio(const AudioInfo& info) override;
    void DestroyAudio(const AudioHandle audio) override;
    void PlayAudio(const AudioHandle channel, const AudioHandle audio) override;
    void StopAudio(const AudioHandle channel, const AudioHandle audio) override;

private:
    static int AudioCallback(
        const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);

private:
    PaStream* m_stream = nullptr;

    List<ChannelImpl> m_channels;
    List<AudioImpl> m_audios;
};