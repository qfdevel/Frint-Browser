#ifndef FRINT_DNS_RESOLVER_H
#define FRINT_DNS_RESOLVER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHostAddress>
#include <QString>
#include <QUrl>
#include <QCache>
#include <QElapsedTimer>
#include <functional>

namespace Frint {

struct DnsCacheEntry {
    QHostAddress address;
    qint64 timestamp;
};

class DnsResolver : public QObject {
    Q_OBJECT

public:
    static DnsResolver &instance();

    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Resolve via DoH (Cloudflare default)
    void resolve(const QString &hostname,
                 std::function<void(bool, QHostAddress)> callback);

    // Blocking resolve
    QHostAddress resolveBlocking(const QString &hostname);

    // Configure DoH endpoint
    void setDohEndpoint(const QUrl &endpoint);
    QUrl dohEndpoint() const;

    // Supported endpoints
    static QUrl cloudflareEndpoint();
    static QUrl quad9Endpoint();
    static QUrl googleEndpoint();

    void setCacheEnabled(bool enabled);
    void clearCache();

signals:
    void resolved(const QString &hostname, const QHostAddress &address);
    void resolutionFailed(const QString &hostname, const QString &error);

private:
    DnsResolver(QObject *parent = nullptr);
    DnsResolver(const DnsResolver &) = delete;
    DnsResolver &operator=(const DnsResolver &) = delete;

    QNetworkAccessManager *m_nam;
    QUrl m_dohEndpoint;
    bool m_enabled = true;
    bool m_cacheEnabled = true;
    QCache<QString, DnsCacheEntry> m_cache;
};

} // namespace Frint

#endif // FRINT_DNS_RESOLVER_H
