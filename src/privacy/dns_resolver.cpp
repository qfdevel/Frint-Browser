#include "privacy/dns_resolver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QEventLoop>
#include <QDebug>
#include <QThread>
#include <QDateTime>

namespace Frint {

// ── DNS cache size ───────────────────────────────────────────────────────
static constexpr int DNS_CACHE_SIZE = 256;
static constexpr qint64 DNS_CACHE_TTL_MS = 300000; // 5 minutes

DnsResolver &DnsResolver::instance()
{
    static DnsResolver s_instance;
    return s_instance;
}

DnsResolver::DnsResolver(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_dohEndpoint("https://cloudflare-dns.com/dns-query")
    , m_cache(DNS_CACHE_SIZE)
{
}

// ── Endpoints ────────────────────────────────────────────────────────────

QUrl DnsResolver::cloudflareEndpoint()
{
    return QUrl("https://cloudflare-dns.com/dns-query");
}

QUrl DnsResolver::quad9Endpoint()
{
    return QUrl("https://dns.quad9.net/dns-query");
}

QUrl DnsResolver::googleEndpoint()
{
    return QUrl("https://dns.google/dns-query");
}

// ── Configuration ────────────────────────────────────────────────────────

void DnsResolver::setEnabled(bool enabled)
{
    m_enabled = enabled;
    qDebug() << "[Frint DnsResolver]" << (enabled ? "Enabled" : "Disabled");
}

bool DnsResolver::isEnabled() const
{
    return m_enabled;
}

void DnsResolver::setDohEndpoint(const QUrl &endpoint)
{
    m_dohEndpoint = endpoint;
    qDebug() << "[Frint DnsResolver] DoH endpoint:" << endpoint.toString();
}

QUrl DnsResolver::dohEndpoint() const
{
    return m_dohEndpoint;
}

void DnsResolver::setCacheEnabled(bool enabled)
{
    m_cacheEnabled = enabled;
    if (!enabled) clearCache();
}

void DnsResolver::clearCache()
{
    m_cache.clear();
    qDebug() << "[Frint DnsResolver] Cache cleared";
}

// ── Resolve (async, callback-based) ──────────────────────────────────────

void DnsResolver::resolve(const QString &hostname,
                          std::function<void(bool, QHostAddress)> callback)
{
    // Check cache first
    if (m_cacheEnabled && m_cache.contains(hostname)) {
        DnsCacheEntry *entry = m_cache.object(hostname);
        if (entry && (QDateTime::currentMSecsSinceEpoch() - entry->timestamp
                      < DNS_CACHE_TTL_MS)) {
            qDebug() << "[Frint DnsResolver] Cache hit:" << hostname
                     << "->" << entry->address.toString();
            if (callback) callback(true, entry->address);
            return;
        }
        m_cache.remove(hostname);
    }

    if (!m_enabled) {
        // Fallback: try system DNS
        QHostAddress addr(hostname);
        if (addr.isNull()) {
            if (callback) callback(false, QHostAddress());
        } else {
            if (callback) callback(true, addr);
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
    request.setTransferTimeout(10000);

    QNetworkReply *reply = m_nam->get(request);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, hostname, callback]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[Frint DnsResolver] Error for" << hostname
                       << ":" << reply->errorString();
            emit resolutionFailed(hostname, reply->errorString());
            if (callback) callback(false, QHostAddress());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit resolutionFailed(hostname, "Invalid JSON response");
            if (callback) callback(false, QHostAddress());
            return;
        }

        QJsonObject obj = doc.object();
        QJsonArray answers = obj["Answer"].toArray();
        QHostAddress addr;

        for (const QJsonValue &val : answers) {
            QJsonObject ans = val.toObject();
            int type = ans["type"].toInt();
            if (type == 1 || type == 28) {
                QString ip = ans["data"].toString();
                if (addr.setAddress(ip)) {
                    break;
                }
            }
        }

        if (addr.isNull()) {
            emit resolutionFailed(hostname, "No address records found");
            if (callback) callback(false, QHostAddress());
            return;
        }

        qDebug() << "[Frint DnsResolver] DoH resolved" << hostname
                 << "->" << addr.toString();

        // Cache the result
        if (m_cacheEnabled) {
            auto *entry = new DnsCacheEntry{ addr,
                QDateTime::currentMSecsSinceEpoch() };
            m_cache.insert(hostname, entry);
        }

        emit resolved(hostname, addr);
        if (callback) callback(true, addr);
    });
}

// ── Resolve (blocking) ───────────────────────────────────────────────────

QHostAddress DnsResolver::resolveBlocking(const QString &hostname)
{
    // Check cache
    if (m_cacheEnabled && m_cache.contains(hostname)) {
        DnsCacheEntry *entry = m_cache.object(hostname);
        if (entry && (QDateTime::currentMSecsSinceEpoch() - entry->timestamp
                      < DNS_CACHE_TTL_MS)) {
            return entry->address;
        }
        m_cache.remove(hostname);
    }

    if (!m_enabled) {
        QHostAddress addr(hostname);
        return addr;
    }

    QUrl url(m_dohEndpoint);
    QUrlQuery query;
    query.addQueryItem("name", hostname);
    query.addQueryItem("type", "A");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/dns-json");
    request.setTransferTimeout(10000);

    QNetworkReply *reply = m_nam->get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QHostAddress result;
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonArray answers = doc.object()["Answer"].toArray();
            for (const QJsonValue &val : answers) {
                QJsonObject ans = val.toObject();
                int type = ans["type"].toInt();
                if (type == 1 || type == 28) {
                    QString ip = ans["data"].toString();
                    if (result.setAddress(ip)) break;
                }
            }
        }
    }

    reply->deleteLater();

    if (!result.isNull() && m_cacheEnabled) {
        auto *entry = new DnsCacheEntry{
            result, QDateTime::currentMSecsSinceEpoch() };
        m_cache.insert(hostname, entry);
    }

    return result;
}

} // namespace Frint
