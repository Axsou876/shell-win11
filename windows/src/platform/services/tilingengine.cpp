#include "tilingengine.hpp"

#include <QVariantList>

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <utility>

namespace {
constexpr qreal kMinRatio = 0.15;
constexpr qreal kMaxRatio = 0.85;
}

TilingEngine::TilingEngine() = default;
TilingEngine::~TilingEngine() = default;
TilingEngine::TilingEngine(TilingEngine&&) noexcept = default;
TilingEngine& TilingEngine::operator=(TilingEngine&&) noexcept = default;

QString TilingEngine::layoutName(Layout layout)
{
    switch (layout) {
    case Layout::Dwindle:
        return QStringLiteral("dwindle");
    case Layout::Master:
        return QStringLiteral("master");
    case Layout::SplitHorizontal:
        return QStringLiteral("split-horizontal");
    case Layout::SplitVertical:
        return QStringLiteral("split-vertical");
    }
    return QStringLiteral("dwindle");
}

TilingEngine::Layout TilingEngine::layoutFromName(const QString& name)
{
    const QString value = name.trimmed().toLower();
    if (value == QStringLiteral("master"))
        return Layout::Master;
    if (value == QStringLiteral("split-horizontal") || value == QStringLiteral("splith")
        || value == QStringLiteral("split-h")) {
        return Layout::SplitHorizontal;
    }
    if (value == QStringLiteral("split-vertical") || value == QStringLiteral("splitv")
        || value == QStringLiteral("split-v")) {
        return Layout::SplitVertical;
    }
    return Layout::Dwindle;
}

void TilingEngine::setLayout(Layout layout)
{
    if (layout == m_layout)
        return;
    if (m_layout == Layout::Dwindle
        && (layout == Layout::Master || layout == Layout::SplitHorizontal
            || layout == Layout::SplitVertical))
        normalizeMasterOrder();
    m_layout = layout;
}

void TilingEngine::setInnerGap(int gap)
{
    m_innerGap = qBound(0, gap, 64);
}

void TilingEngine::setMasterRatio(qreal ratio)
{
    m_masterRatio = qBound(0.25, ratio, 0.8);
}

bool TilingEngine::isMasterLayout() const
{
    return m_layout == Layout::Master;
}

bool TilingEngine::isSplitLayout() const
{
    return m_layout == Layout::SplitHorizontal || m_layout == Layout::SplitVertical;
}

TilingEngine::Node* TilingEngine::findLeaf(Node* node, quintptr hwnd)
{
    if (!node)
        return nullptr;
    if (node->leaf)
        return node->hwnd == hwnd ? node : nullptr;
    if (Node* found = findLeaf(node->first.get(), hwnd))
        return found;
    return findLeaf(node->second.get(), hwnd);
}

const TilingEngine::Node* TilingEngine::findLeaf(const Node* node, quintptr hwnd)
{
    if (!node)
        return nullptr;
    if (node->leaf)
        return node->hwnd == hwnd ? node : nullptr;
    if (const Node* found = findLeaf(node->first.get(), hwnd))
        return found;
    return findLeaf(node->second.get(), hwnd);
}

TilingEngine::Node* TilingEngine::findLeafWithDepth(
    Node* node, quintptr hwnd, int depth, int* foundDepth)
{
    if (!node)
        return nullptr;
    if (node->leaf) {
        if (node->hwnd != hwnd)
            return nullptr;
        if (foundDepth)
            *foundDepth = depth;
        return node;
    }
    if (Node* found = findLeafWithDepth(node->first.get(), hwnd, depth + 1, foundDepth))
        return found;
    return findLeafWithDepth(node->second.get(), hwnd, depth + 1, foundDepth);
}

bool TilingEngine::collectPath(Node* node, const Node* target, QVector<Node*>& path)
{
    if (!node)
        return false;
    path.append(node);
    if (node == target)
        return true;
    if (!node->leaf) {
        if (collectPath(node->first.get(), target, path))
            return true;
        if (collectPath(node->second.get(), target, path))
            return true;
    }
    path.removeLast();
    return false;
}

void TilingEngine::collectLeaves(const Node* node, QVector<quintptr>& out)
{
    if (!node)
        return;
    if (node->leaf) {
        out.append(node->hwnd);
        return;
    }
    collectLeaves(node->first.get(), out);
    collectLeaves(node->second.get(), out);
}

quintptr TilingEngine::firstLeaf(const Node* node)
{
    if (!node)
        return 0;
    while (!node->leaf)
        node = node->first.get();
    return node->hwnd;
}

quintptr TilingEngine::lastLeaf(const Node* node)
{
    if (!node)
        return 0;
    while (!node->leaf)
        node = node->second.get();
    return node->hwnd;
}

void TilingEngine::addWindow(quintptr hwnd, bool focus)
{
    if (!hwnd || contains(hwnd))
        return;

    if (!m_root) {
        m_root = std::make_unique<Node>();
        m_root->hwnd = hwnd;
        m_order.append(hwnd);
        if (focus)
            m_focused = hwnd;
        return;
    }

    Node* target = m_focused ? findLeaf(m_root.get(), m_focused) : nullptr;
    if (!target)
        target = findLeaf(m_root.get(), lastLeaf(m_root.get()));
    if (!target) {
        m_order.append(hwnd);
        if (focus)
            m_focused = hwnd;
        return;
    }

    int depth = 0;
    findLeafWithDepth(m_root.get(), target->hwnd, 0, &depth);

    const quintptr previous = target->hwnd;
    target->leaf = false;
    target->hwnd = 0;
    target->verticalSplit = (depth % 2) == 0;
    target->ratio = 0.5;
    target->first = std::make_unique<Node>();
    target->first->hwnd = previous;
    target->second = std::make_unique<Node>();
    target->second->hwnd = hwnd;

    m_order.append(hwnd);
    if (focus)
        m_focused = hwnd;
}

void TilingEngine::removeWindow(quintptr hwnd)
{
    if (!hwnd || !m_root)
        return;

    const QVector<quintptr> before = displayOrder();
    const qsizetype index = before.indexOf(hwnd);
    quintptr nextFocus = m_focused;
    if (index >= 0) {
        if (before.size() > 1)
            nextFocus = before.at((index + 1) % before.size());
        else
            nextFocus = 0;
    }

    std::function<bool(std::unique_ptr<Node>&)> removeFrom
        = [&](std::unique_ptr<Node>& node) -> bool {
        if (!node)
            return false;
        if (node->leaf) {
            if (node->hwnd != hwnd)
                return false;
            node.reset();
            return true;
        }
        if (removeFrom(node->first)) {
            if (!node->first)
                node = std::move(node->second);
            return true;
        }
        if (removeFrom(node->second)) {
            if (!node->second)
                node = std::move(node->first);
            return true;
        }
        return false;
    };

    removeFrom(m_root);
    m_order.removeAll(hwnd);
    if (!m_root) {
        m_focused = 0;
        m_order.clear();
        return;
    }
    if (m_focused == hwnd || !contains(m_focused))
        m_focused = contains(nextFocus) ? nextFocus : firstLeaf(m_root.get());
}

bool TilingEngine::contains(quintptr hwnd) const
{
    return findLeaf(m_root.get(), hwnd) != nullptr;
}

void TilingEngine::setFocused(quintptr hwnd)
{
    if (contains(hwnd))
        m_focused = hwnd;
}

void TilingEngine::clear()
{
    m_root.reset();
    m_order.clear();
    m_focused = 0;
}

QVector<quintptr> TilingEngine::displayOrder() const
{
    if (!m_root)
        return {};
    if (m_layout == Layout::Dwindle) {
        QVector<quintptr> leaves;
        collectLeaves(m_root.get(), leaves);
        return leaves;
    }
    QVector<quintptr> order;
    order.reserve(m_order.size());
    for (quintptr hwnd : m_order) {
        if (contains(hwnd))
            order.append(hwnd);
    }
    QVector<quintptr> leaves;
    collectLeaves(m_root.get(), leaves);
    if (order.size() != leaves.size()) {
        for (quintptr hwnd : leaves) {
            if (!order.contains(hwnd))
                order.append(hwnd);
        }
    }
    return order;
}

void TilingEngine::normalizeMasterOrder()
{
    m_order = displayOrder();
}

QVector<quintptr> TilingEngine::windows() const
{
    return displayOrder();
}

void TilingEngine::collectGeometry(const Node* node, const QRect& rect, int gap,
    QHash<quintptr, QRect>& out)
{
    if (!node || rect.width() <= 0 || rect.height() <= 0)
        return;
    if (node->leaf) {
        out.insert(node->hwnd, rect);
        return;
    }

    const int effectiveGap = qMin(gap, qMax(0, (node->verticalSplit ? rect.width()
                                                                   : rect.height())
            - 2));
    if (node->verticalSplit) {
        const int usable = qMax(2, rect.width() - effectiveGap);
        const int firstWidth = qBound(1, qRound(usable * node->ratio), usable - 1);
        const QRect first(rect.x(), rect.y(), firstWidth, rect.height());
        const QRect second(rect.x() + firstWidth + effectiveGap, rect.y(),
            rect.width() - firstWidth - effectiveGap, rect.height());
        collectGeometry(node->first.get(), first, gap, out);
        collectGeometry(node->second.get(), second, gap, out);
    } else {
        const int usable = qMax(2, rect.height() - effectiveGap);
        const int firstHeight = qBound(1, qRound(usable * node->ratio), usable - 1);
        const QRect first(rect.x(), rect.y(), rect.width(), firstHeight);
        const QRect second(rect.x(), rect.y() + firstHeight + effectiveGap,
            rect.width(), rect.height() - firstHeight - effectiveGap);
        collectGeometry(node->first.get(), first, gap, out);
        collectGeometry(node->second.get(), second, gap, out);
    }
}

QHash<quintptr, QRect> TilingEngine::geometry(const QRect& content) const
{
    QHash<quintptr, QRect> result;
    if (!m_root || content.width() <= 0 || content.height() <= 0)
        return result;

    if (m_layout == Layout::Dwindle) {
        collectGeometry(m_root.get(), content, m_innerGap, result);
        return result;
    }

    const QVector<quintptr> order = displayOrder();
    if (order.isEmpty())
        return result;
    if (order.size() == 1) {
        result.insert(order.first(), content);
        return result;
    }

    const int gap = qMin(m_innerGap, qMax(0, qMin(content.width(), content.height()) - 2));
    const int count = static_cast<int>(order.size());
    if (m_layout == Layout::SplitHorizontal) {
        const int usable = qMax(2, content.width() - gap * (count - 1));
        int x = content.x();
        for (int i = 0; i < count; ++i) {
            const bool last = i == count - 1;
            const int width = last ? qMax(1, content.right() - x + 1)
                                   : qMax(1, usable / count);
            result.insert(order.at(i), QRect(x, content.y(), width, content.height()));
            x += width + gap;
        }
        return result;
    }
    if (m_layout == Layout::SplitVertical) {
        const int usable = qMax(2, content.height() - gap * (count - 1));
        int y = content.y();
        for (int i = 0; i < count; ++i) {
            const bool last = i == count - 1;
            const int height = last ? qMax(1, content.bottom() - y + 1)
                                    : qMax(1, usable / count);
            result.insert(order.at(i), QRect(content.x(), y, content.width(), height));
            y += height + gap;
        }
        return result;
    }

    // Master: first window on the left, the rest stacked on the right.
    const int usableWidth = qMax(2, content.width() - gap);
    const int masterWidth = qBound(1, qRound(usableWidth * m_masterRatio),
        usableWidth - 1);
    const QRect master(content.x(), content.y(), masterWidth, content.height());
    result.insert(order.first(), master);

    const int stackCount = static_cast<int>(order.size()) - 1;
    const int usableHeight = qMax(2, content.height() - gap * (stackCount - 1));
    int y = content.y();
    const int stackX = content.x() + masterWidth + gap;
    const int stackWidth = content.width() - masterWidth - gap;
    for (int i = 1; i < count; ++i) {
        const bool last = i == count - 1;
        const int height = last ? qMax(1, content.bottom() - y + 1)
                                : qMax(1, usableHeight / stackCount);
        result.insert(order.at(i), QRect(stackX, y, stackWidth, height));
        y += height + gap;
    }
    return result;
}

QRect TilingEngine::geometryFor(const QRect& content, quintptr hwnd) const
{
    return geometry(content).value(hwnd);
}

quintptr TilingEngine::windowInDirection(const QRect& content, int dx, int dy) const
{
    const QHash<quintptr, QRect> rects = geometry(content);
    if (!rects.contains(m_focused) || (dx == 0 && dy == 0))
        return 0;

    const QRect active = rects.value(m_focused);
    const QPoint activeCenter = active.center();
    quintptr best = 0;
    qreal bestScore = std::numeric_limits<qreal>::max();

    for (auto it = rects.constBegin(); it != rects.constEnd(); ++it) {
        if (it.key() == m_focused)
            continue;
        const QRect candidate = it.value();
        const QPoint center = candidate.center();
        bool inDirection = false;
        qreal primary = 0;
        qreal perpendicular = 0;

        if (dx < 0) {
            inDirection = center.x() < activeCenter.x();
            primary = qMax(0, active.left() - candidate.right());
            perpendicular = qAbs(center.y() - activeCenter.y());
        } else if (dx > 0) {
            inDirection = center.x() > activeCenter.x();
            primary = qMax(0, candidate.left() - active.right());
            perpendicular = qAbs(center.y() - activeCenter.y());
        } else if (dy < 0) {
            inDirection = center.y() < activeCenter.y();
            primary = qMax(0, active.top() - candidate.bottom());
            perpendicular = qAbs(center.x() - activeCenter.x());
        } else if (dy > 0) {
            inDirection = center.y() > activeCenter.y();
            primary = qMax(0, candidate.top() - active.bottom());
            perpendicular = qAbs(center.x() - activeCenter.x());
        }

        if (!inDirection)
            continue;

        const qreal score = primary * 2.0 + perpendicular;
        if (score < bestScore) {
            bestScore = score;
            best = it.key();
        }
    }
    return best;
}

quintptr TilingEngine::nextWindow() const
{
    const QVector<quintptr> order = displayOrder();
    if (order.isEmpty())
        return 0;
    const qsizetype index = order.indexOf(m_focused);
    if (index < 0)
        return order.first();
    return order.at((index + 1) % order.size());
}

quintptr TilingEngine::previousWindow() const
{
    const QVector<quintptr> order = displayOrder();
    if (order.isEmpty())
        return 0;
    const qsizetype index = order.indexOf(m_focused);
    if (index < 0)
        return order.first();
    return order.at((index - 1 + order.size()) % order.size());
}

void TilingEngine::swapLeafWindows(Node* node, quintptr a, quintptr b)
{
    if (!node)
        return;
    if (node->leaf) {
        if (node->hwnd == a)
            node->hwnd = b;
        else if (node->hwnd == b)
            node->hwnd = a;
        return;
    }
    swapLeafWindows(node->first.get(), a, b);
    swapLeafWindows(node->second.get(), a, b);
}

bool TilingEngine::swapWithDirection(const QRect& content, int dx, int dy)
{
    const quintptr neighbor = windowInDirection(content, dx, dy);
    if (!neighbor || !m_focused)
        return false;

    swapLeafWindows(m_root.get(), m_focused, neighbor);
    const qsizetype a = m_order.indexOf(m_focused);
    const qsizetype b = m_order.indexOf(neighbor);
    if (a >= 0 && b >= 0)
        m_order.swapItemsAt(a, b);
    return true;
}

bool TilingEngine::promoteMaster()
{
    if (m_layout != Layout::Master || !m_focused)
        return false;
    const qsizetype index = m_order.indexOf(m_focused);
    if (index <= 0)
        return false;
    m_order.move(index, 0);
    return true;
}

bool TilingEngine::resizeFocused(const QRect& content, bool horizontal, qreal delta)
{
    Q_UNUSED(content)
    if (!m_root || !m_focused)
        return false;

    if (m_layout == Layout::Master) {
        if (!horizontal)
            return false;
        setMasterRatio(m_masterRatio + delta);
        return true;
    }
    if (m_layout != Layout::Dwindle)
        return false;

    Node* leaf = findLeaf(m_root.get(), m_focused);
    if (!leaf)
        return false;
    QVector<Node*> path;
    if (!collectPath(m_root.get(), leaf, path))
        return false;

    for (qsizetype i = path.size() - 2; i >= 0; --i) {
        Node* parent = path.at(i);
        if (parent->leaf)
            continue;
        const bool controlsWidth = parent->verticalSplit;
        if (controlsWidth != horizontal)
            continue;
        parent->ratio = qBound(kMinRatio, parent->ratio + delta, kMaxRatio);
        return true;
    }
    return false;
}

QVariantMap TilingEngine::nodeToVariant(const Node* node)
{
    QVariantMap map;
    if (!node)
        return map;
    if (node->leaf) {
        map.insert(QStringLiteral("hwnd"), QString::number(node->hwnd));
        return map;
    }
    map.insert(QStringLiteral("vertical"), node->verticalSplit);
    map.insert(QStringLiteral("ratio"), node->ratio);
    map.insert(QStringLiteral("first"), nodeToVariant(node->first.get()));
    map.insert(QStringLiteral("second"), nodeToVariant(node->second.get()));
    return map;
}

std::unique_ptr<TilingEngine::Node> TilingEngine::nodeFromVariant(const QVariantMap& map)
{
    if (map.isEmpty())
        return nullptr;
    auto node = std::make_unique<Node>();
    if (map.contains(QStringLiteral("hwnd"))) {
        node->hwnd = map.value(QStringLiteral("hwnd")).toString().toULongLong();
        if (!node->hwnd)
            return nullptr;
        return node;
    }
    node->leaf = false;
    node->verticalSplit = map.value(QStringLiteral("vertical"), true).toBool();
    node->ratio = qBound(kMinRatio, map.value(QStringLiteral("ratio"), 0.5).toDouble(),
        kMaxRatio);
    node->first = nodeFromVariant(map.value(QStringLiteral("first")).toMap());
    node->second = nodeFromVariant(map.value(QStringLiteral("second")).toMap());
    if (!node->first || !node->second)
        return nullptr;
    return node;
}

QVariantMap TilingEngine::save() const
{
    QVariantMap state;
    state.insert(QStringLiteral("layout"), static_cast<int>(m_layout));
    state.insert(QStringLiteral("innerGap"), m_innerGap);
    state.insert(QStringLiteral("masterRatio"), m_masterRatio);
    state.insert(QStringLiteral("focused"), QString::number(m_focused));
    QVariantList order;
    order.reserve(m_order.size());
    for (quintptr hwnd : m_order)
        order.append(QString::number(hwnd));
    state.insert(QStringLiteral("order"), order);
    if (m_root)
        state.insert(QStringLiteral("tree"), nodeToVariant(m_root.get()));
    return state;
}

void TilingEngine::restore(const QVariantMap& state)
{
    clear();
    if (state.isEmpty())
        return;

    m_layout = static_cast<Layout>(qBound(0, state.value(QStringLiteral("layout"),
                                                        static_cast<int>(Layout::Dwindle))
                                                 .toInt(),
        3));
    m_innerGap = qBound(0, state.value(QStringLiteral("innerGap"), 5).toInt(), 64);
    setMasterRatio(state.value(QStringLiteral("masterRatio"), 0.6).toDouble());

    m_root = nodeFromVariant(state.value(QStringLiteral("tree")).toMap());
    if (!m_root)
        return;

    QVector<quintptr> leaves;
    collectLeaves(m_root.get(), leaves);

    const QVariantList savedOrder = state.value(QStringLiteral("order")).toList();
    for (const QVariant& value : savedOrder) {
        const quintptr hwnd = value.toString().toULongLong();
        if (contains(hwnd) && !m_order.contains(hwnd))
            m_order.append(hwnd);
    }
    for (quintptr hwnd : leaves) {
        if (!m_order.contains(hwnd))
            m_order.append(hwnd);
    }

    const quintptr savedFocus = state.value(QStringLiteral("focused")).toString().toULongLong();
    m_focused = contains(savedFocus) ? savedFocus : firstLeaf(m_root.get());
}
