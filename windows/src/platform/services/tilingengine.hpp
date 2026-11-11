#pragma once

#include <QHash>
#include <QRect>
#include <QString>
#include <QVariantMap>
#include <QVector>

#include <memory>

// Pure tiling layout engine. It manipulates HWND values but never calls Win32;
// geometry is computed for a supplied content rectangle. This keeps the layout
// logic testable without moving real windows.
class TilingEngine {
public:
    enum class Layout {
        Dwindle,
        Master,
        SplitHorizontal,
        SplitVertical,
    };

    TilingEngine();
    ~TilingEngine();

    TilingEngine(const TilingEngine&) = delete;
    TilingEngine& operator=(const TilingEngine&) = delete;
    TilingEngine(TilingEngine&&) noexcept;
    TilingEngine& operator=(TilingEngine&&) noexcept;

    static QString layoutName(Layout layout);
    static Layout layoutFromName(const QString& name);

    void setLayout(Layout layout);
    [[nodiscard]] Layout layout() const { return m_layout; }
    void setInnerGap(int gap);
    [[nodiscard]] int innerGap() const { return m_innerGap; }
    void setMasterRatio(qreal ratio);
    [[nodiscard]] qreal masterRatio() const { return m_masterRatio; }

    void addWindow(quintptr hwnd, bool focus = true);
    void removeWindow(quintptr hwnd);
    [[nodiscard]] bool contains(quintptr hwnd) const;
    void setFocused(quintptr hwnd);
    [[nodiscard]] quintptr focused() const { return m_focused; }
    [[nodiscard]] QVector<quintptr> windows() const;
    [[nodiscard]] int count() const { return static_cast<int>(windows().size()); }
    [[nodiscard]] bool empty() const { return !m_root; }
    void clear();

    [[nodiscard]] QHash<quintptr, QRect> geometry(const QRect& content) const;
    [[nodiscard]] QRect geometryFor(const QRect& content, quintptr hwnd) const;
    [[nodiscard]] quintptr windowInDirection(
        const QRect& content, int dx, int dy) const;
    [[nodiscard]] quintptr nextWindow() const;
    [[nodiscard]] quintptr previousWindow() const;
    bool swapWithDirection(const QRect& content, int dx, int dy);
    bool promoteMaster();
    bool resizeFocused(const QRect& content, bool horizontal, qreal delta);

    [[nodiscard]] QVariantMap save() const;
    void restore(const QVariantMap& state);

private:
    struct Node {
        bool leaf = true;
        quintptr hwnd = 0;
        bool verticalSplit = true; // true: first/second side by side
        qreal ratio = 0.5;
        std::unique_ptr<Node> first;
        std::unique_ptr<Node> second;
    };

    static Node* findLeaf(Node* node, quintptr hwnd);
    static const Node* findLeaf(const Node* node, quintptr hwnd);
    static Node* findLeafWithDepth(Node* node, quintptr hwnd, int depth, int* foundDepth);
    static bool collectPath(Node* node, const Node* target, QVector<Node*>& path);
    static void collectLeaves(const Node* node, QVector<quintptr>& out);
    static quintptr firstLeaf(const Node* node);
    static quintptr lastLeaf(const Node* node);
    static void collectGeometry(
        const Node* node, const QRect& rect, int gap, QHash<quintptr, QRect>& out);
    static void swapLeafWindows(Node* node, quintptr a, quintptr b);
    static QVariantMap nodeToVariant(const Node* node);
    static std::unique_ptr<Node> nodeFromVariant(const QVariantMap& map);

    [[nodiscard]] QVector<quintptr> displayOrder() const;
    void normalizeMasterOrder();
    [[nodiscard]] bool isMasterLayout() const;
    [[nodiscard]] bool isSplitLayout() const;

    std::unique_ptr<Node> m_root;
    QVector<quintptr> m_order;
    quintptr m_focused = 0;
    Layout m_layout = Layout::Dwindle;
    int m_innerGap = 5;
    qreal m_masterRatio = 0.6;
};
