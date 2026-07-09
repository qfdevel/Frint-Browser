#ifndef FRINT_PRIVACY_URL_INTERCEPTOR_H
#define FRINT_PRIVACY_URL_INTERCEPTOR_H

#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineUrlRequestInfo>

namespace Frint {

class PrivacyUrlInterceptor : public QWebEngineUrlRequestInterceptor {
    Q_OBJECT

public:
    explicit PrivacyUrlInterceptor(QObject *parent = nullptr);
    ~PrivacyUrlInterceptor() override;

    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

    // Toggle privacy features at runtime
    void setTrackingBlockerEnabled(bool enabled);
    void setHttpsOnlyEnabled(bool enabled);
    void setGpcEnabled(bool enabled);
    void setReferrerPolicyEnabled(bool enabled);

signals:
    void requestBlocked(const QString &url, const QString &reason);

private:
    QUrl cleanTrackingParams(const QUrl &url) const;
    QUrl upgradeToHttps(const QUrl &url) const;
    bool isTracker(const QUrl &requestUrl, const QUrl &firstPartyUrl) const;
    QString getReferrerHeader(const QUrl &targetUrl, const QUrl &originUrl) const;
    QString spoofedUserAgent() const;

    bool m_trackingBlockerEnabled = true;
    bool m_httpsOnlyEnabled = true;
    bool m_gpcEnabled = true;
    bool m_referrerPolicyEnabled = true;
};

} // namespace Frint

#endif
