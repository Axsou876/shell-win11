#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

class AppSearchModel;

// One application discovered from the Windows Start Menu.
struct AppEntry {
    QString id;
    QString name;
    QString description;
    QString launchPath; // .lnk or shell:AppsFolder\AUMID
    QString target; // resolved target, used for the icon and duplicate detection
    QString arguments;
    QString workingDirectory;
    QString iconSource;
    QString iconPath;
    qint64 lastUsed = 0;
    int frequency = 0;
    bool favorite = false;
};

class AppListModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        IconRole,
        FavoriteRole,
        RecentRole
    };

    explicit AppListModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] const QList<AppEntry>& entries() const { return m_entries; }
    void setEntries(QList<AppEntry> entries);
    void setIconPath(const QString& id, const QString& path);
    void setFavorite(const QString& id, bool favorite);
    void touch(const QString& id);

private:
    int indexOf(const QString& id) const;
    QList<AppEntry> m_entries;
    QHash<QString, int> m_index;
};

// Sorted/filtered view over AppListModel. Fuzzy matching is a small, local
// scoring function: prefix > substring > subsequence, with favourites and
// usage frequency as tie-breakers. No command text is ever executed.
class AppSearchModel final : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)

public:
    explicit AppSearchModel(AppListModel* source, QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] int count() const { return m_rows.size(); }
    [[nodiscard]] QString query() const { return m_query; }
    void setQuery(const QString& query);

    void rebuild();

    [[nodiscard]] QString idAt(int row) const;
    [[nodiscard]] QString nameAt(int row) const;

signals:
    void countChanged();
    void queryChanged();

private:
    AppListModel* m_source = nullptr;
    QString m_query;
    QList<int> m_rows;
    QList<int> m_scores;
};

class AppLauncherService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QObject* searchModel READ searchModel CONSTANT)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)
    Q_PROPERTY(int resultCount READ resultCount NOTIFY resultCountChanged)
    Q_PROPERTY(QStringList favoriteIds READ favoriteIds NOTIFY favoritesChanged)
    Q_PROPERTY(QVariantList favoriteApps READ favoriteApps NOTIFY favoritesChanged)
    Q_PROPERTY(QVariantList recentApps READ recentApps NOTIFY recentsChanged)

public:
    explicit AppLauncherService(QObject* parent = nullptr);
    ~AppLauncherService() override;

    [[nodiscard]] QObject* searchModel() const;
    [[nodiscard]] bool scanning() const { return m_scanning; }
    [[nodiscard]] bool ready() const { return m_ready; }
    [[nodiscard]] QString error() const { return m_error; }
    [[nodiscard]] int totalCount() const;
    [[nodiscard]] int resultCount() const;
    [[nodiscard]] QStringList favoriteIds() const { return m_favorites; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setQuery(const QString& query);
    Q_INVOKABLE void launch(int searchRow);
    Q_INVOKABLE void launchById(const QString& id);
    Q_INVOKABLE void toggleFavorite(int searchRow);
    Q_INVOKABLE void clearQuery();
    [[nodiscard]] QVariantList favoriteApps() const;
    [[nodiscard]] QVariantList recentApps() const;

    // CLI helpers: known applications only, never a shell command string.
    [[nodiscard]] QStringList knownAppNames() const;
    [[nodiscard]] bool launchByName(const QString& name, QString* errorMessage = nullptr);

signals:
    void scanningChanged();
    void readyChanged();
    void errorChanged();
    void totalCountChanged();
    void resultCountChanged();
    void favoritesChanged();
    void recentsChanged();
    void launchSucceeded(const QString& id, const QString& name);
    void launchFailed(const QString& id, const QString& message);
    void iconChanged();

private:
    void applyEntries(QList<AppEntry> entries);
    void loadSettings();
    void saveStats();
    void startIconQueue();
    void extractNextIcons();
    void setError(const QString& error);
    void setScanning(bool scanning);
    void applyFavorite(const QString& id, bool favorite);
    void launchEntry(const AppEntry& entry);

    AppListModel* m_apps = nullptr;
    AppSearchModel* m_search = nullptr;
    QStringList m_favorites;
    bool m_scanning = false;
    bool m_ready = false;
    QString m_error;
    quint64 m_scanGeneration = 0;
    QStringList m_iconQueue;
    int m_iconCursor = 0;
    class QTimer* m_iconTimer = nullptr;
};

