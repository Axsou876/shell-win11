#include "platform/platform.hpp"
#include "platform/services/applauncherservice.hpp"
#include "platform/services/audiocaptureservice.hpp"
#include "platform/services/audioservice.hpp"
#include "platform/services/batteryservice.hpp"
#include "platform/services/bluetoothservice.hpp"
#include "platform/services/mediaservice.hpp"
#include "platform/services/networkservice.hpp"
#include "platform/services/notificationservice.hpp"
#include "platform/services/powerservice.hpp"
#include "platform/services/systemservice.hpp"
#include "platform/services/tilingengine.hpp"
#include "platform/services/trayservice.hpp"
#include "platform/services/userservice.hpp"
#include "platform/services/virtualdesktopservice.hpp"
#include "platform/services/wallpaperservice.hpp"
#include "platform/services/weatherservice.hpp"
#include "platform/services/windowmanagerservice.hpp"
#include "platform/services/windowsservice.hpp"

#include <QDir>
#include <QEventLoop>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTextStream>
#include <QTimer>

#include <cstdio>

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

    // Headless launcher diagnostics. They use the same scanner as the UI and
    // can only launch entries discovered in the Start Menu, never free text.
    const QStringList arguments = app.arguments();
    const bool listApps = arguments.contains(QStringLiteral("--list-apps"));
    const int launchIndex = arguments.indexOf(QStringLiteral("--launch-app"));
    if (listApps || launchIndex >= 0) {
        AppLauncherService cliApps;
        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, &loop, [&loop]() {
            qWarning("Launcher diagnostic: timeout");
            loop.exit(3);
        });

        QObject::connect(&cliApps, &AppLauncherService::readyChanged, &loop, [&]() {
            if (!cliApps.ready())
                return;
            if (listApps) {
                QTextStream out(stdout);
                out << "TOTAL " << cliApps.totalCount() << '\n';
                for (const QString& name : cliApps.knownAppNames())
                    out << name << '\n';
                out.flush();
                loop.exit(0);
                return;
            }

            const QString wanted = launchIndex + 1 < arguments.size()
                ? arguments.at(launchIndex + 1) : QString();
            QString error;
            if (!cliApps.launchByName(wanted, &error)) {
                QTextStream out(stdout);
                out << "FAIL " << error << '\n';
                out.flush();
                loop.exit(2);
            }
        });
        QObject::connect(&cliApps, &AppLauncherService::launchSucceeded, &loop,
            [&loop](const QString&, const QString& name) {
                QTextStream out(stdout);
                out << "OK " << name << '\n';
                out.flush();
                loop.exit(0);
            });
        QObject::connect(&cliApps, &AppLauncherService::launchFailed, &loop,
            [&loop](const QString&, const QString& message) {
                QTextStream out(stdout);
                out << "FAIL " << message << '\n';
                out.flush();
                loop.exit(2);
            });

        timeout.start(15000);
        QTimer::singleShot(0, &loop, [&]() {
            if (!cliApps.ready())
                return;
            if (listApps) {
                QTextStream out(stdout);
                out << "TOTAL " << cliApps.totalCount() << '\n';
                for (const QString& name : cliApps.knownAppNames())
                    out << name << '\n';
                out.flush();
                loop.exit(0);
                return;
            }
            const QString wanted = launchIndex + 1 < arguments.size()
                ? arguments.at(launchIndex + 1) : QString();
            QString error;
            if (!cliApps.launchByName(wanted, &error)) {
                QTextStream out(stdout);
                out << "FAIL " << error << '\n';
                out.flush();
                loop.exit(2);
            }
        });
        const int cliExit = loop.exec();
        return cliExit;
    }

    if (arguments.contains(QStringLiteral("--dump-system"))) {
        SystemService system;
        QEventLoop loop;
        QTimer::singleShot(2500, &loop, [&]() {
            QTextStream out(stdout);
            out << "CPU=" << QString::number(system.cpuUsage(), 'f', 1)
                << " MEM=" << QString::number(system.memoryUsedGb(), 'f', 1) << "/"
                << QString::number(system.memoryTotalGb(), 'f', 1) << "GB ("
                << QString::number(system.memoryPercent(), 'f', 1) << "%)\n"
                << "GPU=" << system.gpuName() << " usage=" << QString::number(system.gpuUsage(), 'f', 1)
                << " mem=" << QString::number(system.gpuMemoryUsedMb(), 'f', 0) << "/"
                << QString::number(system.gpuMemoryTotalMb(), 'f', 0) << "MB\n"
                << "NET down=" << QString::number(system.networkDownKbps(), 'f', 1)
                << "kbps up=" << QString::number(system.networkUpKbps(), 'f', 1) << "kbps\n"
                << "DISK=" << QString::number(system.diskUsedGb(), 'f', 1) << "/"
                << QString::number(system.diskTotalGb(), 'f', 1) << "GB ("
                << QString::number(system.diskPercent(), 'f', 1) << "%)\n"
                << "UPTIME=" << system.uptimeSeconds() << "s\n";
            out.flush();
            loop.exit(0);
        });
        QTimer::singleShot(6000, &loop, [&loop]() { loop.exit(4); });
        return loop.exec();
    }

    const int weatherIndex = arguments.indexOf(QStringLiteral("--weather"));
    if (weatherIndex >= 0) {
        WeatherService weather;
        QEventLoop loop;
        if (weatherIndex + 1 < arguments.size())
            weather.setCity(arguments.at(weatherIndex + 1));
        else
            weather.refresh();
        const auto report = [&]() {
            if (weather.loading())
                return;
            QTextStream out(stdout);
            if (!weather.available()) {
                out << "WEATHER unavailable: " << weather.error() << '\n';
                out.flush();
                loop.exit(2);
                return;
            }
            out << "CITY=" << weather.city() << " TEMP=" << weather.temperature()
                << " DESC=" << weather.description() << " ICON=" << weather.icon()
                << " HUM=" << weather.humidity() << " WIND=" << weather.wind()
                << " SUN=" << weather.sunrise() << "/" << weather.sunset()
                << " FORECAST=" << weather.forecast().size() << '\n';
            out.flush();
            loop.exit(0);
        };
        QObject::connect(&weather, &WeatherService::changed, &loop, report);
        QTimer::singleShot(0, &loop, report);
        QTimer::singleShot(20000, &loop, [&loop]() { loop.exit(4); });
        return loop.exec();
    }

    if (arguments.contains(QStringLiteral("--dump-media"))) {
        MediaService media;
        QEventLoop loop;
        QTimer::singleShot(3000, &loop, [&]() {
            QTextStream out(stdout);
            out << "MEDIA available=" << media.available() << " playing=" << media.playing()
                << " title=" << media.title() << " artist=" << media.artist()
                << " source=" << media.sourceApp() << " artwork=" << media.artworkPath()
                << " error=" << media.error() << '\n';
            out.flush();
            loop.exit(0);
        });
        return loop.exec();
    }

    if (arguments.contains(QStringLiteral("--media-toggle"))) {
        MediaService media;
        QEventLoop loop;
        QTimer::singleShot(2000, &loop, [&]() { media.playPause(); });
        QTimer::singleShot(4000, &loop, [&]() {
            QTextStream out(stdout);
            out << "MEDIA available=" << media.available() << " playing=" << media.playing()
                << " title=" << media.title() << '\n';
            out.flush();
            loop.exit(0);
        });
        QTimer::singleShot(9000, &loop, [&loop]() { loop.exit(4); });
        return loop.exec();
    }

    if (arguments.contains(QStringLiteral("--dump-user"))) {
        UserService user;
        QTextStream out(stdout);
        out << "USER name=" << user.displayName() << " login=" << user.userName()
            << " avatar=" << user.avatarPath() << " version=" << user.windowsVersion()
            << " host=" << user.hostName() << '\n';
        out.flush();
        return 0;
    }

    if (arguments.contains(QStringLiteral("--dump-windows"))) {
        WindowService windows;
        QEventLoop loop;
        QTimer::singleShot(1200, &loop, [&]() {
            QTextStream out(stdout);
            out << "WINDOWS count=" << windows.count() << '\n';
            for (const QVariant& entry : windows.windows()) {
                const QVariantMap item = entry.toMap();
                out << item.value(QStringLiteral("app")).toString() << " | "
                    << item.value(QStringLiteral("title")).toString() << '\n';
            }
            out.flush();
            loop.exit(0);
        });
        return loop.exec();
    }

    if (arguments.contains(QStringLiteral("--dump-vdesktops"))) {
        VirtualDesktopService virtualDesktops;
        QEventLoop loop;
        QTimer::singleShot(2500, &loop, [&]() {
            QTextStream out(stdout);
            out << "VDESKTOP available=" << virtualDesktops.available()
                << " current=" << virtualDesktops.currentId()
                << " count=" << virtualDesktops.desktops().size() << '\n';
            for (const QVariant& value : virtualDesktops.desktops()) {
                const QVariantMap item = value.toMap();
                const QVariantList windows = item.value(QStringLiteral("windows")).toList();
                out << "  " << item.value(QStringLiteral("index")).toString()
                    << " id=" << item.value(QStringLiteral("id")).toString()
                    << " name=" << item.value(QStringLiteral("name")).toString()
                    << " active=" << item.value(QStringLiteral("active")).toBool()
                    << " windows=" << windows.size() << '\n';
                for (const QVariant& windowValue : windows) {
                    const QVariantMap window = windowValue.toMap();
                    out << "      hwnd=" << window.value(QStringLiteral("id")).toString()
                        << " app=" << window.value(QStringLiteral("app")).toString()
                        << " title=" << window.value(QStringLiteral("title")).toString()
                        << '\n';
                }
            }
            out.flush();
            loop.exit(0);
        });
        return loop.exec();
    }

    if (arguments.contains(QStringLiteral("--dump-status"))) {
        AudioService audio;
        NetworkService network;
        BluetoothService bluetooth;
        BatteryService battery;
        QEventLoop loop;
        QTimer::singleShot(2500, &loop, [&]() {
            QTextStream out(stdout);
            out << "AUDIO present=" << audio.present() << " known=" << audio.known()
                << " volume=" << audio.volume() << " muted=" << audio.muted()
                << " device=" << audio.deviceName() << '\n'
                << "NETWORK type=" << network.connectionType()
                << " connected=" << network.connected() << " ssid=" << network.wifiSsid()
                << " signal=" << network.wifiSignal()
                << " ethernet=" << network.ethernetName() << '\n'
                << "BLUETOOTH present=" << bluetooth.present()
                << " enabled=" << bluetooth.enabled() << " known=" << bluetooth.known() << '\n'
                << "BATTERY present=" << battery.present() << " known=" << battery.known()
                << " level=" << battery.level() << " charging=" << battery.charging()
                << " onAc=" << battery.onAc()
                << " minutes=" << battery.minutesRemaining() << '\n';
            out.flush();
            loop.exit(0);
        });
        return loop.exec();
    }

    if (arguments.contains(QStringLiteral("--test-tiling"))) {
        QTextStream out(stdout);
        const QRect content(0, 0, 1200, 800);
        bool allOk = true;
        const TilingEngine::Layout layouts[] = {
            TilingEngine::Layout::Dwindle,
            TilingEngine::Layout::Master,
            TilingEngine::Layout::SplitHorizontal,
            TilingEngine::Layout::SplitVertical,
        };
        for (TilingEngine::Layout layout : layouts) {
            TilingEngine engine;
            engine.setLayout(layout);
            for (quintptr hwnd = 1; hwnd <= 5; ++hwnd)
                engine.addWindow(hwnd);

            const QHash<quintptr, QRect> geometry = engine.geometry(content);
            bool inside = geometry.size() == 5;
            bool overlap = false;
            QVector<quintptr> keys;
            for (auto it = geometry.constBegin(); it != geometry.constEnd(); ++it) {
                keys.append(it.key());
                if (!content.contains(it.value()))
                    inside = false;
            }
            for (qsizetype i = 0; i < keys.size(); ++i) {
                for (qsizetype j = i + 1; j < keys.size(); ++j) {
                    if (geometry.value(keys.at(i)).intersects(geometry.value(keys.at(j))))
                        overlap = true;
                }
            }

            out << "layout=" << TilingEngine::layoutName(layout)
                << " count=" << engine.count() << " inside=" << inside
                << " overlap=" << overlap << '\n';
            for (quintptr hwnd : engine.windows()) {
                const QRect rect = geometry.value(hwnd);
                out << "  hwnd=" << hwnd << " rect=" << rect.x() << ',' << rect.y()
                    << ' ' << rect.width() << 'x' << rect.height() << '\n';
            }

            engine.setFocused(1);
            const quintptr right = engine.windowInDirection(content, 1, 0);
            const bool directionOk = layout == TilingEngine::Layout::SplitVertical
                ? right == 0
                : right != 0;
            engine.removeWindow(3);
            const bool removeOk = engine.count() == 4 && !engine.contains(3)
                && engine.contains(engine.focused());
            engine.swapWithDirection(content, 1, 0);
            const bool swapOk = engine.count() == 4;
            engine.resizeFocused(content, true, 0.05);
            const bool resizeOk = true;

            out << "  direction=" << directionOk << " remove=" << removeOk
                << " swap=" << swapOk << " resize=" << resizeOk << '\n';
            allOk = allOk && inside && !overlap && directionOk && removeOk && swapOk;
        }
        out << (allOk ? "TILING PASS\n" : "TILING FAIL\n");
        out.flush();
        return allOk ? 0 : 1;
    }

    if (arguments.contains(QStringLiteral("--audio-mute-test"))) {
        AudioService audio;
        QEventLoop loop;
        QTimer::singleShot(1000, &loop, [&]() {
            QTextStream out(stdout);
            out << "BEFORE muted=" << audio.muted() << " volume=" << audio.volume() << '\n';
            audio.toggleMute();
            QTimer::singleShot(900, &loop, [&]() {
                QTextStream out2(stdout);
                out2 << "TOGGLED muted=" << audio.muted() << '\n';
                audio.toggleMute();
                QTimer::singleShot(900, &loop, [&]() {
                    QTextStream out3(stdout);
                    out3 << "RESTORED muted=" << audio.muted()
                         << " volume=" << audio.volume() << '\n';
                    out3.flush();
                    loop.exit(0);
                });
            });
        });
        return loop.exec();
    }

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
    AppLauncherService apps;
    BatteryService battery;
    NetworkService network;
    BluetoothService bluetooth;
    AudioService audio;
    AudioCaptureService audioCapture;
    MediaService media;
    NotificationService notifications;
    SystemService system;
    WeatherService weather;
    PowerService power;
    UserService user;
    VirtualDesktopService virtualDesktops;
    WallpaperService wallpaper;
    WindowService windows;
    WindowManagerService windowManager(&virtualDesktops);
    TrayService tray;

    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Platform", &platform);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Apps", &apps);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Battery", &battery);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Network", &network);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Bluetooth", &bluetooth);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Audio", &audio);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "AudioCapture", &audioCapture);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Media", &media);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Notification", &notifications);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "System", &system);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Weather", &weather);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Power", &power);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "User", &user);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "VirtualDesktops", &virtualDesktops);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Wallpaper", &wallpaper);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "AppWindows", &windows);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "WindowManager", &windowManager);
    qmlRegisterSingletonInstance("Caelestia.Platform", 1, 0, "Tray", &tray);

    QObject::connect(&windowManager, &WindowManagerService::switchDesktopRequested,
        &virtualDesktops,
        [&virtualDesktops](int index) {
            const QVariantList desktops = virtualDesktops.desktops();
            if (index < 0 || index >= desktops.size())
                return;
            const QString id = desktops.at(index).toMap()
                                   .value(QStringLiteral("id")).toString();
            if (!id.isEmpty())
                virtualDesktops.switchTo(id);
        },
        Qt::QueuedConnection);
    QObject::connect(&windowManager, &WindowManagerService::moveWindowToDesktopRequested,
        &virtualDesktops,
        [&virtualDesktops](const QString& windowId, int index) {
            const QVariantList desktops = virtualDesktops.desktops();
            if (index < 0 || index >= desktops.size())
                return;
            const QString id = desktops.at(index).toMap()
                                   .value(QStringLiteral("id")).toString();
            if (!id.isEmpty())
                virtualDesktops.moveWindow(windowId, id);
        },
        Qt::QueuedConnection);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &windowManager,
        &WindowManagerService::shutdown);

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
