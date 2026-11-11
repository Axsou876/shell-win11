#include "audiocaptureservice.hpp"
#include "audioanalyzer.hpp"

#include <QMetaObject>
#include <QVector>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

#include <windows.h>
#include <audioclient.h>
#include <ksmedia.h>
#include <mmdeviceapi.h>
#include <propsys.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace {
constexpr int kFftBandCount = AudioAnalyzer::BandCount;

// {a45c254e-df1c-4efd-8020-67d146a850e0}, 14 (PKEY_Device_FriendlyName).
const PROPERTYKEY kDeviceFriendlyName = {
    { 0xa45c254e, 0xdf1c, 0x4efd, { 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0 } },
    14
};

QString deviceId(IMMDevice* device)
{
    if (!device)
        return {};
    LPWSTR id = nullptr;
    if (FAILED(device->GetId(&id)) || !id)
        return {};
    const QString result = QString::fromWCharArray(id);
    CoTaskMemFree(id);
    return result;
}

QString friendlyName(IMMDevice* device)
{
    if (!device)
        return {};
    ComPtr<IPropertyStore> store;
    if (FAILED(device->OpenPropertyStore(STGM_READ, &store)))
        return {};
    PROPVARIANT value;
    PropVariantInit(&value);
    QString result;
    if (SUCCEEDED(store->GetValue(kDeviceFriendlyName, &value))
        && value.vt == VT_LPWSTR && value.pwszVal) {
        result = QString::fromWCharArray(value.pwszVal);
    }
    PropVariantClear(&value);
    return result;
}

bool sameDefaultDevice(IMMDeviceEnumerator* enumerator, const QString& id)
{
    if (!enumerator || id.isEmpty())
        return false;
    ComPtr<IMMDevice> current;
    if (FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &current)))
        return false;
    return deviceId(current.Get()) == id;
}
} // namespace

AudioCaptureService::AudioCaptureService(QObject* parent)
    : QObject(parent)
{
    // Keep QML's model a stable 64 entries before the first capture.
    for (int i = 0; i < kFftBandCount; ++i)
        m_bands.append(0.0);
}

AudioCaptureService::~AudioCaptureService()
{
    stopCapture();
}

int AudioCaptureService::bandCount() const
{
    return kFftBandCount;
}

void AudioCaptureService::setActive(bool active)
{
    if (m_active.load() == active)
        return;

    m_active = active;
    emit activeChanged();

    if (active)
        startCapture();
    else
        stopCapture();
}

void AudioCaptureService::startCapture()
{
    if (m_thread.joinable())
        m_thread.join();
    m_stop = false;
    m_thread = std::thread([this]() { captureLoop(); });
}

void AudioCaptureService::stopCapture()
{
    m_stop = true;
    if (m_thread.joinable())
        m_thread.join();
    m_stop = false;
    publishStatus(false, {}, {});
}

void AudioCaptureService::publishBands(const QVector<float>& bands)
{
    QVariantList list;
    list.reserve(bands.size());
    for (float value : bands)
        list.append(value);

    QMetaObject::invokeMethod(
        this,
        [this, list = std::move(list)]() {
            m_bands = list;
            emit bandsChanged();
        },
        Qt::QueuedConnection);
}

void AudioCaptureService::publishStatus(
    bool available, const QString& error, const QString& deviceName)
{
    QMetaObject::invokeMethod(
        this,
        [this, available, error, deviceName]() {
            const bool statusChanged = m_available != available || m_error != error
                || m_deviceName != deviceName;
            m_available = available;
            m_error = error;
            m_deviceName = deviceName;
            if (statusChanged)
                emit changed();
        },
        Qt::QueuedConnection);
}

void AudioCaptureService::captureLoop()
{
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(comResult)) {
        publishStatus(false, QStringLiteral("COM audio indisponible"), {});
        return;
    }

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    ComPtr<IMMDeviceEnumerator> enumerator;
    HRESULT hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
    if (FAILED(hr)) {
        publishStatus(false, QStringLiteral("Périphérique audio indisponible"), {});
        CoUninitialize();
        return;
    }

    AudioAnalyzer analyzer;

    while (!m_stop.load() && m_active.load()) {
        ComPtr<IMMDevice> device;
        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
        if (FAILED(hr) || !device) {
            publishStatus(false, QStringLiteral("Aucun périphérique de sortie"), {});
            Sleep(1000);
            continue;
        }

        const QString id = deviceId(device.Get());
        const QString name = friendlyName(device.Get());

        ComPtr<IAudioClient> client;
        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, &client);
        if (FAILED(hr)) {
            publishStatus(false, QStringLiteral("Capture audio inaccessible"), name);
            Sleep(1000);
            continue;
        }

        WAVEFORMATEX* mixFormat = nullptr;
        hr = client->GetMixFormat(&mixFormat);
        if (FAILED(hr) || !mixFormat) {
            publishStatus(false, QStringLiteral("Format audio introuvable"), name);
            Sleep(1000);
            continue;
        }

        const int channels = mixFormat->nChannels;
        const int sampleRate = static_cast<int>(mixFormat->nSamplesPerSec);
        const int bits = mixFormat->wBitsPerSample;
        const int blockAlign = mixFormat->nBlockAlign;
        bool isFloat = mixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT;
        bool isPcm = mixFormat->wFormatTag == WAVE_FORMAT_PCM;
        if (mixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
            const auto* ext = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(mixFormat);
            isFloat = IsEqualGUID(ext->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
            isPcm = IsEqualGUID(ext->SubFormat, KSDATAFORMAT_SUBTYPE_PCM);
        }

        const bool supported = channels > 0 && blockAlign > 0
            && ((isFloat && bits == 32) || (isPcm && (bits == 16 || bits == 24 || bits == 32)));
        const int bytesPerSample = bits / 8;

        if (!supported) {
            CoTaskMemFree(mixFormat);
            publishStatus(false,
                QStringLiteral("Format audio non pris en charge (%1 bits)").arg(bits), name);
            Sleep(1000);
            continue;
        }

        hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
            0, 0, mixFormat, nullptr);
        CoTaskMemFree(mixFormat);
        if (FAILED(hr)) {
            publishStatus(false, QStringLiteral("Initialisation capture audio impossible"), name);
            Sleep(1000);
            continue;
        }

        ComPtr<IAudioCaptureClient> capture;
        hr = client->GetService(IID_PPV_ARGS(&capture));
        if (FAILED(hr) || !capture) {
            publishStatus(false, QStringLiteral("Service de capture audio indisponible"), name);
            Sleep(1000);
            continue;
        }

        analyzer.setSampleRate(sampleRate);
        analyzer.reset();
        hr = client->Start();
        if (FAILED(hr)) {
            publishStatus(false, QStringLiteral("Démarrage capture audio impossible"), name);
            Sleep(1000);
            continue;
        }

        publishStatus(true, {}, name);

        std::vector<float> mono;
        auto lastPublish = std::chrono::steady_clock::now();
        auto lastDeviceCheck = std::chrono::steady_clock::now();

        while (!m_stop.load() && m_active.load()) {
            Sleep(10);

            const auto now = std::chrono::steady_clock::now();
            if (now - lastDeviceCheck > std::chrono::seconds(2)) {
                lastDeviceCheck = now;
                if (!sameDefaultDevice(enumerator.Get(), id))
                    break;
            }

            UINT32 packetSize = 0;
            hr = capture->GetNextPacketSize(&packetSize);
            if (FAILED(hr))
                break;

            while (packetSize > 0 && !m_stop.load() && m_active.load()) {
                BYTE* data = nullptr;
                UINT32 frames = 0;
                DWORD flags = 0;
                hr = capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
                if (FAILED(hr))
                    break;

                if (frames > 0) {
                    if (mono.size() < frames)
                        mono.resize(frames);

                    if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                        std::fill(mono.begin(), mono.begin() + frames, 0.0f);
                    } else if (isFloat) {
                        const auto* samples = reinterpret_cast<const float*>(data);
                        const int stride = channels;
                        for (UINT32 frame = 0; frame < frames; ++frame) {
                            float sum = 0.0f;
                            for (int channel = 0; channel < channels; ++channel)
                                sum += samples[frame * stride + channel];
                            mono[frame] = sum / channels;
                        }
                    } else if (bits == 16) {
                        const auto* samples = reinterpret_cast<const int16_t*>(data);
                        const int stride = channels;
                        for (UINT32 frame = 0; frame < frames; ++frame) {
                            float sum = 0.0f;
                            for (int channel = 0; channel < channels; ++channel)
                                sum += samples[frame * stride + channel] / 32768.0f;
                            mono[frame] = sum / channels;
                        }
                    } else if (bits == 24) {
                        const auto* samples = reinterpret_cast<const uint8_t*>(data);
                        const int stride = channels * bytesPerSample;
                        for (UINT32 frame = 0; frame < frames; ++frame) {
                            float sum = 0.0f;
                            for (int channel = 0; channel < channels; ++channel) {
                                const uint8_t* p = samples + frame * stride + channel * bytesPerSample;
                                int32_t value = static_cast<int32_t>(p[0])
                                    | (static_cast<int32_t>(p[1]) << 8)
                                    | (static_cast<int32_t>(p[2]) << 16);
                                if (value & 0x800000)
                                    value |= ~0xFFFFFF;
                                sum += value / 8388608.0f;
                            }
                            mono[frame] = sum / channels;
                        }
                    } else { // 32-bit PCM
                        const auto* samples = reinterpret_cast<const int32_t*>(data);
                        const int stride = channels;
                        for (UINT32 frame = 0; frame < frames; ++frame) {
                            float sum = 0.0f;
                            for (int channel = 0; channel < channels; ++channel)
                                sum += samples[frame * stride + channel] / 2147483648.0f;
                            mono[frame] = sum / channels;
                        }
                    }

                    analyzer.pushSamples(mono.data(), static_cast<int>(frames));
                }

                capture->ReleaseBuffer(frames);
                hr = capture->GetNextPacketSize(&packetSize);
                if (FAILED(hr))
                    break;
            }

            if (hr == AUDCLNT_E_DEVICE_INVALIDATED)
                break;

            if (std::chrono::steady_clock::now() - lastPublish
                >= std::chrono::milliseconds(16)) {
                publishBands(analyzer.bands());
                lastPublish = std::chrono::steady_clock::now();
            }
        }

        client->Stop();
        if (m_stop.load() || !m_active.load())
            break;

        // A device change or invalidation restarts the client with the new
        // default endpoint.
        publishStatus(false, {}, name);
        Sleep(250);
    }

    publishStatus(false, {}, {});
    CoUninitialize();
}
