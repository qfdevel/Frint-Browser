#include "web_view.h"
#include "settings_manager.h"
#include "storage/storage_partition.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QWebEngineSettings>
#include <QWebEnginePermission>
#include <QWebEngineCookieStore>

namespace Frint {

WebView::WebView(QWidget *parent)
    : QWidget(parent)
{
    // ── Create the privacy URL interceptor ──
    m_interceptor = new PrivacyUrlInterceptor(this);

    // ── Create WebEngine profile with privacy interceptor ──
    m_profile = new QWebEngineProfile(this);
    m_profile->setUrlRequestInterceptor(m_interceptor);

    // ── Privacy-safe settings: memory cache, no persistent cookies ──
    m_profile->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
    m_profile->setHttpCacheMaximumSize(20 * 1024 * 1024); // 20MB memory cache
    m_profile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    m_profile->setHttpUserAgent(FingerprintingDefender::instance().spoofedUserAgent());
    m_profile->setPushServiceEnabled(false);

    // Store permissions in memory only
    m_profile->setPersistentPermissionsPolicy(QWebEngineProfile::PersistentPermissionsPolicy::StoreInMemory);

    // ── Create the WebEngine view ──
    m_engineView = new QWebEngineView(this);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_engineView);

    // Apply WebEngine page with our profile
    auto *page = new QWebEnginePage(m_profile, m_engineView);
    m_engineView->setPage(page);

    // ── Balanced privacy + usability settings ──
    auto *ws = m_engineView->settings();
    ws->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    ws->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, false);
    ws->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    ws->setAttribute(QWebEngineSettings::PluginsEnabled, false);
    ws->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled, false);
    ws->setAttribute(QWebEngineSettings::ErrorPageEnabled, true);
    ws->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    ws->setAttribute(QWebEngineSettings::AutoLoadImages, true);
    ws->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, false);
    ws->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);
    ws->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    ws->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    ws->setAttribute(QWebEngineSettings::WebRTCPublicInterfacesOnly, true);
    ws->setAttribute(QWebEngineSettings::BackForwardCacheEnabled, true);

    m_engineView->setFocusPolicy(Qt::StrongFocus);

    // ── Load anti-fingerprinting script ──
    QFile afFile(":/frint/privacy/anti_fingerprint.js");
    if (afFile.open(QIODevice::ReadOnly)) {
        m_antiFingerprintScript = QString::fromUtf8(afFile.readAll());
        afFile.close();
        qDebug() << "[Frint] Loaded anti-fingerprint script:" << m_antiFingerprintScript.length() << "bytes";
    }

    // ── Auto-deny permission requests (notifications, geolocation, etc.) ──
    connect(page, &QWebEnginePage::permissionRequested,
            this, [](QWebEnginePermission perm) {
        perm.deny();
    });

    // ── Inject anti-fingerprinting JS + cosmetic CSS after every page load ──
    connect(m_engineView, &QWebEngineView::loadFinished,
            this, [this](bool ok) {
        if (ok && m_antiFingerprintEnabled) {
            if (!m_antiFingerprintScript.isEmpty()) {
                m_engineView->page()->runJavaScript(m_antiFingerprintScript);
            }
            if (!m_cosmeticBlockerScript.isEmpty()) {
                m_engineView->page()->runJavaScript(m_cosmeticBlockerScript);
            }
        }
    });

    // ── Connect signals ──\

    // ── Connect signals ──
    connect(m_engineView, &QWebEngineView::urlChanged,
            this, &WebView::onEngineUrlChanged);
    connect(m_engineView, &QWebEngineView::titleChanged,
            this, &WebView::onEngineTitleChanged);
    connect(m_engineView, &QWebEngineView::loadStarted,
            this, &WebView::onEngineLoadStarted);
    connect(m_engineView, &QWebEngineView::loadFinished,
            this, &WebView::onEngineLoadFinished);
    connect(m_engineView, &QWebEngineView::loadProgress,
            this, &WebView::onEngineLoadProgress);

    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(200, 100);

    // Apply initial privacy settings
    updatePrivacySettings();
}

WebView::~WebView()
{
    if (m_engineView) {
        // Detach old page so it can be deleted independently
        auto *oldPage = m_engineView->page();
        if (oldPage) {
            m_engineView->setPage(nullptr);
            // If the old page still references our profile, we need to
            // ensure it's deleted before the profile. schedule it now.
            oldPage->deleteLater();
        }
    }
    if (m_profile) {
        m_profile->clearHttpCache();
        m_profile->cookieStore()->deleteSessionCookies();
    }
}

void WebView::updatePrivacySettings()
{
    auto &settings = SettingsManager::instance();

    m_interceptor->setTrackingBlockerEnabled(settings.isTrackingBlockerEnabled());
    m_interceptor->setHttpsOnlyEnabled(settings.isHttpsOnly());
    m_interceptor->setGpcEnabled(settings.isGpcEnabled());
    m_interceptor->setReferrerPolicyEnabled(true);

    TrackingBlocker::instance().setEnabled(settings.isTrackingBlockerEnabled());
    GpcHeader::instance().setEnabled(settings.isGpcEnabled());
    DnsResolver::instance().setEnabled(settings.isDohEnabled());
    FingerprintingDefender::instance().setEnabled(
        settings.isFingerprintingProtectionEnabled());
    ReferrerPolicy::instance().setPolicy(
        ReferrerPolicy::Policy::StrictOriginWhenCrossOrigin);

    m_antiFingerprintEnabled = settings.isAntiFingerprintEnabled();
}

// ── Privacy helpers ──

QUrl WebView::cleanUrl(const QUrl &url) const
{
    return UrlCleaner::cleanTrackingParams(url);
}

bool WebView::shouldBlockRequest(const QUrl &requestUrl) const
{
    return TrackingBlocker::instance().isTracker(requestUrl, QUrl());
}

// ── Navigation ──

void WebView::loadHtml(const QString &html, const QUrl &baseUrl)
{
    if (html.isEmpty()) return;
    m_engineView->setHtml(html, baseUrl);
    m_currentUrl = baseUrl.isValid() ? baseUrl : QUrl("about:blank");
    emit urlChanged(m_currentUrl);
}

void WebView::loadUrl(const QUrl &url)
{
    if (!url.isValid() || url.toString().isEmpty()) return;

    // Step 1: Clean tracking params
    QUrl clean = cleanUrl(url);

    // Handle about: pages via WebEngine (it handles them natively)
    if (clean.scheme() == "about") {
        m_engineView->setUrl(clean);
        m_currentUrl = clean;
        emit urlChanged(clean);
        emit titleChanged("about:blank");
        emit loadFinished(true);
        return;
    }

    // Check tracker blocking for the main frame URL
    if (shouldBlockRequest(clean)) {
        QString blockedHtml =
            "<html><body style='background:#1e1e2e; color:#cdd6f4; "
            "font-family:sans-serif; display:flex; align-items:center; "
            "justify-content:center; height:100vh; margin:0;'>"
            "<div style='text-align:center; max-width:600px;'>"
            "<div style='font-size:4em; margin-bottom:0.5em;'>&#x1F6AB;</div>"
            "<h2 style='color:#f38ba8;'>Blocked for Privacy</h2>"
            "<p style='color:#bac2de;'>Frint blocked this tracker domain.</p>"
            "<p style='color:#585b70; font-size:0.85em;'>"
            "Domain: " + clean.host() + "</p></div></body></html>";
        m_engineView->setHtml(blockedHtml, QUrl("https://blocked.frint"));
        m_currentUrl = clean;
        emit urlChanged(clean);
        emit loadFinished(true);
        return;
    }

    // Maintain navigation history
    if (m_currentUrl.isValid() && m_currentUrl.scheme() != "about") {
        m_backHistory.push(m_currentUrl);
        while (m_backHistory.size() > MAX_HISTORY) m_backHistory.removeFirst();
        m_forwardHistory.clear();
    }

    m_currentUrl = clean;
    m_engineView->setUrl(clean);
}

void WebView::reload() { m_engineView->reload(); }
void WebView::stop() { m_engineView->stop(); m_isLoading = false; emit statusBarMessage("Stopped"); }

void WebView::goBack()
{
    if (m_engineView->history()->canGoBack()) m_engineView->back();
    else if (!m_backHistory.isEmpty()) {
        m_navigatingHistory = true;
        QUrl prev = m_backHistory.pop();
        m_forwardHistory.push(m_currentUrl);
        loadUrl(prev);
        m_navigatingHistory = false;
    }
}

void WebView::goForward()
{
    if (m_engineView->history()->canGoForward()) m_engineView->forward();
    else if (!m_forwardHistory.isEmpty()) {
        m_navigatingHistory = true;
        QUrl next = m_forwardHistory.pop();
        m_backHistory.push(m_currentUrl);
        loadUrl(next);
        m_navigatingHistory = false;
    }
}

bool WebView::canGoBack() const { return m_engineView->history()->canGoBack() || !m_backHistory.isEmpty(); }
bool WebView::canGoForward() const { return m_engineView->history()->canGoForward() || !m_forwardHistory.isEmpty(); }
void WebView::clearHistory() { m_backHistory.clear(); m_forwardHistory.clear(); }

QUrl WebView::currentUrl() const { return m_engineView->url().isValid() ? m_engineView->url() : m_currentUrl; }
QString WebView::title() const { return m_engineView->title().isEmpty() ? m_title : m_engineView->title(); }
int WebView::loadProgress() const { return m_loadProgress; }
bool WebView::isLoading() const { return m_isLoading; }

// ── Engine signal handlers ──

void WebView::onEngineUrlChanged(const QUrl &url) { m_currentUrl = url; emit urlChanged(url); }
void WebView::onEngineTitleChanged(const QString &title) { m_title = title; emit titleChanged(title); }

void WebView::onEngineLoadStarted()
{
    m_isLoading = true;
    m_loadProgress = 0;
    emit loadStarted();
    emit statusBarMessage("Loading " + m_currentUrl.host() + "...");
}

void WebView::onEngineLoadFinished(bool ok)
{
    m_isLoading = false;
    emit statusBarMessage(ok ? ("Done \u2014 " + m_currentUrl.host()) : "Error loading page");
    emit loadFinished(ok);
}

void WebView::onEngineLoadProgress(int progress) { m_loadProgress = progress; emit loadProgress(progress); }

} // namespace Frint
