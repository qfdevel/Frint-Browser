#ifndef FRINT_WEB_VIEW_H
#define FRINT_WEB_VIEW_H

#include <QWidget>
#include <QUrl>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QStack>
#include <QMouseEvent>
#include <QWheelEvent>

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
    void goBack();
    void goForward();

    QUrl currentUrl() const { return m_currentUrl; }
    QString title() const { return m_title; }
    int loadProgress() const { return m_loadProgress; }
    bool isLoading() const { return m_isLoading; }
    bool canGoBack() const;
    bool canGoForward() const;

    void clearHistory();

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
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QUrl ensureHttps(const QUrl &url) const;
    QUrl cleanUrl(const QUrl &url) const;
    QString getReferrer(const QUrl &targetUrl) const;
    bool shouldBlockRequest(const QUrl &requestUrl) const;
    void applyPrivacyHeaders(QNetworkRequest &request) const;
    void showBlockedPage(const QString &reason);
    void showErrorPage(const QString &title, const QString &message);
    void showPlaceholderPage();
    void addToHistory(const QUrl &url);

    QUrl m_currentUrl;
    QString m_title;
    QByteArray m_lastHtml;
    QNetworkAccessManager *m_nam = nullptr;
    int m_loadProgress = 0;
    bool m_isLoading = false;
    int m_scrollY = 0;

    // Navigation history
    QStack<QUrl> m_backHistory;
    QStack<QUrl> m_forwardHistory;
    static constexpr int MAX_HISTORY = 50;
};

} // namespace Frint

#endif // FRINT_WEB_VIEW_H
