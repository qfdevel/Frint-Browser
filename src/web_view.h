#ifndef FRINT_WEB_VIEW_H
#define FRINT_WEB_VIEW_H

#include <QWidget>
#include <QUrl>
#include <QString>
#include <QVBoxLayout>
#include <QStack>
#include <QWebEngineView>
#include <QWebEngineProfile>
#include <QWebEnginePage>
#include <QWebEngineHistory>
#include <QFile>

#include "privacy/url_cleaner.h"
#include "privacy/referrer_policy.h"
#include "privacy/tracking_blocker.h"
#include "privacy/fingerprinting_defender.h"
#include "privacy/gpc_header.h"
#include "privacy/dns_resolver.h"
#include "privacy/privacy_url_interceptor.h"

namespace Frint {

class WebView : public QWidget {
    Q_OBJECT

public:
    explicit WebView(QWidget *parent = nullptr);
    ~WebView() override;

    void loadUrl(const QUrl &url);
    void loadHtml(const QString &html, const QUrl &baseUrl = QUrl());
    void reload();
    void stop();
    void goBack();
    void goForward();

    QUrl currentUrl() const;
    QString title() const;
    int loadProgress() const;
    bool isLoading() const;
    bool canGoBack() const;
    bool canGoForward() const;

    void clearHistory();
    QWebEngineView *engineView() const { return m_engineView; }
    QWebEnginePage *page() const { return m_engineView ? m_engineView->page() : nullptr; }

    // Privacy interceptor access
    PrivacyUrlInterceptor *privacyInterceptor() const { return m_interceptor; }

    // Apply current privacy settings
    void updatePrivacySettings();

signals:
    void urlChanged(const QUrl &url);
    void titleChanged(const QString &title);
    void loadStarted();
    void loadFinished(bool ok);
    void loadProgress(int progress);
    void statusBarMessage(const QString &message);

private slots:
    void onEngineUrlChanged(const QUrl &url);
    void onEngineTitleChanged(const QString &title);
    void onEngineLoadStarted();
    void onEngineLoadFinished(bool ok);
    void onEngineLoadProgress(int progress);

private:
    QUrl cleanUrl(const QUrl &url) const;
    bool shouldBlockRequest(const QUrl &requestUrl) const;

    QWebEngineView *m_engineView;
    QWebEngineProfile *m_profile;
    PrivacyUrlInterceptor *m_interceptor;

    // Track current state
    QUrl m_currentUrl;
    QString m_title;
    bool m_isLoading = false;
    int m_loadProgress = 0;

    // History
    QStack<QUrl> m_backHistory;
    QString m_antiFingerprintScript;
    QString m_cosmeticBlockerScript;
    bool m_antiFingerprintEnabled = true;
    QStack<QUrl> m_forwardHistory;
    bool m_navigatingHistory = false;
    static constexpr int MAX_HISTORY = 50;
};

} // namespace Frint

#endif
