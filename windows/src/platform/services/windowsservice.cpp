#include "windowsservice.hpp"

#include <QFileInfo>
#include <QTimer>

#include <utility>

#include <windows.h>
#include <dwmapi.h>
#include <shobjidl.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct WindowService::Impl {
    ComPtr<IVirtualDesktopManager> desktopManager;
};

namespace {
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

    BOOL cloaked = FALSE;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))
        && cloaked) {
        return true;
    }
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
} // namespace

WindowService::WindowService(QObject* parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_impl(new Impl())
{
    CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_ALL,
        IID_PPV_ARGS(&m_impl->desktopManager));

    m_timer->setInterval(2000);
    connect(m_timer, &QTimer::timeout, this, &WindowService::refresh);
    m_timer->start();
    refresh();
}

WindowService::~WindowService()
{
    delete m_impl;
}

void WindowService::refresh()
{
    QVariantList result;

    EnumWindows(
        [](HWND hwnd, LPARAM data) -> BOOL {
            auto* list = reinterpret_cast<QVariantList*>(data);
            if (skipWindow(hwnd))
                return TRUE;

            wchar_t title[512] = {};
            GetWindowTextW(hwnd, title, 512);
            const QString windowTitle = QString::fromWCharArray(title).simplified();
            if (windowTitle.isEmpty())
                return TRUE;

            QVariantMap item;
            item.insert(QStringLiteral("id"),
                QString::number(reinterpret_cast<quintptr>(hwnd)));
            item.insert(QStringLiteral("title"), windowTitle);
            item.insert(QStringLiteral("app"), processName(hwnd));
            item.insert(QStringLiteral("active"), GetForegroundWindow() == hwnd);
            list->append(item);
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&result));

    if (m_impl->desktopManager) {
        QVariantList filtered;
        for (const QVariant& entry : std::as_const(result)) {
            const QVariantMap item = entry.toMap();
            const HWND hwnd = reinterpret_cast<HWND>(
                item.value(QStringLiteral("id")).toString().toULongLong());
            BOOL onCurrent = TRUE;
            if (SUCCEEDED(m_impl->desktopManager->IsWindowOnCurrentVirtualDesktop(
                    hwnd, &onCurrent))
                && !onCurrent) {
                continue;
            }
            filtered.append(item);
        }
        result = filtered;
    }

    if (m_windows == result)
        return;
    m_windows = result;
    emit changed();
}

void WindowService::focusWindow(const QString& id)
{
    const HWND hwnd = reinterpret_cast<HWND>(id.toULongLong());
    if (!hwnd || !IsWindow(hwnd))
        return;
    if (IsIconic(hwnd))
        ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
    refresh();
}
