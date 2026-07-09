#include "web_view.h"
#include "settings_manager.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QFontMetrics>
#include <QDateTime>
#include <QDebug>

namespace Frint {

WebView::WebView(QWidget *parent)
    : QWidget(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(m_nam, &QNetworkAccessManager::finished,
            this, &WebView::onReplyFinished);

    // Apply privacy settings from the manager
    auto &settings = SettingsManager::instance();

    TrackingBlocker::instance().setEnabled(settings.isTrackingBlockerEnabled());
    FingerprintingDefender::instance().setEnabled(settings.isFingerprintingProtectionEnabled());
    GpcHeader::instance().setEnabled(settings.isGpcEnabled());
    DnsResolver::instance().setEnabled(settings.isDohEnabled());

    // Set referrer policy
    ReferrerPolicy::instance().setPolicy(
        ReferrerPolicy::Policy::StrictOriginWhenCrossOrigin);
}

WebView::~WebView() = default;

QUrl WebView::ensureHttps(const QUrl &url) const
{
    if (!SettingsManager::instance().isHttpsOnly()) {
        return url;
    }

    QUrl result = url;
    if (result.scheme() == "http" && result.host() != "localhost"
        && result.host() != "127.0.0.1" && result.host() != "::1") {
        result.setScheme("https");
    }
    return result;
}

QUrl WebView::cleanUrl(const QUrl &url) const
{
    return UrlCleaner::cleanTrackingParams(url);
}

QString WebView::getReferrer(const QUrl &targetUrl) const
{
    return ReferrerPolicy::instance().getReferrer(targetUrl, m_currentUrl);
}

bool WebView::shouldBlockRequest(const QUrl &requestUrl) const
{
    return TrackingBlocker::instance().isTracker(requestUrl, m_currentUrl);
}

void WebView::applyPrivacyHeaders(QNetworkRequest &request)
{
    // Apply GPC header (Global Privacy Control)
    auto gpc = GpcHeader::instance().header();
    if (!gpc.first.isEmpty()) {
        request.setRawHeader(gpc.first, gpc.second);
    }

    // Apply referrer
    QString referrer = getReferrer(request.url());
    if (!referrer.isEmpty()) {
        request.setRawHeader("Referer", referrer.toUtf8());
    }

    // Apply Do Not Track (legacy fallback)
    request.setRawHeader("DNT", "1");

    // Set reasonable User-Agent
    QString ua = FingerprintingDefender::instance().spoofedUserAgent();
    if (!ua.isEmpty()) {
        request.setRawHeader("User-Agent", ua.toUtf8());
    }
}

void WebView::loadUrl(const QUrl &url)
{
    QUrl finalUrl = ensureHttps(cleanUrl(url));

    if (finalUrl.scheme() == "about") {
        m_currentUrl = finalUrl;
        m_lastHtml = QByteArrayLiteral("<html><body style='background:#1e1e2e; color:#cdd6f4; "
                     "font-family:sans-serif; display:flex; align-items:center; justify-content:center; "
                     "height:100vh; margin:0;'><div style='text-align:center;'>"
                     "<h1 style='font-size:3em; color:#cba6f7;'>\U0001F310</h1>"
                     "<h2 style='color:#a6e3a1;'>Frint Browser</h2>"
                     "<p style='color:#bac2de;'>Privacy-first. Telemetry-free. Yours.</p>"
                     "<p style='color:#585b70; font-size:0.85em;'>v0.1.0</p>"
                     "</div></body></html>");
        m_title = "about:blank";
        emit loadStarted();
        emit titleChanged(m_title);
        emit urlChanged(m_currentUrl);
        update();
        emit loadFinished(true);
        return;
    }

    m_currentUrl = finalUrl;
    m_isLoading = true;
    m_loadProgress = 0;
    m_lastHtml.clear();

    emit loadStarted();
    emit urlChanged(finalUrl);

    QNetworkRequest request(finalUrl);
    applyPrivacyHeaders(request);

    // Check if this should be blocked
    if (shouldBlockRequest(finalUrl)) {
        m_lastHtml = QByteArrayLiteral("<html><body style='background:#1e1e2e; color:#f38ba8; "
                     "font-family:sans-serif; display:flex; align-items:center; "
                     "justify-content:center; height:100vh; margin:0;'>"
                     "<div style='text-align:center;'>"
                     "<h2>Request Blocked</h2>"
                     "<p>This request was blocked by Frint's tracker blocker.</p>"
                     "</div></body></html>");
        m_isLoading = false;
        emit loadFinished(true);
        update();
        return;
    }

    m_nam->get(request);
}

void WebView::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[Frint] Network error:" << reply->errorString();
        m_lastHtml = QByteArrayLiteral("<html><body style='background:#1e1e2e; color:#f38ba8; "
                     "font-family:sans-serif; display:flex; align-items:center; "
                     "justify-content:center; height:100vh; margin:0;'>"
                     "<div style='text-align:center;'>"
                     "<h2>Failed to Load Page</h2>"
                     "<p style='color:#bac2de;'>").append(
                         reply->errorString().toUtf8()).append(
                     "</p></div></body></html>");
        m_isLoading = false;
        emit loadFinished(false);
        update();
        return;
    }

    // Check for redirects
    QUrl redirectUrl = reply->attribute(
        QNetworkRequest::RedirectPolicyAttribute).toUrl();
    if (redirectUrl.isValid()) {
        loadUrl(redirectUrl);
        return;
    }

    QByteArray contentType = reply->header(
        QNetworkRequest::ContentTypeHeader).toByteArray();

    if (contentType.contains("text/html")) {
        m_lastHtml = reply->readAll();
    } else {
        // Non-HTML content (placeholder)
        m_lastHtml = QByteArrayLiteral("<html><body style='background:#1e1e2e; color:#cdd6f4; "
                     "font-family:sans-serif; padding:2em;'>"
                     "<h2>Non-HTML Content</h2>"
                     "<p>Content type: ").append(contentType).append(
                     "</p></body></html>");
    }

    m_title = m_currentUrl.toString();
    m_isLoading = false;
    m_loadProgress = 100;

    emit titleChanged(m_title);
    emit loadFinished(true);
    update();
}

void WebView::reload()
{
    if (m_currentUrl.isValid()) {
        loadUrl(m_currentUrl);
    }
}

void WebView::stop()
{
    m_isLoading = false;
    // Abort any pending replies
    QList<QNetworkReply *> pending = m_nam->findChildren<QNetworkReply *>();
    for (QNetworkReply *reply : pending) {
        if (reply->isRunning()) {
            reply->abort();
        }
    }
}

QUrl WebView::currentUrl() const
{
    return m_currentUrl;
}

QString WebView::title() const
{
    return m_title;
}

void WebView::setCurrentUrl(const QUrl &url)
{
    m_currentUrl = url;
    emit urlChanged(url);
}

void WebView::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background
    painter.fillRect(rect(), QColor("#1e1e2e"));

    if (m_lastHtml.isEmpty()) {
        // Placeholder page
        painter.setPen(QColor("#585b70"));
        QFont placeholderFont("sans-serif", 14);
        painter.setFont(placeholderFont);
        painter.drawText(rect(), Qt::AlignCenter,
                         "Frint Browser\n\nEnter a URL to browse");
        return;
    }

    // HTML placeholder rendering — in the future this will use LibWeb
    // For now, render a simplified version
    painter.setPen(QColor("#cdd6f4"));
    QFont contentFont("monospace", 10);
    painter.setFont(contentFont);

    QString htmlText = QString::fromUtf8(m_lastHtml);
    QTextStream stream(&htmlText);

    // Simple text extraction from HTML for placeholder rendering
    QString plainText;
    bool inTag = false;
    for (const QChar &ch : htmlText) {
        if (ch == '<') {
            inTag = true;
            continue;
        }
        if (ch == '>') {
            inTag = false;
            continue;
        }
        if (!inTag) {
            plainText += ch;
        }
    }

    // Draw page info
    QRect textRect = rect().adjusted(20, 20, -20, -20);
    painter.drawText(textRect, Qt::AlignLeft | Qt::TextWordWrap,
                     plainText.trimmed().left(1000));
}

void WebView::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
}

void WebView::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
}

void WebView::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
}

void WebView::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);
}

void WebView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

} // namespace Frint
