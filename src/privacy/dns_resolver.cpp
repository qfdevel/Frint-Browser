#include "privacy/dns_resolver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QDebug>
#include <QUrlQuery>
#include <functional>

namespace Frint {

DnsResolver &DnsResolver::instance()
{
    static DnsResolver s_instance;
    return s_instance;
}

DnsResolver::DnsResolver()
    : QObject(nullptr)
    , m_nam(new QNetworkAccessManager(this))
    , m_dohEndpoint("https://cloudflare-dns.com/dns-query")
{
}

void DnsResolver::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool DnsResolver::isEnabled() const
{
    return m_enabled;
}

void DnsResolver::setDohEndpoint(const QUrl &endpoint)
{
    m_dohEndpoint = endpoint;
}

QUrl DnsResolver::dohEndpoint() const
{
    return m_dohEndpoint;
}

void DnsResolver::resolve(const QString &hostname,
                          std::function<void(bool, QHostAddress)> callback)
{
    if (!m_enabled) {
        // Fall back to system DNS
        QHostAddress addr;
        if (addr.setAddress(hostname)) {
            // Already an IP
            if (callback) callback(true, addr);
        } else {
            if (callback) callback(false, QHostAddress());
        }
        return;
    }

    QUrl url(m_dohEndpoint);
    QUrlQuery query;
    query.addQueryItem("name", hostname);
    query.addQueryItem("type", "A");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/dns-json");

    QNetworkReply *reply = m_nam->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, hostname, callback]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[Frint] DoH resolution failed for" << hostname
                       << ":" << reply->errorString();
            emit resolutionFailed(hostname, reply->errorString());
            if (callback) callback(false, QHostAddress());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        if (doc.isNull() || !doc.isObject()) {
            emit resolutionFailed(hostname, "Invalid DoH response");
            if (callback) callback(false, QHostAddress());
            return;
        }

        QJsonObject obj = doc.object();
        QJsonArray answers = obj["Answer"].toArray();
        bool found = false;
        QHostAddress addr;

        for (const QJsonValue &val : answers) {
            QJsonObject ans = val.toObject();
            int type = ans["type"].toInt();
            // Type 1 = A record (IPv4), Type 28 = AAAA record (IPv6)
            if (type == 1 || type == 28) {
                QString ip = ans["data"].toString();
                if (addr.setAddress(ip)) {
                    found = true;
                    qDebug() << "[Frint] DoH resolved" << hostname << "->" << ip;
                    break;
                }
            }
        }

        if (!found) {
            emit resolutionFailed(hostname, "No address records found");
            if (callback) callback(false, QHostAddress());
            return;
        }

        emit resolved(hostname, addr);
        if (callback) callback(true, addr);
    });
}

QHostAddress DnsResolver::resolveBlocking(const QString &hostname)
{
    if (!m_enabled) {
        return QHostAddress();
    }

    QUrl url(m_dohEndpoint);
    QUrlQuery query;
    query.addQueryItem("name", hostname);
    query.addQueryItem("type", "A");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/dns-json");

    QNetworkReply *reply = m_nam->get(request);
    QHostAddress result;

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            QJsonArray answers = obj["Answer"].toArray();

            for (const QJsonValue &val : answers) {
                QJsonObject ans = val.toObject();
                int type = ans["type"].toInt();
                if (type == 1 || type == 28) {
                    QString ip = ans["data"].toString();
                    if (result.setAddress(ip)) {
                        break;
                    }
                }
            }
        }
    }

    reply->deleteLater();
    return result;
}

} // namespace Frint
