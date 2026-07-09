#ifndef FRINT_WEB_VIEW_H
#define FRINT_WEB_VIEW_H

#include <QWidget>
#include <QUrl>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>

#include "privacy/url_cleaner.h"
#include "privacy/referrer_policy.h"
#include "privacy/tracking_blocker.h"
#include "privacy/fingerprinting_defender.h"
#include "privacy/gpc_header.h"
#include "privacy/dns_resolver.h"

namespace Frint {

class WebView : public QWidget {
    Q_OBJECT

public:
    explicit WebView(QWidget *parent = nullptr);
    ~WebView() override;

    void loadUrl(const QUrl &url);
    void reload();
    void stop();

    QUrl currentUrl() const;
    QString title() const;

    // Set the URL without loading it (for display)
    void setCurrentUrl(const QUrl &url);

signals:
    void urlChanged(const QUrl &url);
    void titleChanged(const QString &title);
    void loadStarted();
    void loadFinished(bool ok);
    void loadProgress(int progress);
    void statusBarMessage(const QString &message);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    void applyPrivacyHeaders(QNetworkRequest &request);
    QUrl ensureHttps(const QUrl &url) const;
    QUrl cleanUrl(const QUrl &url) const;
    QString getReferrer(const QUrl &targetUrl) const;
    bool shouldBlockRequest(const QUrl &requestUrl) const;

    QUrl m_currentUrl;
    QString m_title;

    QNetworkAccessManager *m_nam;
    QByteArray m_lastHtml;
    bool m_isLoading = false;

    // Placeholder rendering state
    int m_loadProgress = 0;
};

} // namespace Frint

#endif // FRINT_WEB_VIEW_H
