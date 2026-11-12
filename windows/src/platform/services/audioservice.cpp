#include "audioservice.hpp"

#include <QDebug>
#include <QMetaObject>
#include <QVariantMap>

#include <windows.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <propvarutil.h>
#include <propsys.h>
#include <wrl/client.h>
#include <wrl/implements.h>

#include <string>

namespace {
const PROPERTYKEY kDeviceFriendlyName = {
    { 0xa45c254e, 0xdf1c, 0x4efd, { 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0 } },
    14
};

// Changing the default endpoint has no documented API. IPolicyConfig is the
// interface Explorer itself uses; it has been stable since Vista and is what
// SoundSwitch / nircmd rely on. The vtable order must match policyconfig.h
// exactly, so keep every stub even though only SetDefaultEndpoint is called.
MIDL_INTERFACE("f8679f50-850a-41cf-9c72-430f290290c8")
IPolicyConfig : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE GetMixFormat(PCWSTR, WAVEFORMATEX**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(PCWSTR, INT, WAVEFORMATEX**) = 0;
    virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat(PCWSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(PCWSTR, WAVEFORMATEX*, WAVEFORMATEX*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(PCWSTR, INT, PINT64, PINT64) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(PCWSTR, PINT64) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetShareMode(PCWSTR, struct DeviceShareMode*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetShareMode(PCWSTR, struct DeviceShareMode*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(PCWSTR, const PROPERTYKEY&, PROPVARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(PCWSTR, const PROPERTYKEY&, PROPVARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(PCWSTR wszDeviceId, ERole eRole) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(PCWSTR, INT) = 0;
};

const CLSID kPolicyConfigClient = {
    0x870af99c, 0x171d, 0x4f9e, { 0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9 }
};
} // namespace

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Make;
using Microsoft::WRL::RuntimeClass;
using Microsoft::WRL::RuntimeClassFlags;
using Microsoft::WRL::ClassicCom;

namespace {
QString endpointId(IMMDevice* device)
{
    if (!device)
        return QString();

    LPWSTR rawId = nullptr;
    if (FAILED(device->GetId(&rawId)) || !rawId)
        return QString();

    const QString id = QString::fromWCharArray(rawId);
    CoTaskMemFree(rawId);
    return id;
}

QString endpointName(IMMDevice* device)
{
    if (!device)
        return QString();

    ComPtr<IPropertyStore> store;
    if (FAILED(device->OpenPropertyStore(STGM_READ, &store)))
        return QString();

    QString name;
    PROPVARIANT value;
    PropVariantInit(&value);
    if (SUCCEEDED(store->GetValue(kDeviceFriendlyName, &value)) && value.vt == VT_LPWSTR
        && value.pwszVal) {
        name = QString::fromWCharArray(value.pwszVal);
    }
    PropVariantClear(&value);
    return name;
}

// Returns the active endpoints of one flow as { id, name } maps. Only active
// endpoints are listed: a disabled device cannot be selected anyway, and it
// keeps the popout short on machines with virtual cables installed.
QVariantList listEndpoints(IMMDeviceEnumerator* enumerator, EDataFlow flow)
{
    QVariantList result;
    if (!enumerator)
        return result;

    ComPtr<IMMDeviceCollection> collection;
    if (FAILED(enumerator->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &collection)))
        return result;

    UINT count = 0;
    if (FAILED(collection->GetCount(&count)))
        return result;

    for (UINT index = 0; index < count; ++index) {
        ComPtr<IMMDevice> device;
        if (FAILED(collection->Item(index, &device)))
            continue;

        const QString id = endpointId(device.Get());
        if (id.isEmpty())
            continue;

        QString name = endpointName(device.Get());
        if (name.isEmpty())
            name = QStringLiteral("Périphérique audio");

        QVariantMap entry;
        entry.insert(QStringLiteral("id"), id);
        entry.insert(QStringLiteral("name"), name);
        result.append(entry);
    }

    return result;
}

bool applyDefaultEndpoint(const QString& id)
{
    ComPtr<IPolicyConfig> policy;
    if (FAILED(CoCreateInstance(kPolicyConfigClient, nullptr, CLSCTX_ALL,
            IID_PPV_ARGS(&policy))))
        return false;

    const std::wstring wide = id.toStdWString();
    // Windows keeps three separate defaults (console, multimedia,
    // communications). Switching only eConsole leaves media players on the
    // old device, so apply all three.
    bool switched = false;
    for (const ERole role : { eConsole, eMultimedia, eCommunications }) {
        if (SUCCEEDED(policy->SetDefaultEndpoint(wide.c_str(), role)))
            switched = true;
    }
    return switched;
}
} // namespace

namespace {
// Endpoint notifications arrive on a COM worker thread; hop back to the GUI
// thread before touching QML-visible state.
void queueRefresh(AudioService* owner, bool devices, bool defaultDevice)
{
    if (!owner)
        return;
    QMetaObject::invokeMethod(
        owner,
        [owner, devices, defaultDevice]() {
            if (defaultDevice)
                owner->refreshDefaultDevice();
            if (devices)
                owner->refreshDevices();
        },
        Qt::QueuedConnection);
}

class AudioNotificationClient final
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IMMNotificationClient> {
public:
    explicit AudioNotificationClient(AudioService* owner)
        : m_owner(owner)
    {
    }

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override
    {
        queueRefresh(m_owner, true, true);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override
    {
        queueRefresh(m_owner, true, false);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override
    {
        queueRefresh(m_owner, true, true);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR) override
    {
        if (role == eConsole && (flow == eRender || flow == eCapture))
            queueRefresh(m_owner, false, true);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override
    {
        // A renamed device keeps its id, so reload the labels.
        queueRefresh(m_owner, true, false);
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

// Capture endpoint equivalent of AudioVolumeCallback: keeps the microphone
// toggle in step with the Windows 11 mute hotkey and with other applications.
class AudioMicCallback final
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IAudioEndpointVolumeCallback> {
public:
    explicit AudioMicCallback(AudioService* owner)
        : m_owner(owner)
    {
    }

    HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA data) override
    {
        if (!data)
            return S_OK;

        AudioService* owner = m_owner;
        const bool muted = data->bMuted != FALSE;
        QMetaObject::invokeMethod(
            owner, [owner, muted]() { owner->setMicMuted(muted); }, Qt::QueuedConnection);
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
    ComPtr<IMMDevice> micDevice;
    ComPtr<IAudioEndpointVolume> micVolume;
    ComPtr<AudioNotificationClient> notification;
    ComPtr<AudioVolumeCallback> volumeCallback;
    ComPtr<AudioMicCallback> micCallback;
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
    m_impl->micCallback = Make<AudioMicCallback>(this);

    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
            IID_PPV_ARGS(&m_impl->enumerator)))) {
        setState(false, false, 0, false, QString());
        return;
    }

    m_impl->enumerator->RegisterEndpointNotificationCallback(m_impl->notification.Get());
    refreshDefaultDevice();
    refreshDevices();
}

AudioService::~AudioService()
{
    if (!m_impl)
        return;

    if (m_impl->volume && m_impl->volumeCallback)
        m_impl->volume->UnregisterControlChangeNotify(m_impl->volumeCallback.Get());
    if (m_impl->micVolume && m_impl->micCallback)
        m_impl->micVolume->UnregisterControlChangeNotify(m_impl->micCallback.Get());
    if (m_impl->enumerator && m_impl->notification)
        m_impl->enumerator->UnregisterEndpointNotificationCallback(m_impl->notification.Get());

    m_impl->volume.Reset();
    m_impl->device.Reset();
    m_impl->micVolume.Reset();
    m_impl->micDevice.Reset();
    m_impl->enumerator.Reset();
    m_impl->notification.Reset();
    m_impl->volumeCallback.Reset();
    m_impl->micCallback.Reset();

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

bool AudioService::micMuted() const
{
    return m_micMuted;
}

QString AudioService::deviceName() const
{
    return m_deviceName;
}

QVariantList AudioService::outputs() const
{
    return m_outputs;
}

QVariantList AudioService::inputs() const
{
    return m_inputs;
}

QString AudioService::defaultOutputId() const
{
    return m_defaultOutputId;
}

QString AudioService::defaultInputId() const
{
    return m_defaultInputId;
}

void AudioService::refreshDevices()
{
    if (!m_impl || !m_impl->enumerator)
        return;

    QVariantList outputs = listEndpoints(m_impl->enumerator.Get(), eRender);
    QVariantList inputs = listEndpoints(m_impl->enumerator.Get(), eCapture);
    if (m_outputs == outputs && m_inputs == inputs)
        return;

    m_outputs = outputs;
    m_inputs = inputs;
    emit devicesChanged();

    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_STATUS")) {
        qInfo().noquote() << "Audio devices: outputs=" << m_outputs.size()
                          << "inputs=" << m_inputs.size() << "default output id="
                          << m_defaultOutputId;
    }
}

bool AudioService::setDefaultOutput(const QString& id)
{
    if (!m_impl || !m_impl->enumerator || id.isEmpty())
        return false;
    if (!applyDefaultEndpoint(id))
        return false;

    refreshDefaultDevice();
    return true;
}

bool AudioService::setDefaultInput(const QString& id)
{
    if (!m_impl || !m_impl->enumerator || id.isEmpty())
        return false;
    if (!applyDefaultEndpoint(id))
        return false;

    refreshDefaultInput();
    return true;
}

void AudioService::refreshDefaultInput()
{
    if (!m_impl || !m_impl->enumerator)
        return;

    ComPtr<IMMDevice> device;
    QString id;
    if (SUCCEEDED(m_impl->enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &device)))
        id = endpointId(device.Get());

    // Re-activate only when the default capture endpoint actually moved: the
    // callback registration below is not free and would be repeated on every
    // device notification otherwise.
    if (id != m_defaultInputId || !m_impl->micVolume) {
        if (m_impl->micVolume && m_impl->micCallback)
            m_impl->micVolume->UnregisterControlChangeNotify(m_impl->micCallback.Get());

        m_impl->micVolume.Reset();
        m_impl->micDevice.Reset();

        if (device) {
            ComPtr<IAudioEndpointVolume> volume;
            if (SUCCEEDED(device->Activate(
                    __uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, &volume))) {
                m_impl->micDevice = device;
                m_impl->micVolume = volume;
                if (m_impl->micCallback)
                    m_impl->micVolume->RegisterControlChangeNotify(m_impl->micCallback.Get());
            }
        }
    }

    bool muted = false;
    if (m_impl->micVolume) {
        BOOL value = FALSE;
        if (SUCCEEDED(m_impl->micVolume->GetMute(&value)))
            muted = value != FALSE;
    }

    if (m_defaultInputId == id && m_micMuted == muted)
        return;

    m_defaultInputId = id;
    m_micMuted = muted;
    emit changed();

    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_STATUS")) {
        qInfo().noquote() << "Audio input: muted=" << muted
                          << "device=" << (id.isEmpty() ? QStringLiteral("(aucun)") : id);
    }
}

void AudioService::setMicMuted(bool muted)
{
    if (m_micMuted == muted)
        return;

    m_micMuted = muted;
    emit changed();
}

void AudioService::toggleMicMute()
{
    if (!m_impl || !m_impl->micVolume)
        return;

    m_impl->micVolume->SetMute(!m_micMuted, nullptr);

    // The endpoint callback will report the new state too; reading it back
    // here keeps the UI responsive when the callback has not fired yet.
    BOOL value = FALSE;
    if (SUCCEEDED(m_impl->micVolume->GetMute(&value)))
        setMicMuted(value != FALSE);
}

void AudioService::refreshDefaultDevice()
{
    if (!m_impl || !m_impl->enumerator)
        return;

    const QString previousOutputId = m_defaultOutputId;

    if (m_impl->volume && m_impl->volumeCallback)
        m_impl->volume->UnregisterControlChangeNotify(m_impl->volumeCallback.Get());
    m_impl->volume.Reset();
    m_impl->device.Reset();

    ComPtr<IMMDevice> device;
    if (SUCCEEDED(m_impl->enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device))) {
        ComPtr<IAudioEndpointVolume> volume;
        if (SUCCEEDED(device->Activate(
                __uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, &volume))) {
            m_impl->device = device;
            m_impl->volume = volume;
            if (m_impl->volumeCallback)
                m_impl->volume->RegisterControlChangeNotify(m_impl->volumeCallback.Get());

            m_defaultOutputId = endpointId(device.Get());

            float scalar = 0.0f;
            BOOL mute = FALSE;
            m_impl->volume->GetMasterVolumeLevelScalar(&scalar);
            m_impl->volume->GetMute(&mute);

            setState(true, true, qBound(0, qRound(scalar * 100.0f), 100), mute != FALSE,
                endpointName(device.Get()));
        }
    }

    if (!m_impl->volume) {
        m_defaultOutputId.clear();
        setState(false, true, 0, false, QString());
    }

    // setState() already emits changed() when the volume tuple differs; the
    // default device can change with an identical tuple (two devices at the
    // same level), so cover that case explicitly.
    if (previousOutputId != m_defaultOutputId)
        emit changed();

    refreshDefaultInput();
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

    refreshDevices();
    refreshDefaultInput();
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
