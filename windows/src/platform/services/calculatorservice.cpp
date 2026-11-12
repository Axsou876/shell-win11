#include "calculatorservice.hpp"

#include <cmath>

namespace {
// Long enough for a real expression, short enough that a paste of a log line
// is rejected instead of evaluated.
constexpr int kMaxLength = 256;

// Everything QJSEngine is allowed to see: digits, the identifiers used for
// functions and constants, and arithmetic operators. Parentheses are needed
// for the "(" + source + ")" wrapper and for grouping. Braces, semicolons,
// quotes and assignment are absent, so no statement can be formed.
bool isArithmetic(const QString& source)
{
    for (const QChar ch : source) {
        if (ch.isLetterOrNumber() || ch.isSpace())
            continue;
        switch (ch.unicode()) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '.':
        case ',':
        case '(':
        case ')':
            break;
        default:
            return false;
        }
    }
    return true;
}

QString formatNumber(double value)
{
    // -0 is a valid double but reads as a bug in a result field.
    if (value == 0)
        return QStringLiteral("0");
    // Whole numbers below 2^53 print without a decimal point or exponent,
    // the way a calculator shows them.
    if (std::trunc(value) == value && std::abs(value) < 1e15)
        return QString::number(static_cast<qint64>(value));
    // 12 significant digits hides binary artefacts such as 0.30000000000000004
    // while keeping more precision than the panel can display.
    return QString::number(value, 'g', 12);
}
} // namespace

CalculatorService::CalculatorService(QObject* parent)
    : QObject(parent)
{
    // Short names for what people actually type. "e" only works as an
    // identifier ("2*e"), because "2e" is an incomplete exponent literal.
    m_engine.evaluate(QStringLiteral(
        "var sqrt = Math.sqrt, cbrt = Math.cbrt, abs = Math.abs,"
        " sin = Math.sin, cos = Math.cos, tan = Math.tan,"
        " asin = Math.asin, acos = Math.acos, atan = Math.atan,"
        " atan2 = Math.atan2, log = Math.log, ln = Math.log,"
        " log10 = Math.log10, log2 = Math.log2, exp = Math.exp,"
        " floor = Math.floor, ceil = Math.ceil, round = Math.round,"
        " min = Math.min, max = Math.max, pow = Math.pow,"
        " pi = Math.PI, e = Math.E;"));
}

QVariantMap CalculatorService::evaluate(const QString& expression)
{
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);

    QString source = expression.trimmed();
    if (source.isEmpty()) {
        result.insert(QStringLiteral("error"), QString());
        return result;
    }

    // qalc (and therefore the Linux launcher) accepts these; JavaScript does
    // not, so normalise before it reaches the engine.
    source.replace(QChar(0x00D7), u'*'); // ×
    source.replace(QChar(0x00F7), u'/'); // ÷
    source.replace(QChar(0x2212), u'-'); // −
    source.replace(u'^', QStringLiteral("**"));

    if (source.size() > kMaxLength || !isArithmetic(source)) {
        result.insert(QStringLiteral("error"), tr("Expression non prise en charge."));
        return result;
    }

    // The parentheses make the whole input one expression: without them a
    // trailing operator or a stray token would be parsed as a statement list.
    const QJSValue value = m_engine.evaluate(QStringLiteral("(%1)").arg(source));
    if (value.isError()) {
        result.insert(QStringLiteral("error"), tr("Expression invalide."));
        return result;
    }
    if (!value.isNumber()) {
        result.insert(QStringLiteral("error"), tr("Le résultat n'est pas un nombre."));
        return result;
    }

    const double number = value.toNumber();
    if (!std::isfinite(number)) {
        result.insert(QStringLiteral("error"), tr("Résultat indéfini."));
        return result;
    }

    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("text"), formatNumber(number));
    return result;
}
