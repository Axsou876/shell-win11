#include "trayservice.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QTimer>
#include <QWindow>

#include <QtSvg/QSvgRenderer>

#include <windows.h>
#include <shellapi.h>

namespace {
constexpr UINT kTrayIconId = 1;
constexpr GUID kTrayGuid
    = { 0x6a0f2a7e, 0x3a1b, 0x4f2c, { 0x9e, 0x2d, 0x51, 0x7a, 0x8f, 0x30, 0x44, 0x21 } };

HICON createTrayIcon()
{
    QFile file(QStringLiteral(":/assets/logo.svg"));
    if (!file.open(QIODevice::ReadOnly))
        return nullptr;

    QByteArray svg = file.readAll();
    svg.replace("#7b7c7d", "#ffb0ca");
    svg.replace("#010203", "#efdfe2");

    QSvgRenderer renderer(svg);
    if (!renderer.isValid())
        return nullptr;

    QImage image(32, 32, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter);
    painter.end();

    return image.toHICON();
}

HWND findMenuOwner()
{
    const HWND foreground = GetForegroundWindow();
    DWORD foregroundPid = 0;
    if (foreground)
        GetWindowThreadProcessId(foreground, &foregroundPid);
    if (foreground && foregroundPid == GetCurrentProcessId() && IsWindowVisible(foreground))
        return foreground;

    for (auto* window : QGuiApplication::topLevelWindows()) {
        if (window && window->isVisible() && window->title().startsWith(QStringLiteral("Caelestia Shell")))
            return reinterpret_cast<HWND>(window->winId());
    }

    return nullptr;
}

void focusWithKeyboard()
{
    INPUT inputs[4] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_LWIN;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'B';
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'B';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_LWIN;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, inputs, sizeof(INPUT));
}
} // namespace

struct TrayService::Impl {
    QWindow* window = nullptr;
    HWND hwnd = nullptr;
    HICON icon = nullptr;
    UINT callbackMessage = 0;
    NOTIFYICONDATAW nid = {};
    bool added = false;
};

TrayService::TrayService(QObject* parent)
    : QObject(parent)
    , m_impl(std::make_unique<Impl>())
{
}

TrayService::~TrayService()
{
    if (!m_impl)
        return;

    if (m_impl->added && m_impl->hwnd) {
        NOTIFYICONDATAW nid = m_impl->nid;
        Shell_NotifyIconW(NIM_DELETE, &nid);
    }

    if (QCoreApplication::instance())
        QCoreApplication::instance()->removeNativeEventFilter(this);

    if (m_impl->icon)
        DestroyIcon(m_impl->icon);

    delete m_impl->window;
    m_impl->window = nullptr;
}

bool TrayService::available() const
{
    return m_available;
}

void TrayService::attach()
{
    if (m_impl->added || m_impl->window)
        return;

    m_impl->window = new QWindow();
    m_impl->window->setFlags(Qt::Tool | Qt::FramelessWindowHint);
    m_impl->window->resize(1, 1);
    m_impl->hwnd = reinterpret_cast<HWND>(m_impl->window->winId());
    if (!m_impl->hwnd) {
        delete m_impl->window;
        m_impl->window = nullptr;
        return;
    }

    m_impl->callbackMessage = RegisterWindowMessageW(L"CaelestiaTrayCallback");
    m_impl->icon = createTrayIcon();
    if (!m_impl->callbackMessage || !m_impl->icon)
        return;

    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = m_impl->hwnd;
    nid.uID = kTrayIconId;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP | NIF_GUID;
    nid.uCallbackMessage = m_impl->callbackMessage;
    nid.hIcon = m_impl->icon;
    nid.guidItem = kTrayGuid;
    wcscpy_s(nid.szTip, ARRAYSIZE(nid.szTip), L"Caelestia Shell");

    bool added = Shell_NotifyIconW(NIM_ADD, &nid) != FALSE;

    if (!added) {
        // Some shell configurations reject GUID registrations from moved
        // binaries. Fall back to the documented hWnd + uID identity.
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
        nid.guidItem = {};
        nid.uID = kTrayIconId;
        added = Shell_NotifyIconW(NIM_ADD, &nid) != FALSE;
    }

    if (!added) {
        qWarning("Caelestia tray: Shell_NotifyIcon(NIM_ADD) failed");
        return;
    }

    m_impl->nid = nid;
    m_impl->added = true;

    nid.uVersion = NOTIFYICON_VERSION_4;
    const bool versionSet = Shell_NotifyIconW(NIM_SETVERSION, &nid) != FALSE;
    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_TRAY"))
        qInfo() << "Caelestia tray NIM_SETVERSION:" << versionSet;

    QCoreApplication::instance()->installNativeEventFilter(this);

    NOTIFYICONIDENTIFIER identifier = {};
    identifier.cbSize = sizeof(NOTIFYICONIDENTIFIER);
    identifier.hWnd = m_impl->hwnd;
    identifier.uID = kTrayIconId;
    identifier.guidItem = kTrayGuid;
    RECT iconRect = {};
    if (SUCCEEDED(Shell_NotifyIconGetRect(&identifier, &iconRect))) {
        qInfo().noquote() << "Caelestia tray icon registered at" << iconRect.left << iconRect.top
                          << iconRect.right - iconRect.left << iconRect.bottom - iconRect.top;
    } else {
        qInfo("Caelestia tray icon registered");
    }

    m_available = true;
    emit availableChanged();
}

void TrayService::showMenu()
{
    if (!m_impl->added || !m_impl->hwnd)
        return;

    const HWND owner = findMenuOwner() ? findMenuOwner() : m_impl->hwnd;
    const HWND foreground = GetForegroundWindow();
    const DWORD foregroundThread
        = foreground ? GetWindowThreadProcessId(foreground, nullptr) : 0;
    const DWORD currentThread = GetCurrentThreadId();
    if (foregroundThread && foregroundThread != currentThread)
        AttachThreadInput(currentThread, foregroundThread, TRUE);

    const bool foregroundSet = SetForegroundWindow(owner) != FALSE;
    if (foregroundThread && foregroundThread != currentThread)
        AttachThreadInput(currentThread, foregroundThread, FALSE);

    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_TRAY"))
        qInfo() << "Caelestia tray showMenu foreground:" << foregroundSet;

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1, L"Ouvrir la zone de notification");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, 2, L"Quitter Caelestia");

    POINT point = {};
    GetCursorPos(&point);

    const int command = TrackPopupMenuEx(menu,
        TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, point.x, point.y, owner, nullptr);
    DestroyMenu(menu);
    PostMessageW(owner, WM_NULL, 0, 0);

    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_TRAY"))
        qInfo() << "Caelestia tray menu command:" << command;

    if (command == 1)
        focusNotificationArea();
    else if (command == 2)
        QCoreApplication::quit();
}

void TrayService::focusNotificationArea()
{
    if (!m_impl->added)
        return;

    NOTIFYICONDATAW nid = m_impl->nid;
    if (!Shell_NotifyIconW(NIM_SETFOCUS, &nid))
        focusWithKeyboard();
}

bool TrayService::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
{
    Q_UNUSED(result);

    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")
        return false;

    const auto* msg = static_cast<MSG*>(message);
    if (!msg || !m_impl->added || msg->hwnd != m_impl->hwnd
        || msg->message != m_impl->callbackMessage)
        return false;

    const UINT notification = LOWORD(msg->lParam);
    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_TRAY"))
        qInfo() << "Caelestia tray callback:" << notification;
    if (notification == WM_CONTEXTMENU || notification == NIN_SELECT
        || notification == NIN_KEYSELECT || notification == NIN_BALLOONUSERCLICK) {
        // Let the shell finish the click/overflow transition before opening the
        // native popup; otherwise the same mouse-up can dismiss it immediately.
        QTimer::singleShot(150, this, &TrayService::showMenu);
    }

    return false;
}
