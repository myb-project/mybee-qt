#ifndef URLMODEL_H
#define URLMODEL_H

#include <QObject>
#include <QUrl>

class UrlModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl    location READ location WRITE setLocation NOTIFY locationChanged)
    Q_PROPERTY(QString scheme   READ scheme   WRITE setScheme   NOTIFY schemeChanged)
    Q_PROPERTY(QString userName READ userName WRITE setUserName NOTIFY userNameChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(QString host     READ host     WRITE setHost     NOTIFY hostChanged)
    Q_PROPERTY(int     port     READ port     WRITE setPort     NOTIFY portChanged)
    Q_PROPERTY(QString path     READ path     WRITE setPath     NOTIFY pathChanged)
    Q_PROPERTY(QString query    READ query    WRITE setQuery    NOTIFY queryChanged)
    Q_PROPERTY(QString fragment READ fragment WRITE setFragment NOTIFY fragmentChanged)
    Q_PROPERTY(QString text     READ text     NOTIFY textChanged)
    Q_PROPERTY(bool    empty    READ isEmpty  NOTIFY emptyChanged)
    Q_PROPERTY(bool    valid    READ isValid  NOTIFY validChanged)
    Q_PROPERTY(bool    local    READ isLocal  NOTIFY localChanged)
    Q_PROPERTY(bool    remote   READ isRemote NOTIFY remoteChanged)

public:
    enum FormatingOptions {
        None            = QUrl::None,
        RemoveScheme    = QUrl::RemoveScheme,
        RemovePassword  = QUrl::RemovePassword,
        RemoveUserInfo  = QUrl::RemoveUserInfo,
        RemovePort      = QUrl::RemovePort,
        RemoveAuthority = QUrl::RemoveAuthority,
        RemovePath      = QUrl::RemovePath,
        RemoveQuery     = QUrl::RemoveQuery,
        RemoveFragment  = QUrl::RemoveFragment,
        RemoveFilename  = QUrl::RemoveFilename,
        PreferLocalFile = QUrl::PreferLocalFile,
        StripTrailingSlash = QUrl::StripTrailingSlash,
        NormalizePathSegments = QUrl::NormalizePathSegments,
    };
    Q_ENUM(FormatingOptions)

    explicit UrlModel(QObject *parent = nullptr);
    ~UrlModel();

    Q_INVOKABLE static bool isEmptyAt(const QUrl &url);
    Q_INVOKABLE static bool isValidAt(const QUrl &url);
    Q_INVOKABLE static bool isLocalAt(const QUrl &url);
    Q_INVOKABLE static bool isRemoteAt(const QUrl &url);
    Q_INVOKABLE static QString schemeAt(const QUrl &url);
    Q_INVOKABLE static QString authorityAt(const QUrl &url);
    Q_INVOKABLE inline QString authority() const { return authorityAt(url_model); }
    Q_INVOKABLE static QString userNameAt(const QUrl &url);
    Q_INVOKABLE static QString passwordAt(const QUrl &url);
    Q_INVOKABLE static QString hostAt(const QUrl &url);
    Q_INVOKABLE static int portAt(const QUrl &url);
    Q_INVOKABLE static QString pathAt(const QUrl &url);
    Q_INVOKABLE static QString hostPathAt(const QUrl &url);
    Q_INVOKABLE inline QString hostPath() const { return hostPathAt(url_model); }
    Q_INVOKABLE static QString queryAt(const QUrl &url);
    Q_INVOKABLE static QString fragmentAt(const QUrl &url);
    Q_INVOKABLE static QString textAt(const QUrl &url);
    Q_INVOKABLE static QString pathNameAt(const QUrl &url);
    Q_INVOKABLE inline QString pathName() const { return pathNameAt(url_model); }
    Q_INVOKABLE static QString fileNameAt(const QUrl &url);
    Q_INVOKABLE inline QString fileName() const { return fileNameAt(url_model); }
    Q_INVOKABLE static QUrl adjustedAt(const QUrl &url, UrlModel::FormatingOptions opt);
    Q_INVOKABLE inline QUrl adjusted(UrlModel::FormatingOptions opt) const { return adjustedAt(url_model, opt); }
    Q_INVOKABLE inline void clear() { setLocation(QUrl()); }

    inline bool isEmpty() const { return isEmptyAt(url_model); }
    inline bool isValid() const { return isValidAt(url_model); }
    inline bool isLocal() const { return isLocalAt(url_model); }
    inline bool isRemote() const { return isRemoteAt(url_model); }

    inline const QUrl &location() const { return url_model; }
    void setLocation(const QUrl &url);

    inline QString scheme() const { return schemeAt(url_model); }
    void setScheme(const QString &str);

    inline QString userName() const { return userNameAt(url_model); }
    void setUserName(const QString &str);

    inline QString password() const { return passwordAt(url_model); }
    void setPassword(const QString &str);

    inline QString host() const { return hostAt(url_model); }
    void setHost(const QString &str); // host[:port]

    inline int port() const { return portAt(url_model); }
    void setPort(int num);

    inline QString path() const { return pathAt(url_model); }
    void setPath(const QString &str);

    inline QString query() const { return queryAt(url_model); }
    void setQuery(const QString &str);

    inline QString fragment() const { return fragmentAt(url_model); }
    void setFragment(const QString &str);

    inline QString text() const { return textAt(url_model); }

signals:
    void emptyChanged();
    void validChanged();
    void localChanged();
    void remoteChanged();
    void locationChanged();
    void schemeChanged();
    void userNameChanged();
    void passwordChanged();
    void hostChanged();
    void portChanged();
    void pathChanged();
    void queryChanged();
    void fragmentChanged();
    void textChanged();

private:
    bool reset(const QUrl &url);

    QUrl url_model;
};

#endif // URLMODEL_H
