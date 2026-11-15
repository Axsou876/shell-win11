#include "virtualdesktopservice.hpp"

#include <QDebug>
#include <QSettings>
#include <QFileInfo>
#include <QMetaObject>
#include <QStringList>

#include <algorithm>
#include <chrono>
#include <utility>
#include <vector>

#include <windows.h>
#include <servprov.h>
#include <shobjidl.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace {
// The bar shows three desktops at most: beyond that the column gets taller
// than the bar and a single wheel gesture would keep manufacturing desktops.
constexpr int kMaxDesktops = 3;

// Windows build, read from the registry. GetVersionEx lies without an app
// manifest, and winternl.h's RTL struct clashes with the COM headers here.
DWORD windowsBuildNumber()
{
    QSettings settings(
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
        QSettings::NativeFormat);
    return static_cast<DWORD>(
        settings.value(QStringLiteral("CurrentBuildNumber")).toUInt());
}

// The private interfaces below are layout snapshots: their vtable order changes
// between Windows builds, and calling the wrong slot is an access violation
// rather than an error code. They were validated on 24H2 and 26H2 (26100 and
// 26300). Any other build gets the service marked unavailable instead of
// risking the process — the workspace column simply stays hidden.
bool virtualDesktopLayoutSupported()
{
    const DWORD build = windowsBuildNumber();
    return build >= 26100 && build < 27000;
}

const CLSID kImmersiveShell
    = { 0xC2F03A33, 0x21F5, 0x47FA, { 0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39 } };
const CLSID kVirtualDesktopManagerInternal
    = { 0xC5E0CDCA, 0x7B6E, 0x41B2, { 0x9F, 0xC4, 0xD9, 0x39, 0x75, 0xCC, 0x46, 0x7B } };
const GUID kApplicationViewCollection
    = { 0x1841C6D7, 0x4F9D, 0x42C0, { 0xAF, 0x41, 0x87, 0x47, 0x53, 0x8F, 0x10, 0xE5 } };

// Interface layouts validated against Windows 11 24H2 and 26H2 (build 26300).
struct __declspec(uuid("3F07F4BE-B107-441A-AF0F-39D82529072C")) IVirtualDesktopPrivate
    : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE IsViewVisible(IUnknown* view, BOOL* visible) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetId(GUID* id) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetName(void** name) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetWallpaperPath(void** path) = 0;
    virtual HRESULT STDMETHODCALLTYPE IsRemote(BOOL* remote) = 0;
};

// Private Explorer view collection. Used only as a fallback for windows the
// documented IVirtualDesktopManager refuses (E_ACCESSDENIED, e.g. elevated
// processes); the view pointer is passed straight to MoveViewToDesktop.
struct __declspec(uuid("372E1D3B-38D3-42E4-A15B-8AB2B178F513")) IApplicationViewPrivate
    : IUnknown {
};

struct __declspec(uuid("1841C6D7-4F9D-42C0-AF41-8747538F10E5")) IApplicationViewCollectionPrivate
    : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetViews(IObjectArray** views) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewsByZOrder(IObjectArray** views) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewsByAppUserModelId(void* id, IObjectArray** views) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewForHwnd(HWND hwnd, IApplicationViewPrivate** view) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewForApplication(
        IUnknown* app, IApplicationViewPrivate** view) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewForAppUserModelId(
        void* id, IApplicationViewPrivate** view) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewInFocus(IApplicationViewPrivate** view) = 0;
    virtual HRESULT STDMETHODCALLTYPE Unknown1(IApplicationViewPrivate** view) = 0;
    virtual HRESULT STDMETHODCALLTYPE RefreshCollection() = 0;
    virtual HRESULT STDMETHODCALLTYPE RegisterForApplicationViewChanges(
        IUnknown* listener, DWORD* cookie) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnregisterForApplicationViewChanges(DWORD cookie) = 0;
};

struct __declspec(uuid("53F5CA0B-158F-4124-900C-057158060B27")) IVirtualDesktopManagerInternalPrivate
    : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetCount(UINT* count) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveViewToDesktop(
        IUnknown* view, IVirtualDesktopPrivate* desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE CanViewMoveDesktops(IUnknown* view, BOOL* can) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentDesktop(IVirtualDesktopPrivate** desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesktops(IObjectArray** desktops) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAdjacentDesktop(
        IVirtualDesktopPrivate* from, int direction, IVirtualDesktopPrivate** desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktop(IVirtualDesktopPrivate* desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktopAndMoveForegroundView(
        IVirtualDesktopPrivate* desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDesktop(IVirtualDesktopPrivate** desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveDesktop(IVirtualDesktopPrivate* desktop, int index) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemoveDesktop(
        IVirtualDesktopPrivate* desktop, IVirtualDesktopPrivate* fallback) = 0;
    virtual HRESULT STDMETHODCALLTYPE FindDesktop(
        const GUID* id, IVirtualDesktopPrivate** desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesktopSwitchIncludeExcludeViews(
        IVirtualDesktopPrivate* desktop, IObjectArray** unknown1, IObjectArray** unknown2) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDesktopName(
        IVirtualDesktopPrivate* desktop, void* name) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDesktopWallpaper(
        IVirtualDesktopPrivate* desktop, void* path) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdateWallpaperPathForAllDesktops(void* path) = 0;
    virtual HRESULT STDMETHODCALLTYPE CopyDesktopState(IUnknown* view0, IUnknown* view1) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateRemoteDesktop(
        void* path, IVirtualDesktopPrivate** desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchRemoteDesktop(
        IVirtualDesktopPrivate* desktop, void* switchType) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktopWithAnimation(
        IVirtualDesktopPrivate* desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLastActiveDesktop(
        IVirtualDesktopPrivate** desktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE WaitForAnimationToComplete() = 0;
};

QString guidString(const GUID& id)
{
    wchar_t buffer[64] = {};
    if (StringFromGUID2(id, buffer, 64) <= 0)
        return {};
    QString text = QString::fromWCharArray(buffer);
    if (text.startsWith(QLatin1Char('{')) && text.endsWith(QLatin1Char('}')))
        text = text.mid(1, text.size() - 2);
    return text;
}

bool skipWindow(HWND hwnd)
{
    if (!IsWindowVisible(hwnd))
        return true;
    if (GetWindowTextLengthW(hwnd) == 0)
        return true;
    if (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
        return true;

    wchar_t className[128] = {};
    GetClassNameW(hwnd, className, 128);
    const QString name = QString::fromWCharArray(className);
    if (name == QStringLiteral("Progman") || name == QStringLiteral("WorkerW")
        || name == QStringLiteral("Shell_TrayWnd")
        || name == QStringLiteral("Shell_SecondaryTrayWnd")
        || name == QStringLiteral("Windows.UI.Core.CoreWindow")) {
        return true;
    }

    // Inactive virtual desktops cloak their windows; do not skip them here,
    // the desktop id is exactly what this service needs to collect.
    return false;
}

QString processName(HWND hwnd)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid)
        return QStringLiteral("Application");

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process)
        return QStringLiteral("Application");

    wchar_t path[32768] = {};
    DWORD size = 32768;
    QString result;
    if (QueryFullProcessImageNameW(process, 0, path, &size))
        result = QFileInfo(QString::fromWCharArray(path)).completeBaseName();
    CloseHandle(process);
    return result.isEmpty() ? QStringLiteral("Application") : result;
}

struct WindowEntry {
    HWND hwnd = nullptr;
    GUID desktop = {};
    QString title;
    QString app;
};

struct WindowCollector {
    IVirtualDesktopManager* manager = nullptr;
    std::vector<WindowEntry>* windows = nullptr;
};

BOOL CALLBACK collectWindow(HWND hwnd, LPARAM data)
{
    auto* collector = reinterpret_cast<WindowCollector*>(data);
    if (!collector || !collector->manager || skipWindow(hwnd))
        return TRUE;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId())
        return TRUE;

    GUID desktop = {};
    if (FAILED(collector->manager->GetWindowDesktopId(hwnd, &desktop)))
        return TRUE;

    wchar_t title[512] = {};
    GetWindowTextW(hwnd, title, 512);
    const QString text = QString::fromWCharArray(title).simplified();
    if (text.isEmpty())
        return TRUE;

    collector->windows->push_back({ hwnd, desktop, text, processName(hwnd) });
    return TRUE;
}

bool findDesktop(IVirtualDesktopManagerInternalPrivate* manager, const QString& id,
    ComPtr<IVirtualDesktopPrivate>& desktop, ComPtr<IObjectArray>& array)
{
    if (!manager)
        return false;
    if (FAILED(manager->GetDesktops(&array)) || !array)
        return false;

    UINT count = 0;
    if (FAILED(array->GetCount(&count)))
        return false;

    for (UINT i = 0; i < count; ++i) {
        ComPtr<IVirtualDesktopPrivate> candidate;
        if (FAILED(array->GetAt(i, __uuidof(IVirtualDesktopPrivate),
                reinterpret_cast<void**>(candidate.GetAddressOf())))
            || !candidate) {
            continue;
        }
        GUID guid = {};
        if (FAILED(candidate->GetId(&guid)))
            continue;
        if (guidString(guid) == id) {
            desktop = candidate;
            return true;
        }
    }
    return false;
}
} // namespace

VirtualDesktopService::VirtualDesktopService(QObject* parent)
    : QAbstractListModel(parent)
{
    m_thread = std::thread([this]() { run(); });
}

VirtualDesktopService::~VirtualDesktopService()
{
    stop();
}

void VirtualDesktopService::stop()
{
    m_stop = true;
    m_cv.notify_all();
    if (m_thread.joinable())
        m_thread.join();
}

void VirtualDesktopService::enqueue(Command command)
{
    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_VDESK")) {
        qInfo().noquote() << "VDESK enqueue" << static_cast<int>(command.type)
                          << command.id << command.windowId;
    }
    {
        std::lock_guard lock(m_mutex);
        m_commands.push_back(std::move(command));
    }
    m_cv.notify_one();
}

void VirtualDesktopService::refresh()
{
    enqueue({ CommandType::Refresh, {}, {} });
}

void VirtualDesktopService::switchTo(const QString& id)
{
    enqueue({ CommandType::Switch, id, {} });
}

void VirtualDesktopService::switchBy(int delta)
{
    if (delta == 0 || m_desktops.isEmpty())
        return;

    int current = -1;
    for (int i = 0; i < m_desktops.size(); ++i) {
        if (m_desktops.at(i).toMap().value(QStringLiteral("id")).toString() == m_currentId) {
            current = i;
            break;
        }
    }
    if (current < 0)
        return;

    if (delta > 0) {
        // Forward: the next existing desktop, or a brand new one when this is
        // the last — up to the cap. Explorer switches to a desktop it just
        // created, so the scroll keeps going down the column instead of
        // stopping dead.
        if (current + 1 < m_desktops.size())
            switchTo(m_desktops.at(current + 1).toMap().value(QStringLiteral("id")).toString());
        else if (m_desktops.size() < kMaxDesktops)
            createDesktop();
        return;
    }

    // Backward never creates: there is nothing above the first desktop.
    if (current > 0)
        switchTo(m_desktops.at(current - 1).toMap().value(QStringLiteral("id")).toString());
}

void VirtualDesktopService::createDesktop()
{
    // The bar only offers three: more than that makes the column unreadable
    // and the wheel gesture turns into a desktop factory.
    if (m_desktops.size() >= kMaxDesktops) {
        if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_VDESK"))
            qInfo().noquote() << "VDESK create refused, already" << m_desktops.size();
        return;
    }
    enqueue({ CommandType::Create, {}, {} });
}

void VirtualDesktopService::closeDesktop(const QString& id)
{
    enqueue({ CommandType::Close, id, {} });
}

void VirtualDesktopService::moveWindow(const QString& windowId, const QString& desktopId)
{
    enqueue({ CommandType::Move, desktopId, windowId });
}

int VirtualDesktopService::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_desktops.size());
}

QVariant VirtualDesktopService::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_desktops.size())
        return {};

    const QVariantMap item = m_desktops.at(index.row()).toMap();
    switch (role) {
    case IdRole:
        return item.value(QStringLiteral("id"));
    case NameRole:
        return item.value(QStringLiteral("name"));
    case IndexRole:
        return item.value(QStringLiteral("index"));
    case ActiveRole:
        return item.value(QStringLiteral("active"));
    case OccupiedRole:
        return item.value(QStringLiteral("occupied"));
    case WindowsRole:
        return item.value(QStringLiteral("windows"));
    default:
        return {};
    }
}

QHash<int, QByteArray> VirtualDesktopService::roleNames() const
{
    return {
        { IdRole, "id" },
        { NameRole, "name" },
        { IndexRole, "desktopIndex" },
        { ActiveRole, "active" },
        { OccupiedRole, "occupied" },
        { WindowsRole, "windows" },
    };
}

void VirtualDesktopService::applyDesktops(const QVariantList& desktops)
{
    const auto idOf = [](const QVariant& value) {
        return value.toMap().value(QStringLiteral("id")).toString();
    };

    if (m_desktops.isEmpty()) {
        if (desktops.isEmpty())
            return;
        beginResetModel();
        m_desktops = desktops;
        endResetModel();
        return;
    }

    QStringList oldIds;
    oldIds.reserve(m_desktops.size());
    for (const QVariant& value : m_desktops)
        oldIds.append(idOf(value));
    QStringList newIds;
    newIds.reserve(desktops.size());
    for (const QVariant& value : desktops)
        newIds.append(idOf(value));

    // Keep delegates stable for additions/removals; a manual reorder in Task
    // View falls back to a model reset.
    int searchFrom = 0;
    for (const QString& id : oldIds) {
        const qsizetype at = newIds.indexOf(id, searchFrom);
        if (at < 0) {
            beginResetModel();
            m_desktops = desktops;
            endResetModel();
            return;
        }
        searchFrom = at + 1;
    }

    for (int i = static_cast<int>(m_desktops.size()) - 1; i >= 0; --i) {
        if (newIds.contains(oldIds.at(i)))
            continue;
        beginRemoveRows({}, i, i);
        m_desktops.removeAt(i);
        oldIds.removeAt(i);
        endRemoveRows();
    }

    for (int i = 0; i < desktops.size(); ++i) {
        if (i < oldIds.size() && oldIds.at(i) == newIds.at(i))
            continue;
        beginInsertRows({}, i, i);
        m_desktops.insert(i, desktops.at(i));
        oldIds.insert(i, newIds.at(i));
        endInsertRows();
    }

    Q_ASSERT(m_desktops.size() == desktops.size());
    for (int i = 0; i < desktops.size(); ++i) {
        const QVariantMap previous = m_desktops.at(i).toMap();
        const QVariantMap next = desktops.at(i).toMap();
        if (previous == next)
            continue;

        QVector<int> roles;
        if (previous.value(QStringLiteral("name")) != next.value(QStringLiteral("name")))
            roles.append(NameRole);
        if (previous.value(QStringLiteral("index")) != next.value(QStringLiteral("index")))
            roles.append(IndexRole);
        if (previous.value(QStringLiteral("active")) != next.value(QStringLiteral("active")))
            roles.append(ActiveRole);
        if (previous.value(QStringLiteral("occupied")) != next.value(QStringLiteral("occupied")))
            roles.append(OccupiedRole);
        if (previous.value(QStringLiteral("windows")) != next.value(QStringLiteral("windows")))
            roles.append(WindowsRole);

        m_desktops[i] = next;
        if (!roles.isEmpty())
            emit dataChanged(index(i), index(i), roles);
    }
}

void VirtualDesktopService::publish(QVariantList desktops, const QString& currentId,
    bool available, const QString& error)
{
    QMetaObject::invokeMethod(
        this,
        [this, desktops = std::move(desktops), currentId, available, error]() {
            const bool desktopsEqual = m_desktops == desktops;
            if (desktopsEqual && m_currentId == currentId && m_available == available
                && m_error == error) {
                return;
            }
            if (!desktopsEqual)
                applyDesktops(desktops);
            m_currentId = currentId;
            m_available = available;
            m_error = error;
            emit changed();
        },
        Qt::QueuedConnection);
}

void VirtualDesktopService::run()
{
    if (!virtualDesktopLayoutSupported()) {
        publish({}, {}, false,
            QStringLiteral("Bureaux virtuels : Windows build %1 non prise en charge")
                .arg(windowsBuildNumber()));
        return;
    }

    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com)) {
        publish({}, {}, false, QStringLiteral("COM bureaux virtuels indisponible"));
        return;
    }

    ComPtr<IServiceProvider> provider;
    ComPtr<IVirtualDesktopManagerInternalPrivate> internal;
    ComPtr<IVirtualDesktopManager> documented;
    ComPtr<IApplicationViewCollectionPrivate> viewCollection;
    QString lastCommandError;

    const auto ensureManager = [&]() -> bool {
        if (internal && documented)
            return true;
        if (!provider) {
            if (FAILED(CoCreateInstance(kImmersiveShell, nullptr, CLSCTX_ALL,
                    IID_PPV_ARGS(&provider)))
                || !provider) {
                return false;
            }
        }
        if (!internal) {
            if (FAILED(provider->QueryService(kVirtualDesktopManagerInternal,
                    __uuidof(IVirtualDesktopManagerInternalPrivate),
                    reinterpret_cast<void**>(internal.GetAddressOf())))
                || !internal) {
                return false;
            }
        }
        if (!documented) {
            if (FAILED(CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_ALL,
                    IID_PPV_ARGS(&documented)))
                || !documented) {
                return false;
            }
        }
        if (!viewCollection) {
            // Optional: only needed for windows the documented move API rejects.
            provider->QueryService(kApplicationViewCollection,
                __uuidof(IApplicationViewCollectionPrivate),
                reinterpret_cast<void**>(viewCollection.GetAddressOf()));
        }
        return true;
    };

    const auto refreshState = [&](const QString& commandError) -> bool {
        if (!ensureManager())
            return false;

        ComPtr<IObjectArray> array;
        if (FAILED(internal->GetDesktops(&array)) || !array)
            return false;

        UINT count = 0;
        if (FAILED(array->GetCount(&count)) || count == 0)
            return false;

        ComPtr<IVirtualDesktopPrivate> currentDesktop;
        GUID currentGuid = {};
        if (FAILED(internal->GetCurrentDesktop(&currentDesktop)) || !currentDesktop
            || FAILED(currentDesktop->GetId(&currentGuid))) {
            return false;
        }

        struct DesktopInfo {
            QString id;
            GUID guid = {};
            int index = 0;
            QVariantList windows;
        };
        std::vector<DesktopInfo> list;
        list.reserve(count);

        for (UINT i = 0; i < count; ++i) {
            ComPtr<IVirtualDesktopPrivate> desktop;
            if (FAILED(array->GetAt(i, __uuidof(IVirtualDesktopPrivate),
                    reinterpret_cast<void**>(desktop.GetAddressOf())))
                || !desktop) {
                continue;
            }
            GUID guid = {};
            if (FAILED(desktop->GetId(&guid)))
                continue;
            DesktopInfo info;
            info.id = guidString(guid);
            info.guid = guid;
            info.index = static_cast<int>(i);
            list.push_back(std::move(info));
        }
        if (list.empty())
            return false;

        std::vector<WindowEntry> windows;
        WindowCollector collector { documented.Get(), &windows };
        EnumWindows(collectWindow, reinterpret_cast<LPARAM>(&collector));

        for (const WindowEntry& window : windows) {
            for (DesktopInfo& desktop : list) {
                if (!IsEqualGUID(desktop.guid, window.desktop))
                    continue;
                QVariantMap entry;
                entry.insert(QStringLiteral("id"),
                    QString::number(reinterpret_cast<quintptr>(window.hwnd)));
                entry.insert(QStringLiteral("title"), window.title);
                entry.insert(QStringLiteral("app"), window.app);
                desktop.windows.append(entry);
                break;
            }
        }

        QVariantList result;
        const QString currentId = guidString(currentGuid);
        for (DesktopInfo& desktop : list) {
            QVariantMap entry;
            entry.insert(QStringLiteral("id"), desktop.id);
            entry.insert(QStringLiteral("name"),
                QStringLiteral("Bureau %1").arg(desktop.index + 1));
            entry.insert(QStringLiteral("index"), desktop.index);
            entry.insert(QStringLiteral("active"), desktop.id == currentId);
            entry.insert(QStringLiteral("occupied"), !desktop.windows.isEmpty());
            entry.insert(QStringLiteral("windows"), desktop.windows);
            result.append(entry);
        }
        if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_VDESK")) {
            qInfo().noquote() << "VDESK refresh count" << result.size()
                              << "current" << currentId << "windows" << windows.size()
                              << "error" << commandError;
            for (const QVariant& value : result) {
                const QVariantMap desktop = value.toMap();
                qInfo().noquote() << "VDESK desktop"
                                  << desktop.value(QStringLiteral("index")).toInt()
                                  << "active" << desktop.value(QStringLiteral("active")).toBool()
                                  << "windows"
                                  << desktop.value(QStringLiteral("windows")).toList().size();
            }
        }
        publish(result, currentId, true, commandError);
        return true;
    };

    while (!m_stop.load()) {
        std::deque<Command> commands;
        {
            std::unique_lock lock(m_mutex);
            m_cv.wait_for(lock, std::chrono::seconds(1), [this]() {
                return m_stop.load() || !m_commands.empty();
            });
            if (m_stop.load())
                break;
            commands.swap(m_commands);
        }

        if (!ensureManager()) {
            publish({}, {}, false, QStringLiteral("Bureaux virtuels Windows indisponibles"));
            continue;
        }

        lastCommandError.clear();
        for (const Command& command : commands) {
            if (m_stop.load())
                break;
            if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_VDESK")) {
                qInfo().noquote() << "VDESK command" << static_cast<int>(command.type)
                                  << command.id << command.windowId;
            }
            if (command.type == CommandType::Refresh)
                continue;

            if (command.type == CommandType::Switch) {
                ComPtr<IVirtualDesktopPrivate> desktop;
                ComPtr<IObjectArray> array;
                if (findDesktop(internal.Get(), command.id, desktop, array)
                    && desktop) {
                    if (FAILED(internal->SwitchDesktop(desktop.Get())))
                        lastCommandError = QStringLiteral("Changement de bureau refusé");
                } else {
                    lastCommandError = QStringLiteral("Bureau introuvable");
                }
                Sleep(300);
            } else if (command.type == CommandType::Create) {
                ComPtr<IVirtualDesktopPrivate> created;
                if (SUCCEEDED(internal->CreateDesktop(&created)) && created) {
                    // Let Explorer publish the new desktop before switching;
                    // an immediate switch can be rejected.
                    Sleep(250);
                    if (FAILED(internal->SwitchDesktop(created.Get())))
                        lastCommandError = QStringLiteral("Bureau créé mais non activé");
                } else {
                    lastCommandError = QStringLiteral("Création de bureau impossible");
                }
                Sleep(350);
            } else if (command.type == CommandType::Close) {
                ComPtr<IVirtualDesktopPrivate> desktop;
                ComPtr<IObjectArray> array;
                if (!findDesktop(internal.Get(), command.id, desktop, array)
                    || !desktop) {
                    lastCommandError = QStringLiteral("Bureau introuvable");
                } else {
                    UINT count = 0;
                    array->GetCount(&count);
                    if (count <= 1) {
                        lastCommandError = QStringLiteral("Impossible de fermer le dernier bureau");
                    } else {
                        ComPtr<IVirtualDesktopPrivate> current;
                        GUID currentGuid = {};
                        internal->GetCurrentDesktop(&current);
                        if (current)
                            current->GetId(&currentGuid);
                        ComPtr<IVirtualDesktopPrivate> fallback;
                        GUID targetGuid = {};
                        desktop->GetId(&targetGuid);
                        UINT targetIndex = 0;
                        for (UINT i = 0; i < count; ++i) {
                            ComPtr<IVirtualDesktopPrivate> candidate;
                            array->GetAt(i, __uuidof(IVirtualDesktopPrivate),
                                reinterpret_cast<void**>(candidate.GetAddressOf()));
                            if (!candidate)
                                continue;
                            GUID candidateGuid = {};
                            candidate->GetId(&candidateGuid);
                            if (IsEqualGUID(candidateGuid, targetGuid))
                                targetIndex = i;
                            if (!IsEqualGUID(candidateGuid, targetGuid)
                                && (!fallback || IsEqualGUID(candidateGuid, currentGuid))) {
                                fallback = candidate;
                            }
                        }
                        if (!fallback && count > 1) {
                            array->GetAt((targetIndex + 1) % count,
                                __uuidof(IVirtualDesktopPrivate),
                                reinterpret_cast<void**>(fallback.GetAddressOf()));
                        }
                        if (fallback
                            && SUCCEEDED(internal->RemoveDesktop(desktop.Get(), fallback.Get()))) {
                            // Windows selects the fallback when the removed desktop was active.
                        } else {
                            lastCommandError = QStringLiteral("Fermeture de bureau refusée");
                        }
                    }
                }
                Sleep(350);
            } else if (command.type == CommandType::Move) {
                const HWND hwnd = reinterpret_cast<HWND>(command.windowId.toULongLong());
                ComPtr<IVirtualDesktopPrivate> desktop;
                ComPtr<IObjectArray> array;
                if (findDesktop(internal.Get(), command.id, desktop, array)
                    && desktop && hwnd && IsWindow(hwnd)) {
                    GUID guid = {};
                    desktop->GetId(&guid);
                    HRESULT moved = documented->MoveWindowToDesktop(hwnd, guid);
                    const bool documentedFailed = FAILED(moved);
                    if (documentedFailed && viewCollection) {
                        ComPtr<IApplicationViewPrivate> view;
                        if (SUCCEEDED(viewCollection->GetViewForHwnd(hwnd, view.GetAddressOf()))
                            && view) {
                            moved = internal->MoveViewToDesktop(view.Get(), desktop.Get());
                        }
                    }
                    if (FAILED(moved))
                        lastCommandError = QStringLiteral("Déplacement de fenêtre refusé");
                    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_VDESK")) {
                        qInfo().noquote() << "VDESK move" << command.windowId
                                          << "to" << command.id
                                          << "fallback" << documentedFailed
                                          << "result" << static_cast<qint32>(moved);
                    }
                } else {
                    lastCommandError = QStringLiteral("Fenêtre ou bureau introuvable");
                }
                Sleep(250);
            }
        }

        if (!refreshState(lastCommandError))
            publish({}, {}, false, lastCommandError.isEmpty()
                ? QStringLiteral("Bureaux virtuels Windows indisponibles")
                : lastCommandError);
    }

    CoUninitialize();
}

