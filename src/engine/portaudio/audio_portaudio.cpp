#include <engine/portaudio/audio_portaudio.hpp>
#include <engine/log.hpp>

//#include <rttr/registration.h>
//RTTR_PLUGIN_REGISTRATION
//{
//    rttr::registration::class_<AudioPortAudio>("AudioPortAudio")
//        .constructor();
//}

BX_MODULE_DEFINE(AudioPortAudio)
BX_MODULE_DEFINE_INTERFACE(Audio, AudioPortAudio)

int AudioPortAudio::AudioCallback(
    const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    auto& impl = AudioPortAudio::Get();
    auto* out = static_cast<f32*>(outputBuffer);

    for (u64 i = 0; i < framesPerBuffer; ++i)
    {
        f32 mix = 0.0f;
        for (const auto& channel : impl.m_channels)
        {
            for (SizeType a = 0; a < AUDIO_MAX_CLIPS_PER_CHANNEL; ++a)
            {
                AudioHandle audio = channel.audios[a];
                if (audio == AUDIO_INVALID_HANDLE)
                    continue;

                auto& audioData = impl.m_audios[audio];
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
        BX_LOGE(Audio, "Failed to initialize PortAudio: {}", Pa_GetErrorText(err));
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
        BX_LOGE(Audio, "PortAudio failed to open default stream: {}", Pa_GetErrorText(err));
        return false;
    }

    err = Pa_StartStream(m_stream);
    if (err != paNoError)
    {
        BX_LOGE(Audio, "PortAudio failed to start stream: {}", Pa_GetErrorText(err));
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
        BX_LOGE(Audio, "PortAudio failed to stop stream: ", Pa_GetErrorText(err));
    }

    err = Pa_CloseStream(m_stream);
    if (err != paNoError)
    {
        BX_LOGE(Audio, "PortAudio failed to close stream: ", Pa_GetErrorText(err));
    }

    Pa_Terminate();
}