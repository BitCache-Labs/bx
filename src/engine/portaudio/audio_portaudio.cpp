#include <engine/audio.hpp>

#include <engine/macros.hpp>

#include <stdio.h>
#include <math.h>
#include <portaudio.h>

//#include <rttr/registration.h>
//RTTR_PLUGIN_REGISTRATION
//{
//    rttr::registration::class_<AudioPortAudio>("AudioPortAudio")
//        .constructor();
//}

#define AUDIO_MAX_CLIPS_PER_CHANNEL (100)

class AudioPortAudio final : public Audio
{
    //RTTR_ENABLE(Audio)

public:
    AudioPortAudio() = default;

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
    PaStream* m_stream = nullptr;
};

Audio& Audio::Get()
{
    static AudioPortAudio instance;
    return instance;
}

struct ChannelImpl
{
    f32 volume = 1.0f;
    AudioHandle audios[AUDIO_MAX_CLIPS_PER_CHANNEL];
};

struct AudioImpl
{
    List<f32> samples;
    SizeType currentIndex = 0;
};

static List<ChannelImpl> g_channels;
static List<AudioImpl> g_audios;

static int AudioCallback(
    const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    auto* out = static_cast<f32*>(outputBuffer);

    for (u64 i = 0; i < framesPerBuffer; ++i)
    {
        f32 mix = 0.0f;
        for (const auto& channel : g_channels)
        {
            for (SizeType a = 0; a < AUDIO_MAX_CLIPS_PER_CHANNEL; ++a)
            {
                AudioHandle audio = channel.audios[a];
                if (audio == AUDIO_INVALID_HANDLE)
                    continue;

                auto& audioData = g_audios[audio];
                if (audioData.currentIndex < audioData.samples.size())
                {
                    mix += audioData.samples[audioData.currentIndex++] * channel.volume;
                }
            }
        }
        *out++ = mix;
    }

    return paContinue;
}

AudioHandle AudioPortAudio::GetDefaultChannel()
{
    return 0;
}

void AudioPortAudio::CreateChannel(const ChannelInfo& info)
{
}

void AudioPortAudio::DestroyChannel(const AudioHandle channel)
{
}

void AudioPortAudio::SetChannelVolume(const AudioHandle channel, f32 volume)
{
}

void AudioPortAudio::CreateAudio(const AudioInfo& info)
{
}

void AudioPortAudio::DestroyAudio(const AudioHandle audio)
{
}

void AudioPortAudio::PlayAudio(const AudioHandle channel, const AudioHandle audio)
{
}

void AudioPortAudio::StopAudio(const AudioHandle channel, const AudioHandle audio)
{
}

bool AudioPortAudio::Initialize()
{
    PaError err = Pa_Initialize();
    if (err != paNoError)
    {
        LOGE(Audio, "Failed to initialize PortAudio: {}", Pa_GetErrorText(err));
        return false;
    }

    err = Pa_OpenDefaultStream(
        &m_stream,
        0,          // no input channels
        1,          // mono output
        paFloat32,  // 32-bit floating point output
        44100,      // sample rate
        256,        // frames per buffer
        AudioCallback,
        0);// &data);

    if (err != paNoError)
    {
        LOGE(Audio, "PortAudio failed to open default stream: {}", Pa_GetErrorText(err));
        return false;
    }

    err = Pa_StartStream(m_stream);
    if (err != paNoError)
    {
        LOGE(Audio, "PortAudio failed to start stream: {}", Pa_GetErrorText(err));
        return false;
    }

    return true;
}

void AudioPortAudio::Reload()
{
}

void AudioPortAudio::Shutdown()
{
    PaError err = Pa_StopStream(m_stream);
    if (err != paNoError)
    {
        LOGE(Audio, "PortAudio failed to stop stream: ", Pa_GetErrorText(err));
    }

    err = Pa_CloseStream(m_stream);
    if (err != paNoError)
    {
        LOGE(Audio, "PortAudio failed to close stream: ", Pa_GetErrorText(err));
    }

    Pa_Terminate();
}