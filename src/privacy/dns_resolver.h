#ifndef FRINT_DNS_RESOLVER_H
#define FRINT_DNS_RESOLVER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHostAddress>
#include <QString>
#include <QUrl>

namespace Frint {

class DnsResolver : public QObject {
    Q_OBJECT

public:
    static DnsResolver &instance();

    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Resolve a hostname via DNS-over-HTTPS (Cloudflare)
    void resolve(const QString &hostname, std::function<void(bool, QHostAddress)> callback);

    // Blocking resolve (simple version for initial implementation)
    QHostAddress resolveBlocking(const QString &hostname);

    void setDohEndpoint(const QUrl &endpoint);
    QUrl dohEndpoint() const;

signals:
    void resolved(const QString &hostname, const QHostAddress &address);
    void resolutionFailed(const QString &hostname, const QString &error);

private:
    DnsResolver();
    QNetworkAccessManager *m_nam;
    QUrl m_dohEndpoint;
    bool m_enabled = true;
};

} // namespace Frint

#endif // FRINT_DNS_RESOLVER_H
