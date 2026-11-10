#include "platform/platform.hpp"
#include "platform/services/audioservice.hpp"
#include "platform/services/batteryservice.hpp"
#include "platform/services/bluetoothservice.hpp"
#include "platform/services/networkservice.hpp"
#include "platform/services/trayservice.hpp"

#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTimer>

#include <qqml.h>

#include <windows.h>

namespace {
QString graphicsApiName(QSGRendererInterface::GraphicsApi api)
{
    switch (api) {
    case QSGRendererInterface::Direct3D11:
        return QStringLiteral("Direct3D 11");
    case QSGRendererInterface::Direct3D12:
        return QStringLiteral("Direct3D 12");
    case QSGRendererInterface::OpenGL:
        return QStringLiteral("OpenGL");
    case QSGRendererInterface::Vulkan:
        return QStringLiteral("Vulkan");
    case QSGRendererInterface::Metal:
        return QStringLiteral("Metal");
    case QSGRendererInterface::Software:
        return QStringLiteral("Software");
    case QSGRendererInterface::Null:
        return QStringLiteral("Null");
    default:
        return QStringLiteral("Unknown");
    }
}
} // namespace

int main(int argc, char* argv[])
{
    // Phase 1 explicitly exercises the Direct3D 11 RHI path so the HLSL
    // variants of the QSB shaders are loaded. The environment variable can
    // still be overridden for comparison runs.
    if (qEnvironmentVariableIsEmpty("QSG_RHI_BACKEND"))
        qputenv("QSG_RHI_BACKEND", "d3d11");

    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Caelestia"));
    app.setApplicationName(QStringLiteral("Caelestia Shell Windows"));
    app.setApplicationVersion(QStringLiteral("0.2.5"));

    // A second launch must not create another AppBar or tray icon. The named
    // mutex is released by the OS even if the first instance crashes.
    const HANDLE instanceMutex = CreateMutexW(nullptr, FALSE, L"Local\\CaelestiaShellWindows");
    if (instanceMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        qInfo("Caelestia Shell Windows is already running; exiting.");
        CloseHandle(instanceMutex);
        return 0;
    }
    if (!instanceMutex)
        qWarning("Caelestia single-instance mutex could not be created; continuing unprotected");

    Platform platform;
    BatteryService battery;
    NetworkService network;
    BluetoothService bluetooth;
    AudioService audio;
    TrayService tray;

    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Platform", &platform);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Battery", &battery);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Network", &network);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Bluetooth", &bluetooth);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Audio", &audio);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Tray", &tray);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("Caelestia.Shell", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    tray.attach();

    const auto findBarWindow = []() -> QQuickWindow* {
        QQuickWindow* fallback = nullptr;
        for (auto* window : QGuiApplication::topLevelWindows()) {
            auto* quickWindow = qobject_cast<QQuickWindow*>(window);
            if (!quickWindow || !quickWindow->isVisible()
                || !quickWindow->title().startsWith(QStringLiteral("Caelestia Shell")))
                continue;
            if (quickWindow->screen() == QGuiApplication::primaryScreen())
                return quickWindow;
            if (!fallback)
                fallback = quickWindow;
        }
        return fallback;
    };

    QTimer::singleShot(0, &app, [&platform]() {
        for (auto* window : QGuiApplication::topLevelWindows()) {
            auto* quickWindow = qobject_cast<QQuickWindow*>(window);
            if (!quickWindow || !quickWindow->title().startsWith(QStringLiteral("Caelestia Shell")))
                continue;
            QObject::connect(quickWindow, &QQuickWindow::sceneGraphInitialized, quickWindow,
                [&platform, quickWindow]() {
                    const QString backend
                        = graphicsApiName(quickWindow->rendererInterface()->graphicsApi());
                    platform.setRhiBackend(backend);
                    qInfo().noquote() << "Caelestia Shell Windows RHI backend:" << backend;
                });
        }
    });

    if (app.arguments().contains(QStringLiteral("--smoke-test"))) {
        int delay = qEnvironmentVariableIntValue("CAELESTIA_SMOKE_DELAY");
        if (delay <= 0)
            delay = 1500;

        QTimer::singleShot(delay, &app, [findBarWindow]() {
            auto* window = findBarWindow();
            if (!window) {
                qWarning("Smoke-test: no Caelestia AppBar window found");
                QCoreApplication::quit();
                return;
            }
            const QString path = QDir::current().filePath(QStringLiteral("caelestia-smoke.png"));
            const QImage image = window->grabWindow();
            const bool saved = image.save(path);
            qInfo().noquote() << "Smoke-test screenshot:" << path << (saved ? "saved" : "FAILED");
            QCoreApplication::quit();
        });
    }

    const int exitCode = app.exec();
    if (instanceMutex)
        CloseHandle(instanceMutex);
    return exitCode;
}


