// mixer output through XAudio2
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <xaudio2.h>

#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

#include "audio/audio.h"
#include "core/log.h"

namespace audio {

void StartOutput(std::function<void(float*, uint32_t)> render) {
    std::thread([render] {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        IXAudio2* xa = nullptr;
        IXAudio2MasteringVoice* master = nullptr;
        IXAudio2SourceVoice* voice = nullptr;
        WAVEFORMATEX wf{};
        wf.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        wf.nChannels = 2;
        wf.nSamplesPerSec = kRate;
        wf.wBitsPerSample = 32;
        wf.nBlockAlign = 8;
        wf.nAvgBytesPerSec = kRate * 8;
        HRESULT hr = XAudio2Create(&xa, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (SUCCEEDED(hr)) hr = xa->CreateMasteringVoice(&master);
        if (SUCCEEDED(hr)) hr = xa->CreateSourceVoice(&voice, &wf);
        if (FAILED(hr)) {
            LOGE("audio: XAudio2 unavailable (%08X), no sound", unsigned(hr));
            return;
        }
        voice->Start();
        constexpr uint32_t kFrames = kRate / 100;
        constexpr uint32_t kMinQueued = 3;
        constexpr uint32_t kMaxQueued = 10;
        // one slot more than can be queued, so the one rendered is never one XAudio2 still reads
        constexpr uint32_t kSlots = kMaxQueued + 1;
        std::vector<float> bufs(kSlots * kFrames * 2);
        uint32_t depth = kMinQueued;
        for (uint32_t n = 0;;) {
            XAUDIO2_VOICE_STATE st;
            voice->GetState(&st, XAUDIO2_VOICE_NOSAMPLESPLAYED);
            // a starved voice crackles: queue one buffer more each time, up to kMaxQueued
            if (n && !st.BuffersQueued && depth < kMaxQueued) {
                depth++;
                LOGI("audio: output starved, queueing %u ms", depth * 10);
            }
            if (st.BuffersQueued >= depth) {
                Sleep(2);
                continue;
            }
            float* b = &bufs[(n % kSlots) * kFrames * 2];
            render(b, kFrames);
            // XBE_AUDIO_DUMP=<file.wav>: the mixed output written as 32-bit float stereo, for checks without listening
            static FILE* dump = [] {
                const char* d = std::getenv("XBE_AUDIO_DUMP");
                FILE* f = d ? std::fopen(d, "wb") : nullptr;
                if (f) {
                    uint32_t hdr[11] = {0x46464952, 0x7FFFFFFF, 0x45564157, 0x20746D66, 16, 0x00020003, kRate, kRate * 8, 0x00200008,
                                        0x61746164, 0x7FFFFFDB};
                    std::fwrite(hdr, 4, 11, f);
                }
                return f;
            }();
            if (dump) {
                std::fwrite(b, 8, kFrames, dump);
                std::fflush(dump);
            }
            XAUDIO2_BUFFER xb{};
            xb.AudioBytes = kFrames * 8;
            xb.pAudioData = reinterpret_cast<const BYTE*>(b);
            voice->SubmitSourceBuffer(&xb);
            n++;  // only a submitted buffer takes a slot, XAudio2 still reads the queued ones
        }
    }).detach();
}

}  // namespace audio
