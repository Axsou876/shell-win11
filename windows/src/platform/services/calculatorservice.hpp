#pragma once

#include <QJSEngine>
#include <QObject>
#include <QString>
#include <QVariantMap>

// The Linux shell pipes ">calc <expression>" into Qalculator (libqalculate).
// Windows has no equivalent library, so expressions run through QJSEngine with
// only the Math helpers installed as globals. A bare QJSEngine has no Qt or
// DOM bindings, and the input is filtered down to an arithmetic alphabet
// before evaluation, which keeps statements out of the engine.
class CalculatorService final : public QObject {
    Q_OBJECT

public:
    explicit CalculatorService(QObject* parent = nullptr);

    // Returns { ok, text, error }. text is the formatted result when ok;
    // error explains the rejection otherwise.
    Q_INVOKABLE QVariantMap evaluate(const QString& expression);

private:
    QJSEngine m_engine;
};
