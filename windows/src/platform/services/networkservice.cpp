#include "networkservice.hpp"

#include <QByteArray>
#include <QDebug>
#include <QMetaObject>
#include <QTimer>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <wlanapi.h>

namespace {
void CALLBACK wlanNotificationCallback(PWLAN_NOTIFICATION_DATA, PVOID context)
{
    if (context)
        static_cast<NetworkService*>(context)->scheduleRefresh();
}

void NETIOAPI_API_ ipInterfaceChangeCallback(
    PVOID context, PMIB_IPINTERFACE_ROW, MIB_NOTIFICATION_TYPE)
{
    if (context)
        static_cast<NetworkService*>(context)->scheduleRefresh();
}
} // namespace

NetworkService::NetworkService(QObject* parent)
    : QObject(parent)
{
    HANDLE handle = nullptr;
    DWORD negotiatedVersion = 0;
    if (WlanOpenHandle(WLAN_API_VERSION, nullptr, &negotiatedVersion, &handle) == ERROR_SUCCESS) {
        m_wlanHandle = handle;
        WlanRegisterNotification(handle, WLAN_NOTIFICATION_SOURCE_ALL, TRUE,
            wlanNotificationCallback, this, nullptr, nullptr);
    }

    HANDLE ipHandle = nullptr;
    if (NotifyIpInterfaceChange(AF_UNSPEC, ipInterfaceChangeCallback, this, FALSE, &ipHandle)
        == NO_ERROR)
        m_ipChangeHandle = ipHandle;

    refresh();
}

NetworkService::~NetworkService()
{
    stopNotifications();
}

QString NetworkService::connectionType() const
{
    return m_connectionType;
}

QString NetworkService::networkName() const
{
    return m_networkName;
}

int NetworkService::signalStrength() const
{
    return m_signalStrength;
}

bool NetworkService::connected() const
{
    return m_connected;
}

bool NetworkService::known() const
{
    return m_known;
}

bool NetworkService::wifiConnected() const
{
    return m_wifiConnected;
}

QString NetworkService::wifiSsid() const
{
    return m_wifiSsid;
}

int NetworkService::wifiSignal() const
{
    return m_wifiSignal;
}

bool NetworkService::ethernetConnected() const
{
    return m_ethernetConnected;
}

QString NetworkService::ethernetName() const
{
    return m_ethernetName;
}

void NetworkService::scheduleRefresh()
{
    QMetaObject::invokeMethod(
        this,
        [this]() {
            if (m_refreshPending)
                return;

            m_refreshPending = true;
            QTimer::singleShot(0, this, [this]() {
                m_refreshPending = false;
                refresh();
            });
        },
        Qt::QueuedConnection);
}

void NetworkService::refreshNow()
{
    scheduleRefresh();
}

void NetworkService::refresh()
{
    bool wifiConnected = false;
    QString wifiSsid;
    int wifiSignal = 0;
    bool wlanQueried = false;

    if (m_wlanHandle) {
        PWLAN_INTERFACE_INFO_LIST interfaces = nullptr;
        if (WlanEnumInterfaces(static_cast<HANDLE>(m_wlanHandle), nullptr, &interfaces)
            == ERROR_SUCCESS) {
            wlanQueried = true;

            for (DWORD i = 0; i < interfaces->dwNumberOfItems; ++i) {
                const auto& interfaceInfo = interfaces->InterfaceInfo[i];
                WLAN_INTERFACE_STATE state = wlan_interface_state_disconnected;
                DWORD dataSize = 0;
                PVOID data = nullptr;
                if (WlanQueryInterface(static_cast<HANDLE>(m_wlanHandle),
                        &interfaceInfo.InterfaceGuid, wlan_intf_opcode_interface_state, nullptr,
                        &dataSize, &data, nullptr)
                    == ERROR_SUCCESS) {
                    state = *static_cast<WLAN_INTERFACE_STATE*>(data);
                    WlanFreeMemory(data);
                    data = nullptr;
                }

                if (state != wlan_interface_state_connected)
                    continue;

                WLAN_CONNECTION_ATTRIBUTES* attributes = nullptr;
                dataSize = 0;
                if (WlanQueryInterface(static_cast<HANDLE>(m_wlanHandle),
                        &interfaceInfo.InterfaceGuid, wlan_intf_opcode_current_connection, nullptr,
                        &dataSize, reinterpret_cast<PVOID*>(&attributes), nullptr)
                    != ERROR_SUCCESS)
                    continue;

                const auto& ssid = attributes->wlanAssociationAttributes.dot11Ssid;
                if (ssid.uSSIDLength > 0) {
                    wifiSsid = QString::fromUtf8(
                        reinterpret_cast<const char*>(ssid.ucSSID), ssid.uSSIDLength);
                }
                wifiSignal = attributes->wlanAssociationAttributes.wlanSignalQuality;
                wifiConnected = true;
                WlanFreeMemory(attributes);
                break;
            }

            WlanFreeMemory(interfaces);
        }
    }

    bool ethernetConnected = false;
    QString ethernetName;
    bool adaptersQueried = false;

    ULONG bufferSize = 0;
    GetAdaptersAddresses(AF_UNSPEC,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr,
        nullptr, &bufferSize);

    if (bufferSize > 0) {
        QByteArray buffer;
        buffer.resize(static_cast<int>(bufferSize));
        auto* addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        if (GetAdaptersAddresses(AF_UNSPEC,
                GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                nullptr, addresses, &bufferSize)
            == NO_ERROR) {
            adaptersQueried = true;

            for (auto* adapter = addresses; adapter; adapter = adapter->Next) {
                if (adapter->IfType != IF_TYPE_ETHERNET_CSMACD
                    || adapter->OperStatus != IfOperStatusUp)
                    continue;

                ethernetConnected = true;
                if (adapter->FriendlyName)
                    ethernetName = QString::fromWCharArray(adapter->FriendlyName);
                break;
            }
        }
    }

    const bool known = wlanQueried || adaptersQueried;
    QString connectionType = QStringLiteral("unavailable");
    QString networkName;
    int signalStrength = 0;

    if (wifiConnected) {
        connectionType = QStringLiteral("wifi");
        networkName = wifiSsid;
        signalStrength = wifiSignal;
    } else if (ethernetConnected) {
        connectionType = QStringLiteral("ethernet");
        networkName = ethernetName;
        signalStrength = 100;
    } else if (known) {
        connectionType = QStringLiteral("offline");
    }

    setState(connectionType, networkName, signalStrength, wifiConnected || ethernetConnected,
        known, wifiConnected, wifiSsid, wifiSignal, ethernetConnected, ethernetName);
}

void NetworkService::setState(const QString& connectionType, const QString& networkName,
    int signalStrength, bool connected, bool known, bool wifiConnected, const QString& wifiSsid,
    int wifiSignal, bool ethernetConnected, const QString& ethernetName)
{
    if (m_connectionType == connectionType && m_networkName == networkName
        && m_signalStrength == signalStrength && m_connected == connected && m_known == known
        && m_wifiConnected == wifiConnected && m_wifiSsid == wifiSsid
        && m_wifiSignal == wifiSignal && m_ethernetConnected == ethernetConnected
        && m_ethernetName == ethernetName)
        return;

    m_connectionType = connectionType;
    m_networkName = networkName;
    m_signalStrength = signalStrength;
    m_connected = connected;
    m_known = known;
    m_wifiConnected = wifiConnected;
    m_wifiSsid = wifiSsid;
    m_wifiSignal = wifiSignal;
    m_ethernetConnected = ethernetConnected;
    m_ethernetName = ethernetName;
    emit changed();

    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_STATUS")) {
        qInfo().noquote() << "Network: type=" << connectionType << "name=" << networkName
                          << "signal=" << signalStrength << "connected=" << connected
                          << "wifi=" << wifiConnected << "ethernet=" << ethernetConnected
                          << "known=" << known;
    }
}

void NetworkService::stopNotifications()
{
    if (m_wlanHandle) {
        WlanRegisterNotification(static_cast<HANDLE>(m_wlanHandle),
            WLAN_NOTIFICATION_SOURCE_NONE, TRUE, nullptr, nullptr, nullptr, nullptr);
        WlanCloseHandle(static_cast<HANDLE>(m_wlanHandle), nullptr);
        m_wlanHandle = nullptr;
    }

    if (m_ipChangeHandle) {
        CancelMibChangeNotify2(static_cast<HANDLE>(m_ipChangeHandle));
        m_ipChangeHandle = nullptr;
    }
}

