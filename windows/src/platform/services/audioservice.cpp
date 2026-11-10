#include "audioservice.hpp"

#include <QDebug>
#include <QMetaObject>

#include <windows.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <propvarutil.h>
#include <propsys.h>
#include <wrl/client.h>
#include <wrl/implements.h>

namespace {
const PROPERTYKEY kDeviceFriendlyName = {
    { 0xa45c254e, 0xdf1c, 0x4efd, { 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0 } },
    14
};
} // namespace

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Make;
using Microsoft::WRL::RuntimeClass;
using Microsoft::WRL::RuntimeClassFlags;
using Microsoft::WRL::ClassicCom;

namespace {
class AudioNotificationClient final
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IMMNotificationClient> {
public:
    explicit AudioNotificationClient(AudioService* owner)
        : m_owner(owner)
    {
    }

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR) override
    {
        if (flow == eRender && role == eConsole) {
            AudioService* owner = m_owner;
            QMetaObject::invokeMethod(
                owner, [owner]() { owner->refreshDefaultDevice(); }, Qt::QueuedConnection);
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override
    {
        return S_OK;
    }

private:
    AudioService* m_owner;
};

class AudioVolumeCallback final
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IAudioEndpointVolumeCallback> {
public:
    explicit AudioVolumeCallback(AudioService* owner)
        : m_owner(owner)
    {
    }

    HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA data) override
    {
        if (!data)
            return S_OK;

        AudioService* owner = m_owner;
        const double volume = data->fMasterVolume;
        const bool muted = data->bMuted != FALSE;
        QMetaObject::invokeMethod(
            owner, [owner, volume, muted]() { owner->setVolume(volume, muted); },
            Qt::QueuedConnection);
        return S_OK;
    }

private:
    AudioService* m_owner;
};
} // namespace

struct AudioService::Impl {
    bool comInitialized = false;
    ComPtr<IMMDeviceEnumerator> enumerator;
    ComPtr<IMMDevice> device;
    ComPtr<IAudioEndpointVolume> volume;
    ComPtr<AudioNotificationClient> notification;
    ComPtr<AudioVolumeCallback> volumeCallback;
};

AudioService::AudioService(QObject* parent)
    : QObject(parent)
    , m_impl(std::make_unique<Impl>())
{
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(comResult))
        m_impl->comInitialized = true;
    else if (comResult != RPC_E_CHANGED_MODE) {
        setState(false, false, 0, false, QString());
        return;
    }

    m_impl->notification = Make<AudioNotificationClient>(this);
    m_impl->volumeCallback = Make<AudioVolumeCallback>(this);

    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
            IID_PPV_ARGS(&m_impl->enumerator)))) {
        setState(false, false, 0, false, QString());
        return;
    }

    m_impl->enumerator->RegisterEndpointNotificationCallback(m_impl->notification.Get());
    refreshDefaultDevice();
}

AudioService::~AudioService()
{
    if (!m_impl)
        return;

    if (m_impl->volume && m_impl->volumeCallback)
        m_impl->volume->UnregisterControlChangeNotify(m_impl->volumeCallback.Get());
    if (m_impl->enumerator && m_impl->notification)
        m_impl->enumerator->UnregisterEndpointNotificationCallback(m_impl->notification.Get());

    m_impl->volume.Reset();
    m_impl->device.Reset();
    m_impl->enumerator.Reset();
    m_impl->notification.Reset();
    m_impl->volumeCallback.Reset();

    if (m_impl->comInitialized)
        CoUninitialize();
}

bool AudioService::present() const
{
    return m_present;
}

bool AudioService::known() const
{
    return m_known;
}

int AudioService::volume() const
{
    return m_volume;
}

bool AudioService::muted() const
{
    return m_muted;
}

QString AudioService::deviceName() const
{
    return m_deviceName;
}

void AudioService::refreshDefaultDevice()
{
    if (!m_impl || !m_impl->enumerator)
        return;

    if (m_impl->volume && m_impl->volumeCallback)
        m_impl->volume->UnregisterControlChangeNotify(m_impl->volumeCallback.Get());
    m_impl->volume.Reset();
    m_impl->device.Reset();

    ComPtr<IMMDevice> device;
    if (FAILED(m_impl->enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device))) {
        setState(false, true, 0, false, QString());
        return;
    }

    ComPtr<IAudioEndpointVolume> volume;
    if (FAILED(device->Activate(
            __uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, &volume))) {
        setState(false, true, 0, false, QString());
        return;
    }

    m_impl->device = device;
    m_impl->volume = volume;
    if (m_impl->volumeCallback)
        m_impl->volume->RegisterControlChangeNotify(m_impl->volumeCallback.Get());

    QString deviceName;
    ComPtr<IPropertyStore> store;
    if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &store))) {
        PROPVARIANT value;
        PropVariantInit(&value);
        if (SUCCEEDED(store->GetValue(kDeviceFriendlyName, &value))
            && value.vt == VT_LPWSTR && value.pwszVal) {
            deviceName = QString::fromWCharArray(value.pwszVal);
        }
        PropVariantClear(&value);
    }

    float scalar = 0.0f;
    BOOL mute = FALSE;
    m_impl->volume->GetMasterVolumeLevelScalar(&scalar);
    m_impl->volume->GetMute(&mute);

    setState(true, true, qBound(0, qRound(scalar * 100.0f), 100), mute != FALSE, deviceName);
}

void AudioService::refreshVolume()
{
    if (!m_impl || !m_impl->volume)
        return;

    float scalar = 0.0f;
    BOOL mute = FALSE;
    m_impl->volume->GetMasterVolumeLevelScalar(&scalar);
    m_impl->volume->GetMute(&mute);

    setState(true, true, qBound(0, qRound(scalar * 100.0f), 100), mute != FALSE, m_deviceName);
}

void AudioService::setVolume(double scalar, bool muted)
{
    setState(m_present, m_known, qBound(0, qRound(scalar * 100.0), 100), muted, m_deviceName);
}

void AudioService::setVolumePercent(int percent)
{
    if (!m_impl || !m_impl->volume)
        return;
    const float scalar = qBound(0, percent, 100) / 100.0f;
    if (SUCCEEDED(m_impl->volume->SetMasterVolumeLevelScalar(scalar, nullptr)))
        refreshVolume();
}

void AudioService::toggleMute()
{
    if (!m_impl || !m_impl->volume)
        return;
    BOOL muted = FALSE;
    if (FAILED(m_impl->volume->GetMute(&muted)))
        return;
    if (SUCCEEDED(m_impl->volume->SetMute(!muted, nullptr)))
        refreshVolume();
}

void AudioService::refresh()
{
    if (m_impl && m_impl->volume)
        refreshVolume();
    else
        refreshDefaultDevice();
}

void AudioService::setState(
    bool present, bool known, int volume, bool muted, const QString& deviceName)
{
    if (m_present == present && m_known == known && m_volume == volume && m_muted == muted
        && m_deviceName == deviceName)
        return;

    m_present = present;
    m_known = known;
    m_volume = volume;
    m_muted = muted;
    m_deviceName = deviceName;
    emit changed();

    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_STATUS")) {
        qInfo().noquote() << "Audio: present=" << present << "known=" << known
                          << "volume=" << volume << "muted=" << muted
                          << "device=" << deviceName;
    }
}
