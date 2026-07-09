#include "web_view.h"
#include "settings_manager.h"

#include <QPainter>
#include <QTextStream>
#include <QFontMetrics>
#include <QDebug>
#include <QStyleOption>
#include <QDateTime>
#include <QUrlQuery>

namespace Frint {

WebView::WebView(QWidget *parent)
    : QWidget(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(200, 100);

    connect(m_nam, &QNetworkAccessManager::finished,
            this, &WebView::onReplyFinished);

    // Apply initial privacy settings
    auto &settings = SettingsManager::instance();
    TrackingBlocker::instance().setEnabled(settings.isTrackingBlockerEnabled());
    GpcHeader::instance().setEnabled(settings.isGpcEnabled());
    DnsResolver::instance().setEnabled(settings.isDohEnabled());
    FingerprintingDefender::instance().setEnabled(
        settings.isFingerprintingProtectionEnabled());
    ReferrerPolicy::instance().setPolicy(
        ReferrerPolicy::Policy::StrictOriginWhenCrossOrigin);

    showPlaceholderPage();
}

WebView::~WebView()
{
    // Cancel any pending requests
    QList<QNetworkReply *> pending = m_nam->findChildren<QNetworkReply *>();
    for (auto *r : pending) {
        if (r->isRunning()) r->abort();
    }
}

// ── Privacy helpers ──────────────────────────────────────────────────────

QUrl WebView::ensureHttps(const QUrl &url) const
{
    if (!SettingsManager::instance().isHttpsOnly()) {
        return url;
    }

    QUrl result = url;
    if (result.scheme() == "http"
        && result.host() != "localhost"
        && result.host() != "127.0.0.1"
        && result.host() != "::1") {
        result.setScheme("https");
        qDebug() << "[Frint] HTTPS upgrade:" << url.toString()
                 << "->" << result.toString();
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

void WebView::applyPrivacyHeaders(QNetworkRequest &request) const
{
    // GPC
    auto gpc = GpcHeader::instance().header();
    if (!gpc.first.isEmpty()) {
        request.setRawHeader(gpc.first, gpc.second);
    }

    // Referrer
    QString referrer = getReferrer(request.url());
    if (!referrer.isEmpty()) {
        request.setRawHeader("Referer", referrer.toUtf8());
    }

    // DNT (legacy)
    request.setRawHeader("DNT", "1");

    // User-Agent
    QString ua = FingerprintingDefender::instance().spoofedUserAgent();
    if (!ua.isEmpty()) {
        request.setRawHeader("User-Agent", ua.toUtf8());
    }
}

// ── Navigation ───────────────────────────────────────────────────────────

void WebView::loadUrl(const QUrl &url)
{
    if (!url.isValid() || url.toString().isEmpty()) return;

    // Step 1: Clean tracking params
    QUrl clean = cleanUrl(url);

    // Step 2: Ensure HTTPS
    QUrl finalUrl = ensureHttps(clean);

    // Handle about: pages
    if (finalUrl.scheme() == "about") {
        m_currentUrl = finalUrl;
        showPlaceholderPage();
        emit urlChanged(finalUrl);
        emit titleChanged("about:blank");
        emit loadFinished(true);
        return;
    }

    // Step 3: Check tracker blocking
    if (shouldBlockRequest(finalUrl)) {
        showBlockedPage("This domain is blocked by the tracker blocker.\n\n"
                        "URL: " + finalUrl.toString());
        m_currentUrl = finalUrl;
        emit urlChanged(finalUrl);
        emit loadFinished(true);
        return;
    }

    addToHistory(finalUrl);
    m_currentUrl = finalUrl;
    m_isLoading = true;
    m_loadProgress = 0;

    emit loadStarted();
    emit urlChanged(finalUrl);
    emit statusBarMessage("Loading " + finalUrl.host() + "...");

    // Step 4: Apply privacy headers and fetch
    QNetworkRequest request(finalUrl);
    applyPrivacyHeaders(request);
    request.setTransferTimeout(30000);
    m_nam->get(request);
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
    QList<QNetworkReply *> pending = m_nam->findChildren<QNetworkReply *>();
    for (auto *r : pending) {
        if (r->isRunning()) r->abort();
    }
    emit statusBarMessage("Stopped");
}

void WebView::goBack()
{
    if (!m_backHistory.isEmpty()) {
        QUrl prev = m_backHistory.pop();
        m_forwardHistory.push(m_currentUrl.isValid() ? m_currentUrl
                                                     : QUrl("about:blank"));
        m_currentUrl = QUrl();  // Reset to force fresh load
        loadUrl(prev);
    }
}

void WebView::goForward()
{
    if (!m_forwardHistory.isEmpty()) {
        QUrl next = m_forwardHistory.pop();
        m_backHistory.push(m_currentUrl.isValid() ? m_currentUrl
                                                  : QUrl("about:blank"));
        m_currentUrl = QUrl();
        loadUrl(next);
    }
}

bool WebView::canGoBack() const
{
    return !m_backHistory.isEmpty();
}

bool WebView::canGoForward() const
{
    return !m_forwardHistory.isEmpty();
}

void WebView::addToHistory(const QUrl &url)
{
    if (m_currentUrl.isValid() && m_currentUrl != url
        && url.scheme() != "about") {
        m_backHistory.push(m_currentUrl);
        // Trim back history
        while (m_backHistory.size() > MAX_HISTORY) {
            m_backHistory.removeFirst();
        }
    }
    // Clear forward history on new navigation
    m_forwardHistory.clear();
}

void WebView::clearHistory()
{
    m_backHistory.clear();
    m_forwardHistory.clear();
}

// ── Network reply handling ───────────────────────────────────────────────

void WebView::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (!m_isLoading) return;  // Was stopped
    m_isLoading = false;

    if (reply->error() != QNetworkReply::NoError) {
        // Check for redirect
        QVariant redirect = reply->attribute(
            QNetworkRequest::RedirectionTargetAttribute);
        if (redirect.isValid()) {
            QUrl redirectUrl = redirect.toUrl();
            if (redirectUrl.isRelative()) {
                redirectUrl = reply->url().resolved(redirectUrl);
            }
            qDebug() << "[Frint] Redirect:" << reply->url().toString()
                     << "->" << redirectUrl.toString();
            loadUrl(redirectUrl);
            return;
        }

        QString errMsg = reply->errorString();
        showErrorPage("Network Error", errMsg);
        emit statusBarMessage("Error: " + errMsg);
        emit loadFinished(false);
        return;
    }

    // Read content
    QByteArray contentType = reply->header(
        QNetworkRequest::ContentTypeHeader).toByteArray().toLower();

    m_loadProgress = 100;

    if (contentType.contains("text/html")
        || contentType.contains("application/xhtml")) {
        m_lastHtml = reply->readAll();
        m_title = m_currentUrl.toString();

        // Try to extract <title>
        QString htmlStr = QString::fromUtf8(m_lastHtml);
        int titleStart = htmlStr.indexOf("<title", Qt::CaseInsensitive);
        if (titleStart >= 0) {
            int tagEnd = htmlStr.indexOf('>', titleStart) + 1;
            int titleEnd = htmlStr.indexOf("</title", tagEnd, Qt::CaseInsensitive);
            if (tagEnd > 0 && titleEnd > tagEnd) {
                m_title = htmlStr.mid(tagEnd, titleEnd - tagEnd).trimmed();
            }
        }
    } else {
        // Non-HTML content
        m_lastHtml = QByteArrayLiteral(
            "<html><body style='background:#1e1e2e; color:#cdd6f4; "
            "font-family:sans-serif; padding:2em;'>"
            "<h2>Non-HTML Content</h2>"
            "<p>Content type: </p>")
            .append(contentType)
            .append("</body></html>");
        m_title = m_currentUrl.toString();
    }

    emit titleChanged(m_title);
    emit loadFinished(true);
    emit loadProgress(100);
    emit statusBarMessage("Done — " + m_currentUrl.host());

    update();
}

// ── Page rendering ───────────────────────────────────────────────────────

void WebView::showBlockedPage(const QString &reason)
{
    m_lastHtml = QByteArrayLiteral(
        "<html><body style='background:#1e1e2e; color:#cdd6f4; "
        "font-family:sans-serif; display:flex; align-items:center; "
        "justify-content:center; height:100vh; margin:0;'>"
        "<div style='text-align:center; max-width:600px;'>"
        "<div style='font-size:4em; margin-bottom:0.5em;'>&#x1F6AB;</div>"
        "<h2 style='color:#f38ba8;'>Request Blocked</h2>"
        "<p style='color:#bac2de;'>Frint Browser blocked this request "
        "to protect your privacy.</p>"
        "<p style='color:#585b70; font-size:0.85em;'>")
        .append(reason.toUtf8())
        .append("</p></div></body></html>");
}

void WebView::showErrorPage(const QString &title, const QString &message)
{
    m_lastHtml = QByteArrayLiteral(
        "<html><body style='background:#1e1e2e; color:#cdd6f4; "
        "font-family:sans-serif; display:flex; align-items:center; "
        "justify-content:center; height:100vh; margin:0;'>"
        "<div style='text-align:center; max-width:600px;'>"
        "<div style='font-size:4em; margin-bottom:0.5em;'>&#x26A0;</div>"
        "<h2 style='color:#f38ba8;'>")
        .append(title.toUtf8())
        .append("</h2><p style='color:#bac2de;'>")
        .append(message.toUtf8())
        .append("</p></div></body></html>");
}

void WebView::showPlaceholderPage()
{
    m_lastHtml = QByteArrayLiteral(
        "<html><body style='background:#1e1e2e; color:#cdd6f4; "
        "font-family:sans-serif; display:flex; align-items:center; "
        "justify-content:center; height:100vh; margin:0;'>"
        "<div style='text-align:center;'>"
        "<h1 style='font-size:3em; color:#cba6f7; margin:0;'>&#x1F310;</h1>"
        "<h2 style='color:#a6e3a1; margin:0.5em 0;'>Frint Browser</h2>"
        "<p style='color:#bac2de;'>Privacy-first. Telemetry-free. Yours.</p>"
        "<p style='color:#585b70; font-size:0.85em; margin-top:2em;'>"
        "Enter a URL in the address bar above to start browsing</p>"
        "</div></body></html>");
    m_currentUrl = QUrl("about:blank");
    m_title = "Frint Browser";
}

void WebView::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Background
    painter.fillRect(rect(), QColor("#1e1e2e"));

    if (m_lastHtml.isEmpty()) {
        // Draw placeholder
        painter.setPen(QColor("#585b70"));
        QFont f("sans-serif", 14);
        painter.setFont(f);
        painter.drawText(rect(), Qt::AlignCenter, "Frint Browser\n\nEnter a URL to browse");
        return;
    }

    // Simple HTML text extraction for display
    QString htmlStr = QString::fromUtf8(m_lastHtml);
    QString plainText;
    bool inTag = false;
    bool inScript = false;
    bool inStyle = false;

    for (int i = 0; i < htmlStr.length(); ++i) {
        QChar ch = htmlStr[i];

        if (ch == '<') {
            // Check for script/style tags
            QString remaining = htmlStr.mid(i).toLower();
            if (remaining.startsWith("<script")) {
                inScript = true;
            } else if (remaining.startsWith("<style")) {
                inStyle = true;
            } else if (inScript && remaining.startsWith("</script")) {
                inScript = false;
            } else if (inStyle && remaining.startsWith("</style")) {
                inStyle = false;
            }
            inTag = true;
            continue;
        }
        if (ch == '>') {
            inTag = false;
            continue;
        }
        if (!inTag && !inScript && !inStyle) {
            // Convert &nbsp; and other entities
            if (ch == '&') {
                int semi = htmlStr.indexOf(';', i);
                if (semi > i && semi - i < 20) {
                    QString entity = htmlStr.mid(i + 1, semi - i - 1);
                    if (entity == "nbsp") plainText += ' ';
                    else if (entity == "amp") plainText += '&';
                    else if (entity == "lt") plainText += '<';
                    else if (entity == "gt") plainText += '>';
                    else if (entity == "quot") plainText += '"';
                    else if (entity == "apos") plainText += '\'';
                    i = semi;
                    continue;
                }
            }
            plainText += ch;
        }
    }

    // Collapse whitespace
    QString displayText;
    bool lastWasSpace = false;
    for (const QChar &c : plainText) {
        if (c == '\n' || c == '\r' || c == '\t') {
            if (!lastWasSpace) {
                displayText += ' ';
                lastWasSpace = true;
            }
        } else if (c.isSpace()) {
            if (!lastWasSpace) {
                displayText += ' ';
                lastWasSpace = true;
            }
        } else {
            displayText += c;
            lastWasSpace = false;
        }
    }

    // Draw text
    QFont font("monospace", 10);
    QFontMetrics fm(font);
    painter.setFont(font);
    painter.setPen(QColor("#cdd6f4"));

    QRect textRect = rect().adjusted(16, 16 + m_scrollY, -16, -16);
    painter.drawText(textRect, Qt::AlignLeft | Qt::TextWordWrap,
                     displayText.trimmed().left(5000));
}

// ── Event forwarding (for future LibWeb integration) ──────────────────────

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

void WebView::wheelEvent(QWheelEvent *event)
{
    m_scrollY += event->angleDelta().y();
    m_scrollY = qMin(0, m_scrollY); // Don't scroll past top
    update();
    event->accept();
}

void WebView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

} // namespace Frint
