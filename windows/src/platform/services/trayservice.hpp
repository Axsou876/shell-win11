#pragma once

#include <QAbstractNativeEventFilter>
#include <QObject>

#include <memory>

class QWindow;

class TrayService final : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

    Q_PROPERTY(bool available READ available NOTIFY availableChanged)

public:
    explicit TrayService(QObject* parent = nullptr);
    ~TrayService() override;

    [[nodiscard]] bool available() const;

    void attach();
    Q_INVOKABLE void showMenu();
    Q_INVOKABLE void focusNotificationArea();

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    void availableChanged();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool m_available = false;
};
