#include "browser_window.h"
#include "web_view.h"
#include <QGraphicsOpacityEffect>
#include "settings_manager.h"
#include "settings_dialog.h"
#include <QWebEngineDownloadRequest>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <cpuid.h>
#include <GL/gl.h>
#include "privacy/tracking_blocker.h"
#include "privacy/gpc_header.h"
#include "privacy/fingerprinting_defender.h"
#include "privacy/privacy_url_interceptor.h"
#include "bookmarks.h"
#include "history_manager.h"
#include "reading_list.h"
#include "note_taking.h"
#include "quick_dial.h"
#include "screenshot.h"
#include "cookie_manager.h"
#include "autofill.h"

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QClipboard>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QShortcut>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QStyle>
#include <QToolTip>
#include <QDesktopServices>
#include <QWebEngineSettings>
#include <QWebEnginePage>
#include <QWebEngineView>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QDialog>
#include <QTextEdit>
#include <QListWidget>
#include <QTreeWidget>
#include <QHeaderView>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QWidgetAction>
#include <QProcess>

namespace Frint {

// ── CPU Feature Detection ────────────────────────────────────────────
static QString detectCPUFeatures()
{
    QStringList features;
#ifdef __x86_64__
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        if (ecx & (1 << 20)) features << "SSE4.2";
    }
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        if (ebx & (1 << 5))  features << "AVX2";
        if (ebx & (1 << 16)) features << "AVX-512F";
        if (ebx & (1 << 28)) features << "AVX-512VPOPCNTDQ";
        if (ebx & (1 << 31)) features << "AVX-512VL";
    }
    unsigned int eax1, ebx1, ecx1, edx1;
    if (__get_cpuid(1, &eax1, &ebx1, &ecx1, &edx1)) {
        if (ecx1 & (1 << 28) && !features.contains("AVX2")) features << "AVX";
    }
#else
    QFile cpuinfo("/proc/cpuinfo");
    if (cpuinfo.open(QIODevice::ReadOnly)) {
        QString data = cpuinfo.readAll();
        QStringList flags;
        for (const QString &line : data.split('\n')) {
            if (line.trimmed().startsWith("flags\t")) {
                flags = line.section(':', 1).trimmed().split(' ', Qt::SkipEmptyParts);
                break;
            }
        }
        if (flags.contains("sse4_2")) features << "SSE4.2";
        if (flags.contains("avx"))   features << "AVX";
        if (flags.contains("avx2"))  features << "AVX2";
        if (flags.contains("avx512f")) features << "AVX-512F";
        cpuinfo.close();
    }
#endif
    if (features.isEmpty()) features << "Basic (SSE2)";
    return features.join(", ");
}

static QString detectGPUInfo()
{
    QOpenGLContext ctx;
    if (ctx.create()) {
        QSurfaceFormat fmt = ctx.format();
        const char *renderer = (const char *)glGetString(GL_RENDERER);
        const char *version  = (const char *)glGetString(GL_VERSION);
        const char *vendor   = (const char *)glGetString(GL_VENDOR);
        if (renderer && version && vendor)
            return QString("%1: %2\nOpenGL: %3").arg(vendor, renderer, version);
    }
    QFile gpuInfo("/sys/class/drm/card0/device/uevent");
    if (gpuInfo.open(QIODevice::ReadOnly)) {
        QString data = gpuInfo.readAll();
        for (const QString &line : data.split('\n')) {
            if (line.startsWith("DRIVER=")) return "GPU: " + line.mid(7);
        }
    }
    return "Unknown GPU";
}

static QString s_baseStyleSheet;

static QString loadThemeCss(const QString &name)
{
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList paths = {
        appDir + "/../../store/themes/" + name + ".css",
        appDir + "/../store/themes/" + name + ".css",
        appDir + "/store/themes/" + name + ".css",
        SettingsManager::instance().frintDataDir() + "/themes/" + name + ".css",
    };
    for (const auto &p : paths) {
        QFile f(p);
        if (f.open(QIODevice::ReadOnly))
            return f.readAll();
    }
    return QString();
}

BrowserWindow::BrowserWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Frint Browser");
    resize(1280, 800);
    setMinimumSize(640, 480);
    setWindowIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon));

    // ── Tab widget ──
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setElideMode(Qt::ElideRight);
    setCentralWidget(m_tabWidget);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &BrowserWindow::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &BrowserWindow::onCloseTab);

    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupStyleSheet();
    setupNotifications();
    setupAutoUpdateCheck();
    setupConnectivityMonitor();

    auto &settings = SettingsManager::instance();

    if (settings.searchEngine().isEmpty())
        settings.setSearchEngine("https://duckduckgo.com/?q=");

    // ── Entrance animation ──
    auto *entranceEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(entranceEffect);
    entranceEffect->setOpacity(0.0);
    auto *entranceAnim = new QPropertyAnimation(entranceEffect, "opacity", this);
    entranceAnim->setDuration(400);
    entranceAnim->setStartValue(0.0);
    entranceAnim->setEndValue(1.0);
    entranceAnim->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(entranceAnim, &QPropertyAnimation::finished, this, [this]() {
        setGraphicsEffect(nullptr);
    });
    entranceAnim->start(QAbstractAnimation::DeleteWhenStopped);

    applyTheme(settings.theme());
    applyPerformanceSettings();

    // Connect download tracking
    QWebEngineProfile *defaultProfile = QWebEngineProfile::defaultProfile();
    connect(defaultProfile, &QWebEngineProfile::downloadRequested,
            this, [this](QWebEngineDownloadRequest *download) {
        m_downloadProgressBar->setVisible(true);
        m_downloadProgressBar->setValue(0);
        QString fileName = download->downloadFileName();
        m_downloadProgressBar->setFormat(fileName.left(20) + (fileName.length()>20?"...":"") + " %p%");

        connect(download, &QWebEngineDownloadRequest::receivedBytesChanged, this,
            [this, download, fileName]() {
                qint64 received = download->receivedBytes();
                qint64 total = download->totalBytes();
                if (total > 0) {
                    int pct = static_cast<int>(received * 100 / total);
                    m_downloadProgressBar->setValue(pct);
                    m_downloadProgressBar->setFormat(
                        fileName.left(16) + (fileName.length()>16?"...":"") + " " + QString::number(pct) + "%");
                }
            });

        connect(download, &QWebEngineDownloadRequest::isFinishedChanged, this,
            [this, download]() {
                if (download->isFinished()) {
                    m_downloadProgressBar->setVisible(false);
                    showNotification("Download Complete",
                        download->downloadFileName() + " finished");
                }
            });

        download->accept();
    });

    // ── Shortcuts ──
    auto *st = new QShortcut(QKeySequence("Ctrl+T"), this);
    connect(st, &QShortcut::activated, this, &BrowserWindow::onNewTab);
    auto *sc = new QShortcut(QKeySequence("Ctrl+W"), this);
    connect(sc, &QShortcut::activated, this, [this]() {
        int idx = m_tabWidget->currentIndex();
        if (idx >= 0) onCloseTab(idx);
    });
    auto *sr = new QShortcut(QKeySequence("Ctrl+R"), this);
    connect(sr, &QShortcut::activated, this, &BrowserWindow::onReload);
    auto *sl = new QShortcut(QKeySequence("Ctrl+L"), this);
    connect(sl, &QShortcut::activated, this, [this]() { m_addressBar->setFocus(); m_addressBar->selectAll(); });
    auto *sq = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(sq, &QShortcut::activated, qApp, &QApplication::quit);
    auto *splus = new QShortcut(QKeySequence("Ctrl+="), this);
    connect(splus, &QShortcut::activated, this, &BrowserWindow::onZoomIn);
    auto *sminus = new QShortcut(QKeySequence("Ctrl+-"), this);
    connect(sminus, &QShortcut::activated, this, &BrowserWindow::onZoomOut);
    auto *s0 = new QShortcut(QKeySequence("Ctrl+0"), this);
    connect(s0, &QShortcut::activated, this, &BrowserWindow::onResetZoom);
    auto *sb = new QShortcut(QKeySequence("Alt+Left"), this);
    connect(sb, &QShortcut::activated, this, &BrowserWindow::onBack);
    auto *sf = new QShortcut(QKeySequence("Alt+Right"), this);
    connect(sf, &QShortcut::activated, this, &BrowserWindow::onForward);
    auto *sh = new QShortcut(QKeySequence("Alt+Home"), this);
    connect(sh, &QShortcut::activated, this, &BrowserWindow::onHome);

    // === NEW SHORTCUTS ===
    auto *sBookmark = new QShortcut(QKeySequence("Ctrl+D"), this);
    connect(sBookmark, &QShortcut::activated, this, &BrowserWindow::onAddBookmark);
    auto *sHistory = new QShortcut(QKeySequence("Ctrl+H"), this);
    connect(sHistory, &QShortcut::activated, this, &BrowserWindow::onShowHistory);
    auto *sClearData = new QShortcut(QKeySequence("Ctrl+Shift+Delete"), this);
    connect(sClearData, &QShortcut::activated, this, &BrowserWindow::onClearDataShortcut);
    auto *sScreenshot = new QShortcut(QKeySequence("Ctrl+Shift+S"), this);
    connect(sScreenshot, &QShortcut::activated, this, &BrowserWindow::onScreenshot);
    auto *sNight = new QShortcut(QKeySequence("Ctrl+Shift+N"), this);
    connect(sNight, &QShortcut::activated, this, &BrowserWindow::onToggleNightMode);
    auto *sReading = new QShortcut(QKeySequence("Ctrl+Shift+L"), this);
    connect(sReading, &QShortcut::activated, this, &BrowserWindow::onAddToReadingList);

    // Performance shortcut: Ctrl+Shift+P
    auto *sPerf = new QShortcut(QKeySequence("Ctrl+Shift+P"), this);
    connect(sPerf, &QShortcut::activated, this, &BrowserWindow::onShowPerformanceStatus);

    // Page load timer
    m_pageLoadTimer = new QTimer(this);
    m_pageLoadTimer->setSingleShot(true);

    // ── Initial tab ──
    addTab(QUrl(settings.homePage()));
    updatePrivacyIndicators();
    updateTabCountTitle();
    updateConnectionStatus();

    // Show download notification on first launch
    showNotification("Frint Browser", "Privacy-first browser ready", QSystemTrayIcon::Information);
}

BrowserWindow::~BrowserWindow()
{
    auto &settings = SettingsManager::instance();
    if (settings.isClearOnExit()) clearAllSessionData();
    settings.sync();
}

void BrowserWindow::clearAllSessionData()
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *wv = qobject_cast<WebView *>(m_tabWidget->widget(i));
        if (wv && wv->page()) {
            auto *profile = wv->page()->profile();
            profile->clearHttpCache();
            profile->cookieStore()->deleteAllCookies();
            profile->cookieStore()->deleteSessionCookies();
            profile->clearAllVisitedLinks();
            profile->setHttpCacheType(QWebEngineProfile::NoCache);
            profile->setHttpCacheMaximumSize(0);
        }
    }
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *wv = qobject_cast<WebView *>(m_tabWidget->widget(i));
        if (wv && wv->page()) {
            wv->page()->runJavaScript(
                "try { localStorage.clear(); } catch(e) {}"
                "try { sessionStorage.clear(); } catch(e) {}"
                "window.name = '';");
        }
    }
    CookieManager::instance().clearCookies();
    qDebug() << "[Frint] Cleared all session data";
}

// ═══════════════════════════════════════════════════════════════════════
// CLOSE EVENT — Exit Confirmation + Close Empty Tab
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::closeEvent(QCloseEvent *event)
{
    auto &settings = SettingsManager::instance();

    // Exit Confirmation
    if (settings.getValue("exit_confirmation", true).toBool()) {
        auto reply = QMessageBox::question(this, "Exit Frint Browser?",
            "Are you sure you want to close?\n\nOpen tabs: " +
            QString::number(m_tabWidget->count()) + "\n\n"
            "Tip: You can disable this in Settings.",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            event->ignore();
            return;
        }
    }

    if (settings.isClearOnExit()) clearAllSessionData();
    settings.sync();

    while (m_tabWidget->count() > 0) {
        QWidget *w = m_tabWidget->widget(0);
        m_tabWidget->removeTab(0);
        w->deleteLater();
    }
    event->accept();
}

// ═══════════════════════════════════════════════════════════════════════
// MENU BAR
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::setupMenuBar()
{
    // ── File Menu ──
    m_fileMenu = menuBar()->addMenu("&File");
    auto *newTab = m_fileMenu->addAction("&New Tab");
    newTab->setShortcut(QKeySequence::AddTab);
    connect(newTab, &QAction::triggered, this, &BrowserWindow::onNewTab);

    auto *closeTab = m_fileMenu->addAction("&Close Tab");
    closeTab->setShortcut(QKeySequence::Close);
    connect(closeTab, &QAction::triggered, this, [this]() {
        int idx = m_tabWidget->currentIndex();
        if (idx >= 0) onCloseTab(idx);
    });
    m_fileMenu->addSeparator();

    auto *newWindow = m_fileMenu->addAction("&New Window");
    connect(newWindow, &QAction::triggered, this, []() {
        QProcess::startDetached(QApplication::applicationFilePath());
    });

    auto *quickDial = m_fileMenu->addAction("&Quick Dial");
    quickDial->setShortcut(QKeySequence("Ctrl+1"));
    connect(quickDial, &QAction::triggered, this, &BrowserWindow::onShowQuickDial);

    // Bookmarks submenu in File
    m_bookmarksMenu = m_fileMenu->addMenu("&Bookmarks");
    auto *addBm = m_bookmarksMenu->addAction("&Add Bookmark");
    addBm->setShortcut(QKeySequence("Ctrl+D"));
    connect(addBm, &QAction::triggered, this, &BrowserWindow::onAddBookmark);
    auto *showBm = m_bookmarksMenu->addAction("&Show All Bookmarks");
    connect(showBm, &QAction::triggered, this, &BrowserWindow::onShowBookmarks);
    m_bookmarksMenu->addSeparator();
    auto *impBm = m_bookmarksMenu->addAction("&Import Bookmarks...");
    connect(impBm, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Import Bookmarks", QString(),
            "Bookmarks (*.html *.json);;All Files (*)");
        if (!path.isEmpty()) {
            if (path.endsWith(".json"))
                BookmarkManager::instance().importFromJson(path);
            else
                BookmarkManager::instance().importFromHtml(path);
            showNotification("Bookmarks", "Bookmarks imported successfully");
        }
    });
    auto *expBm = m_bookmarksMenu->addAction("&Export Bookmarks...");
    connect(expBm, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, "Export Bookmarks", "bookmarks.html",
            "HTML (*.html);;JSON (*.json)");
        if (!path.isEmpty()) {
            if (path.endsWith(".json"))
                BookmarkManager::instance().exportToJson(path);
            else
                BookmarkManager::instance().exportToHtml(path);
            showNotification("Bookmarks", "Bookmarks exported to " + path);
        }
    });

    // History
    auto *histAct = m_fileMenu->addAction("&History");
    histAct->setShortcut(QKeySequence("Ctrl+H"));
    connect(histAct, &QAction::triggered, this, &BrowserWindow::onShowHistory);

    m_fileMenu->addSeparator();

    auto *settingsAct = m_fileMenu->addAction("&Settings");
    connect(settingsAct, &QAction::triggered, this, &BrowserWindow::onShowSettings);
    m_fileMenu->addSeparator();
    auto *quit = m_fileMenu->addAction("&Quit");
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, qApp, &QApplication::quit);

    // ── Privacy Menu ──
    m_privacyMenu = menuBar()->addMenu("&Privacy");
    m_trackingAction = m_privacyMenu->addAction("&Tracker Blocker");
    m_trackingAction->setCheckable(true);
    connect(m_trackingAction, &QAction::triggered, this, &BrowserWindow::onToggleTrackingBlocker);
    m_httpsAction = m_privacyMenu->addAction("HTTPS-&Only");
    m_httpsAction->setCheckable(true);
    connect(m_httpsAction, &QAction::triggered, this, &BrowserWindow::onToggleHttpsOnly);
    m_gpcAction = m_privacyMenu->addAction("&GPC");
    m_gpcAction->setCheckable(true);
    connect(m_gpcAction, &QAction::triggered, this, &BrowserWindow::onToggleGpc);
    m_fingerprintingAction = m_privacyMenu->addAction("&Fingerprint Defense");
    m_fingerprintingAction->setCheckable(true);
    connect(m_fingerprintingAction, &QAction::triggered, this, &BrowserWindow::onToggleFingerprinting);
    m_privacyMenu->addSeparator();
    m_antiFingerprintAction = m_privacyMenu->addAction("Anti-Fingerprinting JS");
    m_antiFingerprintAction->setCheckable(true);
    connect(m_antiFingerprintAction, &QAction::triggered, this, [this]() {
        auto &s = SettingsManager::instance();
        s.setAntiFingerprintEnabled(!s.isAntiFingerprintEnabled());
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            if (auto *wv = qobject_cast<WebView *>(m_tabWidget->widget(i)))
                wv->updatePrivacySettings();
        }
    });
    m_webrtcAction = m_privacyMenu->addAction("Block &WebRTC");
    m_webrtcAction->setCheckable(true);
    connect(m_webrtcAction, &QAction::triggered, this, [this]() {
        auto &s = SettingsManager::instance();
        s.setWebrtcBlocked(!s.isWebrtcBlocked());
    });
    m_etagAction = m_privacyMenu->addAction("Strip &ETags");
    m_etagAction->setCheckable(true);
    connect(m_etagAction, &QAction::triggered, this, [this]() {
        auto &s = SettingsManager::instance();
        s.setEtagTrackingBlocked(!s.isEtagTrackingBlocked());
    });
    m_privacyMenu->addSeparator();
    auto *cs = m_privacyMenu->addAction("Clear All &Storage");
    connect(cs, &QAction::triggered, this, &BrowserWindow::onClearStorage);
    auto *autoCookie = m_privacyMenu->addAction("Auto-Clear Cookies Every 30min");
    autoCookie->setCheckable(true);
    autoCookie->setChecked(CookieManager::instance().isAutoClearActive());
    connect(autoCookie, &QAction::triggered, this, &BrowserWindow::onToggleAutoCookieClear);

    // ── Tools Menu ──
    m_toolsMenu = menuBar()->addMenu("&Tools");
    auto *notesAct = m_toolsMenu->addAction("&Notes");
    connect(notesAct, &QAction::triggered, this, &BrowserWindow::onShowNotes);
    auto *readingAct = m_toolsMenu->addAction("&Reading List");
    connect(readingAct, &QAction::triggered, this, &BrowserWindow::onShowReadingList);
    m_toolsMenu->addSeparator();
    m_translateAction = m_toolsMenu->addAction("&Translate Page");
    connect(m_translateAction, &QAction::triggered, this, &BrowserWindow::onTranslatePage);
    m_nightModeAction = m_toolsMenu->addAction("&Night Mode");
    m_nightModeAction->setCheckable(true);
    connect(m_nightModeAction, &QAction::triggered, this, &BrowserWindow::onToggleNightMode);
    m_spellCheckAction = m_toolsMenu->addAction("&Spell Check");
    m_spellCheckAction->setCheckable(true);
    m_spellCheckAction->setChecked(true);
    connect(m_spellCheckAction, &QAction::triggered, this, [this]() {
        bool enabled = m_spellCheckAction->isChecked();
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            if (auto *wv = qobject_cast<WebView *>(m_tabWidget->widget(i))) {
                if (wv->page())
                    wv->page()->profile()->setSpellCheckEnabled(enabled);
            }
        }
    });
    m_toolsMenu->addSeparator();
    auto *screenshotAct = m_toolsMenu->addAction("&Screenshot (Visible)");
    screenshotAct->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(screenshotAct, &QAction::triggered, this, &BrowserWindow::onScreenshot);
    auto *voiceAct = m_toolsMenu->addAction("&Voice Search");
    connect(voiceAct, &QAction::triggered, this, &BrowserWindow::onVoiceSearch);
    m_toolsMenu->addSeparator();
    auto *shortcutsAct = m_toolsMenu->addAction("&Keyboard Shortcuts...");
    connect(shortcutsAct, &QAction::triggered, this, &BrowserWindow::onShowShortcuts);
    m_toolsMenu->addSeparator();

    // ── Performance Submenu ──
    auto *perfMenu = m_toolsMenu->addMenu(QString::fromUtf8("\xF0\x9F\x9A\x80 Performance"));
    m_hwAccelAction = perfMenu->addAction("\u2611 Hardware Acceleration");
    m_hwAccelAction->setCheckable(true);
    connect(m_hwAccelAction, &QAction::triggered, this, &BrowserWindow::onToggleHardwareAcceleration);

    m_sseAvxAction = perfMenu->addAction("\u2611 SSE/AVX Optimizations");
    m_sseAvxAction->setCheckable(true);
    connect(m_sseAvxAction, &QAction::triggered, this, &BrowserWindow::onToggleSSEAVX);

    perfMenu->addSeparator();
    auto *perfStatusAct = perfMenu->addAction("\xF0\x9F\x93\x8A Performance Status...");
    connect(perfStatusAct, &QAction::triggered, this, &BrowserWindow::onShowPerformanceStatus);

    // ── Hamburger Menu ──
    m_hamburgerMenu = new QMenu(this);
    m_hamburgerMenu->addAction("&New Tab", QKeySequence::AddTab, this, &BrowserWindow::onNewTab);
    m_hamburgerMenu->addAction("&Quick Dial", QKeySequence("Ctrl+1"), this, &BrowserWindow::onShowQuickDial);
    m_hamburgerMenu->addSeparator();
    m_hamburgerMenu->addAction("&Settings", this, &BrowserWindow::onShowSettings);
    m_hamburgerMenu->addSeparator();
    m_hamburgerMenu->addAction("Zoom &In", QKeySequence("Ctrl+="), this, &BrowserWindow::onZoomIn);
    m_hamburgerMenu->addAction("Zoom &Out", QKeySequence("Ctrl+-"), this, &BrowserWindow::onZoomOut);
    m_hamburgerMenu->addAction("&Reset Zoom", QKeySequence("Ctrl+0"), this, &BrowserWindow::onResetZoom);
    m_hamburgerMenu->addSeparator();
    m_hamburgerMenu->addAction(QString::fromUtf8("\xF0\x9F\x9A\x80 Performance Status..."), this, &BrowserWindow::onShowPerformanceStatus);
    m_hamburgerMenu->addSeparator();
    m_hamburgerMenu->addAction("&Downloads", this, &BrowserWindow::onShowDownloads);
    m_hamburgerMenu->addAction("&History", this, &BrowserWindow::onShowHistory);
    m_hamburgerMenu->addAction("&Bookmarks", this, &BrowserWindow::onShowBookmarks);
    m_hamburgerMenu->addAction("&Reading List", this, &BrowserWindow::onShowReadingList);
    m_hamburgerMenu->addAction("&Notes", this, &BrowserWindow::onShowNotes);
    m_hamburgerMenu->addSeparator();
    m_hamburgerMenu->addAction("&Night Mode", this, &BrowserWindow::onToggleNightMode);

    m_profileMenu = m_hamburgerMenu->addMenu("&Profiles");
    auto &settings = SettingsManager::instance();
    for (const auto &p : settings.profiles()) {
        auto *pa = m_profileMenu->addAction(p);
        connect(pa, &QAction::triggered, this, [this, p]() {
            SettingsManager::instance().setActiveProfile(p);
            onSwitchProfile();
        });
    }
    QString active = settings.activeProfile();
    for (auto *a : m_profileMenu->actions()) {
        if (a->text() == active) a->setCheckable(true);
    }

    m_hamburgerMenu->addSeparator();
    auto *restartAct = m_hamburgerMenu->addAction("&Restart Browser");
    connect(restartAct, &QAction::triggered, this, &BrowserWindow::onRestartBrowser);
    m_hamburgerMenu->addAction("&About Frint", this, &BrowserWindow::onAboutFrint);
    m_hamburgerMenu->addAction("&Quit", QKeySequence::Quit, qApp, &QApplication::quit);

    // Extensions menu
    m_extensionsMenu = new QMenu("Extensions", this);
    m_extensionsMenu->addAction("Browse Frint Plugin Store...", this, [this]() {
        QDesktopServices::openUrl(QUrl("https://github.com/qfdevel/Frint-Plugin-Theme-Store"));
    });
    m_extensionsMenu->addAction("View Approved Plugins...", this, [this]() {
        QDesktopServices::openUrl(QUrl("https://github.com/qfdevel/Frint-Plugin-Theme-Store/tree/main/approved"));
    });
    m_extensionsMenu->addSeparator();
    m_extensionsMenu->addAction("Local Extensions Folder", this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(
            SettingsManager::instance().frintDataDir() + "/extensions"));
    });
}

// ═══════════════════════════════════════════════════════════════════════
// TOOLBAR
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::setupToolBar()
{
    m_toolBar = new QToolBar("Navigation", this);
    m_toolBar->setMovable(false);
    m_toolBar->setIconSize(QSize(18, 18));
    addToolBar(m_toolBar);

    auto makeBtn = [this](const QString &text, const QString &tip) {
        auto *btn = new QToolButton(this);
        btn->setText(text);
        btn->setToolTip(tip);
        btn->setAutoRaise(true);
        btn->setFixedSize(34, 34);
        btn->setCursor(Qt::PointingHandCursor);
        return btn;
    };

    m_backBtn = makeBtn(QChar(0x25C0), "Back (Alt+Left)");
    connect(m_backBtn, &QToolButton::clicked, this, &BrowserWindow::onBack);
    m_toolBar->addWidget(m_backBtn);

    m_forwardBtn = makeBtn(QChar(0x25B6), "Forward (Alt+Right)");
    connect(m_forwardBtn, &QToolButton::clicked, this, &BrowserWindow::onForward);
    m_toolBar->addWidget(m_forwardBtn);

    m_reloadBtn = makeBtn(QChar(0x21BB), "Reload (Ctrl+R)");
    connect(m_reloadBtn, &QToolButton::clicked, this, &BrowserWindow::onReload);
    m_toolBar->addWidget(m_reloadBtn);

    m_homeBtn = makeBtn(QChar(0x2302), "Home (Alt+Home)");
    connect(m_homeBtn, &QToolButton::clicked, this, &BrowserWindow::onHome);
    m_toolBar->addWidget(m_homeBtn);

    m_nightModeBtn = makeBtn(QChar(0x1F319), "Night Mode (Ctrl+Shift+N)");
    connect(m_nightModeBtn, &QToolButton::clicked, this, &BrowserWindow::onToggleNightMode);
    m_toolBar->addWidget(m_nightModeBtn);

    m_toolBar->addSeparator();

    // Security lock icon
    m_securityLock = new QLabel(QString::fromUtf8("\xF0\x9F\x94\x92"), this);
    m_securityLock->setToolTip("Connection secure (HTTPS)");
    m_securityLock->setStyleSheet("font-size: 15px; padding: 0 4px;");
    m_toolBar->addWidget(m_securityLock);

    // Address bar
    m_addressBar = new QLineEdit(this);
    m_addressBar->setPlaceholderText("Search or enter URL...");
    m_addressBar->setClearButtonEnabled(true);
    m_addressBar->setMinimumWidth(300);
    m_addressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_addressBar->installEventFilter(this);
    connect(m_addressBar, &QLineEdit::returnPressed, this, &BrowserWindow::onAddressBarReturnPressed);
    m_toolBar->addWidget(m_addressBar);

    // Bookmark button (star)
    m_bookmarkBtn = makeBtn(QString::fromUtf8("\xE2\x98\x85"), "Bookmark (Ctrl+D)");
    connect(m_bookmarkBtn, &QToolButton::clicked, this, &BrowserWindow::onAddBookmark);
    m_toolBar->addWidget(m_bookmarkBtn);

    // Reading List button
    m_readingListBtn = makeBtn(QString::fromUtf8("\xF0\x9F\x93\x9A"), "Reading List");
    connect(m_readingListBtn, &QToolButton::clicked, this, &BrowserWindow::onAddToReadingList);
    m_toolBar->addWidget(m_readingListBtn);

    // Profile label
    m_profileLabel = new QLabel("👤 " + SettingsManager::instance().activeProfile(), this);
    m_profileLabel->setToolTip("Current profile. Click to switch.");
    m_profileLabel->setCursor(Qt::PointingHandCursor);
    m_profileLabel->setStyleSheet("color: #585b70; font-size: 11px; padding: 0 6px;");
    m_toolBar->addWidget(m_profileLabel);

    // Extensions button
    m_extensionsBtn = makeBtn("🧩", "Extensions");
    connect(m_extensionsBtn, &QToolButton::clicked, this, &BrowserWindow::onToggleExtensions);
    m_toolBar->addWidget(m_extensionsBtn);

    // New tab button
    m_newTabBtn = makeBtn("+", "New Tab (Ctrl+T)");
    m_newTabBtn->setFixedSize(30, 30);
    m_newTabBtn->setStyleSheet(
        "QToolButton { background: rgba(137,180,250,0.15); color: #89b4fa;"
        "  border: 1px solid rgba(137,180,250,0.2); border-radius: 15px;"
        "  font-size: 18px; font-weight: bold; }"
        "QToolButton:hover { background: rgba(137,180,250,0.35);"
        "  border-color: rgba(137,180,250,0.5); }"
        "QToolButton:pressed { background: rgba(137,180,250,0.5); }");
    m_newTabBtn->installEventFilter(this);
    connect(m_newTabBtn, &QToolButton::clicked, this, &BrowserWindow::onNewTab);
    m_toolBar->addWidget(m_newTabBtn);

    // Hamburger menu button
    m_menuBtn = makeBtn(QChar(0x2630), "Menu");
    m_menuBtn->installEventFilter(this);
    connect(m_menuBtn, &QToolButton::clicked, this, [this]() {
        m_hamburgerMenu->exec(m_menuBtn->mapToGlobal(
            QPoint(0, m_menuBtn->height())));
    });
    m_toolBar->addWidget(m_menuBtn);
}

// ═══════════════════════════════════════════════════════════════════════
// STATUS BAR
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("Ready", this);
    m_statusLabel->setStyleSheet("color: #a6adc8;");
    statusBar()->addWidget(m_statusLabel, 1);

    // Page load time
    m_pageLoadTimeLabel = new QLabel("", this);
    m_pageLoadTimeLabel->setStyleSheet("color: #585b70; font-size: 11px; padding: 0 8px;");
    statusBar()->addPermanentWidget(m_pageLoadTimeLabel);

    // Connection status
    m_connectionStatusLabel = new QLabel(QString::fromUtf8("\xF0\x9F\x93\xA1 Online"), this);
    m_connectionStatusLabel->setStyleSheet("color: #a6e3a1; font-size: 11px; padding: 0 6px;");
    statusBar()->addPermanentWidget(m_connectionStatusLabel);

    // Tracker counter
    m_trackerCountLabel = new QLabel(QString::fromUtf8("\xF0\x9F\x9B\x91 0 blocked"), this);
    m_trackerCountLabel->setStyleSheet("color: #585b70; font-size: 11px; padding: 0 6px;");
    statusBar()->addPermanentWidget(m_trackerCountLabel);

    // Performance indicator
    m_performanceIndicator = new QLabel(QString::fromUtf8("\xF0\x9F\x9F\xA2 GPU Active"), this);
    m_performanceIndicator->setStyleSheet("color: #a6e3a1; font-size: 11px; padding: 0 6px;");
    statusBar()->addPermanentWidget(m_performanceIndicator);

    m_privacyIndicator = new QLabel(QString::fromUtf8("\u2705 Privacy Active"), this);
    m_privacyIndicator->setStyleSheet("color: #a6e3a1; padding: 0 8px;");
    statusBar()->addPermanentWidget(m_privacyIndicator);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumWidth(120);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    m_progressBar->setTextVisible(false);
    statusBar()->addPermanentWidget(m_progressBar);

    // Download progress bar
    m_downloadProgressBar = new QProgressBar(this);
    m_downloadProgressBar->setMaximumWidth(150);
    m_downloadProgressBar->setMaximum(100);
    m_downloadProgressBar->setValue(0);
    m_downloadProgressBar->setVisible(false);
    m_downloadProgressBar->setTextVisible(true);
    m_downloadProgressBar->setFormat("Downloading: %p%");
    statusBar()->addPermanentWidget(m_downloadProgressBar);
}

void BrowserWindow::setupStyleSheet()
{
    s_baseStyleSheet = QString::fromUtf8(
        "QMainWindow { background: #1e1e2e; }"
        "QToolBar { background: rgba(24, 25, 37, 0.7); border: none; border-bottom: 1px solid rgba(49, 50, 68, 0.5);"
        "  padding: 4px 8px; spacing: 6px; border-radius: 0; }"
        "QToolButton { background: rgba(49, 50, 68, 0.2); color: #cdd6f4; border: 1px solid rgba(255,255,255,0.06);"
        "  border-radius: 10px; padding: 4px 10px; font-size: 14px; }"
        "QToolButton:hover { background: rgba(69, 71, 90, 0.5); border-color: rgba(255,255,255,0.12); }"
        "QToolButton:pressed { background: rgba(69, 71, 90, 0.7); }"
        "QToolButton:disabled { color: #45475a; }"
        "QLineEdit { background: rgba(49, 50, 68, 0.4); color: #cdd6f4; border: 2px solid rgba(69, 71, 90, 0.5);"
        "  border-radius: 12px; padding: 8px 16px; font-size: 13px; }"
        "QLineEdit:focus { border-color: rgba(137, 180, 250, 0.7); background: rgba(49, 50, 68, 0.6); }"
        "QTabWidget::pane { border: none; background: rgba(30, 30, 46, 0.5); }"
        "QTabBar::tab { background: rgba(24, 25, 37, 0.5); color: #585b70; border: 1px solid transparent;"
        "  padding: 6px 18px; margin: 2px 2px 0 2px;"
        "  border-top-left-radius: 10px; border-top-right-radius: 10px;"
        "  font-size: 12px; }"
        "QTabBar::tab:selected { background: rgba(30, 30, 46, 0.8); color: #cdd6f4; border-color: rgba(255,255,255,0.06); }"
        "QTabBar::tab:hover:!selected { background: rgba(49, 50, 68, 0.6); }"
        "QStatusBar { background: rgba(24, 25, 37, 0.7); color: #bac2de; border-top: 1px solid rgba(49, 50, 68, 0.5);"
        "  font-size: 11px; padding: 2px 8px; }"
        "QMenuBar { background: rgba(24, 25, 37, 0.6); color: #cdd6f4; border: none; padding: 2px; }"
        "QMenuBar::item:selected { background: rgba(49, 50, 68, 0.7); border-radius: 6px; }"
        "QMenu { background: rgba(30, 30, 46, 0.85); color: #cdd6f4; border: 1px solid rgba(255,255,255,0.06);"
        "  border-radius: 14px; padding: 6px; }"
        "QMenu::item { padding: 8px 30px; border-radius: 8px; }"
        "QMenu::item:selected { background: rgba(49, 50, 68, 0.7); }"
        "QMenu::item:checked { background: rgba(69, 71, 90, 0.5); }"
        "QMenu::separator { height: 1px; background: rgba(255,255,255,0.06); margin: 6px 12px; }"
        "QProgressBar { background: rgba(49, 50, 68, 0.3); border: 1px solid rgba(255,255,255,0.04); border-radius: 6px;"
        "  height: 6px; text-align: center; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "  stop:0 #89b4fa, stop:0.5 #a6e3a1, stop:1 #cba6f7);"
        "  border-radius: 6px; }"
        "QScrollBar:vertical { background: transparent; width: 8px; border: none;"
        "  border-radius: 4px; }"
        "QScrollBar::handle:vertical { background: rgba(69, 71, 90, 0.4); min-height: 30px;"
        "  border-radius: 6px; }"
        "QScrollBar::handle:vertical:hover { background: rgba(69, 71, 90, 0.7); }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QToolTip { background: rgba(49, 50, 68, 0.8); color: #cdd6f4; border: 1px solid rgba(255,255,255,0.06);"
        "  border-radius: 10px; padding: 8px 14px; font-size: 12px; }"
    );
    setStyleSheet(s_baseStyleSheet);
}

void BrowserWindow::applyTheme(const QString &themeName)
{
    QString themeCss = loadThemeCss(themeName);
    QString fullCss = s_baseStyleSheet;
    if (!themeCss.isEmpty()) {
        fullCss += "\n/* Theme: " + themeName + " */\n" + themeCss;
    }
    QString customCss = SettingsManager::instance().getValue("custom_css", "").toString();
    if (!customCss.trimmed().isEmpty()) {
        fullCss += "\n/* Custom CSS */\n" + customCss;
    }
    setStyleSheet(fullCss);
    qDebug() << "[Frint] Applied theme:" << themeName;
}

void BrowserWindow::animateProperty(QWidget *target, const QByteArray &prop,
                                     const QVariant &from, const QVariant &to, int duration)
{
    auto *anim = new QPropertyAnimation(target, prop, this);
    anim->setDuration(duration);
    anim->setStartValue(from);
    anim->setEndValue(to);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

bool BrowserWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Enter) {
        if (auto *btn = qobject_cast<QToolButton *>(obj)) {
            auto *effect = new QGraphicsOpacityEffect(btn);
            btn->setGraphicsEffect(effect);
            auto *anim = new QPropertyAnimation(effect, "opacity", this);
            anim->setDuration(200);
            anim->setStartValue(0.85);
            anim->setEndValue(1.0);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
    if (event->type() == QEvent::Leave) {
        if (auto *btn = qobject_cast<QToolButton *>(obj)) {
            if (auto *effect = btn->graphicsEffect()) {
                auto *anim = new QPropertyAnimation(effect, "opacity", this);
                anim->setDuration(250);
                anim->setStartValue(1.0);
                anim->setEndValue(0.85);
                anim->setEasingCurve(QEasingCurve::OutCubic);
                anim->start(QAbstractAnimation::DeleteWhenStopped);
            }
        }
    }
    if (event->type() == QEvent::FocusIn) {
        if (auto *le = qobject_cast<QLineEdit *>(obj)) {
            auto *effect = new QGraphicsOpacityEffect(le);
            le->setGraphicsEffect(effect);
            auto *anim = new QPropertyAnimation(effect, "opacity", this);
            anim->setDuration(300);
            anim->setStartValue(le->graphicsEffect() ? 1.0 : 0.7);
            anim->setEndValue(1.0);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// ═══════════════════════════════════════════════════════════════════════
// TAB MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::addTab(const QUrl &url)
{
    // If Quick Dial URL, load the generated HTML
    if (url.toString() == "frint://quickdial") {
        auto *wv = new WebView(this);
        int idx = m_tabWidget->addTab(wv, "Quick Dial");
        connect(wv, &WebView::urlChanged, this, &BrowserWindow::onUrlChanged);
        connect(wv, &WebView::titleChanged, this, &BrowserWindow::onTitleChanged);
        connect(wv, &WebView::loadStarted, this, [this]() { onLoadStarted(); });
        connect(wv, &WebView::loadFinished, this, [this](bool ok) { onLoadFinished(ok); });
        connect(wv, &WebView::statusBarMessage, m_statusLabel, &QLabel::setText);
        m_tabWidget->setCurrentIndex(idx);
        wv->loadHtml(QuickDial::instance().generateHtml(), QUrl("frint://quickdial/"));
        updateTabCountTitle();
        return;
    }

    auto *wv = new WebView(this);
    int idx = m_tabWidget->addTab(wv, "New Tab");

    connect(wv, &WebView::urlChanged, this, &BrowserWindow::onUrlChanged);
    connect(wv, &WebView::titleChanged, this, &BrowserWindow::onTitleChanged);
    connect(wv, &WebView::loadStarted, this, [this]() { onLoadStarted(); });

    if (auto *interceptor = wv->privacyInterceptor()) {
        connect(interceptor, &PrivacyUrlInterceptor::requestBlocked,
                this, [this](const QString &, const QString &) {
            m_blockedTrackerCount++;
            m_trackerCountLabel->setText(
                QString::fromUtf8("\xF0\x9F\x9B\x91 %1 blocked").arg(m_blockedTrackerCount));
        }, Qt::QueuedConnection);
    }
    connect(wv, &WebView::loadFinished, this, [this](bool ok) { onLoadFinished(ok); });
    connect(wv, &WebView::statusBarMessage, m_statusLabel, &QLabel::setText);
    m_tabWidget->setCurrentIndex(idx);
    if (url.isValid()) wv->loadUrl(url);
    updateTabCountTitle();
}

void BrowserWindow::removeTab(int index)
{
    // Close Empty Tab behavior: if closing last tab, exit or open new tab
    if (m_tabWidget->count() <= 1) {
        bool closeEmptyAction = SettingsManager::instance()
            .getValue("close_empty_tab_action", "exit").toString() == "exit";
        if (closeEmptyAction) {
            // Exit browser
            close();
            return;
        } else {
            // Open new tab before closing this one
            addTab(QUrl("about:blank"));
        }
    }

    QWidget *w = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    w->deleteLater();
    updateTabCountTitle();
}

WebView *BrowserWindow::currentWebView() const
{
    return qobject_cast<WebView *>(m_tabWidget->currentWidget());
}

// ═══════════════════════════════════════════════════════════════════════
// NAVIGATION SLOTS
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onNewTab() { addTab(QUrl("about:blank")); }

void BrowserWindow::onShowQuickDial() { addTab(QUrl("frint://quickdial")); }

void BrowserWindow::onCloseTab(int idx) { removeTab(idx); }

void BrowserWindow::onTabChanged(int)
{
    auto *wv = currentWebView();
    if (wv) {
        updateAddressBar(wv->currentUrl());
        updateNavButtons();
        updateTabCountTitle();
    }
}

void BrowserWindow::onUrlChanged(const QUrl &url)
{
    auto *wv = qobject_cast<WebView *>(sender());
    if (wv && wv == currentWebView()) updateAddressBar(url);
}

void BrowserWindow::onTitleChanged(const QString &title)
{
    auto *wv = qobject_cast<WebView *>(sender());
    if (!wv) return;
    int idx = m_tabWidget->indexOf(wv);
    if (idx >= 0) {
        m_tabWidget->setTabText(idx, title.left(24).isEmpty() ? "New Tab" : title.left(24));
        QString tabTip = title.isEmpty() ? "New Tab" : title;
        QUrl tabUrl = wv->currentUrl();
        if (tabUrl.isValid() && !tabUrl.toString().isEmpty()) {
            tabTip += "\n" + tabUrl.toString();
        }
        m_tabWidget->setTabToolTip(idx, tabTip);
    }

    // Update window title with tab count
    if (wv == currentWebView()) {
        int tabCount = m_tabWidget->count();
        QString titleStr = title.isEmpty() ? "Frint Browser" : title + " - Frint Browser";
        if (tabCount > 1)
            titleStr += " (" + QString::number(tabCount) + " tabs)";
        setWindowTitle(titleStr);
    }

    // Record in history
    if (wv) {
        QUrl u = wv->currentUrl();
        if (u.isValid() && u.scheme() != "about" && u.scheme() != "frint")
            HistoryManager::instance().addVisit(u, title);
    }
}

void BrowserWindow::onLoadStarted()
{
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_loadTimer.start();
    m_pageLoadTimeLabel->setText("Loading...");
}

void BrowserWindow::onLoadFinished(bool ok)
{
    m_progressBar->setVisible(false);

    if (ok && m_loadTimer.isValid()) {
        qint64 elapsed = m_loadTimer.elapsed();
        QString timeStr = elapsed < 1000
            ? QString::number(elapsed) + " ms"
            : QString::number(elapsed / 1000.0, 'f', 2) + " s";
        m_pageLoadTimeLabel->setText("⏱ " + timeStr);
    } else if (!ok) {
        m_pageLoadTimeLabel->setText("⏱ Error");
    }

    m_statusLabel->setText(ok ? "Done" : "Error loading page");
    updateNavButtons();

    // Apply autofill after page loads
    auto *wv = currentWebView();
    if (wv && wv->page()) {
        QString afJs = AutofillManager::instance().generateAutofillJs("profile");
        if (!afJs.isEmpty())
            wv->page()->runJavaScript(afJs);
    }
}

void BrowserWindow::onLoadProgress(int progress)
{
    m_progressBar->setValue(progress);
}

void BrowserWindow::onAddressBarReturnPressed()
{
    QString text = m_addressBar->text().trimmed();
    if (text.isEmpty()) return;

    QUrl url(text);
    if (!url.isValid() || url.scheme().isEmpty()) {
        QString searchUrl = SettingsManager::instance().searchEngine();
        url = QUrl(searchUrl + QUrl::toPercentEncoding(text));
    } else if (url.scheme().length() <= 1) {
        url = QUrl("https://" + text);
    }

    auto *wv = currentWebView();
    if (wv) wv->loadUrl(url);
}

void BrowserWindow::onBack() { if (auto *w = currentWebView()) w->goBack(); }
void BrowserWindow::onForward() { if (auto *w = currentWebView()) w->goForward(); }
void BrowserWindow::onReload() { if (auto *w = currentWebView()) w->reload(); }
void BrowserWindow::onHome()
{
    auto *wv = currentWebView();
    if (wv) wv->loadUrl(QUrl(SettingsManager::instance().homePage()));
}

void BrowserWindow::onZoomIn()
{
    if (auto *w = currentWebView()) w->engineView()->setZoomFactor(w->engineView()->zoomFactor() + 0.1);
}
void BrowserWindow::onZoomOut()
{
    if (auto *w = currentWebView()) w->engineView()->setZoomFactor(qMax(0.3, w->engineView()->zoomFactor() - 0.1));
}
void BrowserWindow::onResetZoom()
{
    if (auto *w = currentWebView()) w->engineView()->setZoomFactor(1.0);
}

// ═══════════════════════════════════════════════════════════════════════
// BOOKMARKS
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onAddBookmark()
{
    auto *wv = currentWebView();
    if (!wv) return;

    QUrl url = wv->currentUrl();
    if (!url.isValid() || url.scheme() == "about" || url.scheme() == "frint") return;

    if (BookmarkManager::instance().hasBookmark(url)) {
        BookmarkManager::instance().removeBookmark(url);
        m_bookmarkBtn->setStyleSheet("color: #585b70;");
        showNotification("Bookmark Removed", wv->title());
    } else {
        BookmarkManager::instance().addBookmark(wv->title(), url);
        m_bookmarkBtn->setStyleSheet("color: #f9e2af;");
        showNotification("Bookmark Added", wv->title() + "\n" + url.toString());
    }
}

void BrowserWindow::onShowBookmarks()
{
    auto &bm = BookmarkManager::instance();

    auto *dialog = new QDialog(this);
    dialog->setWindowTitle("Bookmarks");
    dialog->setMinimumSize(500, 400);
    dialog->setStyleSheet("QDialog { background: #1e1e2e; color: #cdd6f4; }");

    auto *layout = new QVBoxLayout(dialog);
    auto *tree = new QTreeWidget(dialog);
    tree->setHeaderLabels({"Title", "URL", "Folder"});
    tree->setAlternatingRowColors(true);
    tree->setRootIsDecorated(false);
    tree->header()->setStretchLastSection(true);
    tree->setStyleSheet("QTreeWidget { background: rgba(49,50,68,0.3); color: #cdd6f4; border-radius: 8px; }"
                        "QTreeWidget::item { padding: 4px; }"
                        "QTreeWidget::item:selected { background: rgba(69,71,90,0.5); }");

    for (const auto &b : bm.bookmarks()) {
        auto *item = new QTreeWidgetItem;
        item->setText(0, b.title);
        item->setText(1, b.url.toString());
        item->setText(2, b.folder);
        item->setData(1, Qt::UserRole, b.url);
        tree->addTopLevelItem(item);
    }

    layout->addWidget(tree);

    auto *btnLayout = new QHBoxLayout;
    auto *openBtn = new QPushButton("Open");
    auto *delBtn = new QPushButton("Delete");
    auto *folderBtn = new QPushButton("New Folder");
    auto *closeBtn = new QPushButton("Close");

    connect(openBtn, &QPushButton::clicked, [this, dialog, tree]() {
        auto item = tree->currentItem();
        if (item) {
            QUrl url = item->data(1, Qt::UserRole).toUrl();
            if (url.isValid()) {
                addTab(url);
                dialog->accept();
            }
        }
    });
    connect(delBtn, &QPushButton::clicked, [this, dialog, tree]() {
        auto item = tree->currentItem();
        if (item) {
            QUrl url = item->data(1, Qt::UserRole).toUrl();
            BookmarkManager::instance().removeBookmark(url);
            delete item;
        }
    });
    connect(folderBtn, &QPushButton::clicked, [this, dialog]() {
        bool ok;
        QString name = QInputDialog::getText(dialog, "New Folder", "Folder name:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty()) {
            BookmarkManager::instance().addFolder(name);
            showNotification("Folder Created", name);
        }
    });
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    btnLayout->addWidget(openBtn);
    btnLayout->addWidget(delBtn);
    btnLayout->addWidget(folderBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

// ═══════════════════════════════════════════════════════════════════════
// READING LIST
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onAddToReadingList()
{
    auto *wv = currentWebView();
    if (!wv) return;
    QUrl url = wv->currentUrl();
    if (!url.isValid() || url.scheme() == "about") return;

    auto &rl = ReadingListManager::instance();
    if (rl.hasItem(url)) {
        rl.toggleRead(url);
        showNotification("Reading List", "Marked as " + QString(rl.hasItem(url) ? "unread" : "read"));
    } else {
        rl.addItem(url, wv->title());
        showNotification("Added to Reading List", wv->title());
    }
}

void BrowserWindow::onShowReadingList()
{
    auto &rl = ReadingListManager::instance();
    auto *dialog = new QDialog(this);
    dialog->setWindowTitle("Reading List");
    dialog->setMinimumSize(480, 350);
    dialog->setStyleSheet("QDialog { background: #1e1e2e; color: #cdd6f4; }");

    auto *layout = new QVBoxLayout(dialog);
    auto *list = new QListWidget(dialog);
    list->setStyleSheet("QListWidget { background: rgba(49,50,68,0.3); border-radius: 8px; padding: 4px; }"
                        "QListWidget::item { padding: 8px; border-radius: 6px; }"
                        "QListWidget::item:selected { background: rgba(69,71,90,0.5); }");

    for (const auto &item : rl.items()) {
        auto *lwi = new QListWidgetItem;
        QString prefix = item.read ? "✅ " : "📖 ";
        lwi->setText(prefix + item.title + "\n" + item.url.toString());
        lwi->setData(Qt::UserRole, item.url);
        list->addItem(lwi);
    }

    layout->addWidget(list);

    auto *btnLayout = new QHBoxLayout;
    auto *openBtn = new QPushButton("Open");
    auto *delBtn = new QPushButton("Remove");
    auto *toggleBtn = new QPushButton("Toggle Read");
    auto *closeBtn = new QPushButton("Close");

    connect(openBtn, &QPushButton::clicked, [this, dialog, list]() {
        auto item = list->currentItem();
        if (item) {
            QUrl url = item->data(Qt::UserRole).toUrl();
            if (url.isValid()) addTab(url);
            dialog->accept();
        }
    });
    connect(delBtn, &QPushButton::clicked, [dialog, list]() {
        auto item = list->currentItem();
        if (item) {
            QUrl url = item->data(Qt::UserRole).toUrl();
            ReadingListManager::instance().removeItem(url);
            delete item;
        }
    });
    connect(toggleBtn, &QPushButton::clicked, [list]() {
        auto item = list->currentItem();
        if (item) {
            QUrl url = item->data(Qt::UserRole).toUrl();
            ReadingListManager::instance().toggleRead(url);
        }
    });
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    btnLayout->addWidget(openBtn);
    btnLayout->addWidget(delBtn);
    btnLayout->addWidget(toggleBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

// ═══════════════════════════════════════════════════════════════════════
// NOTES
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onShowNotes()
{
    auto &nm = NoteManager::instance();
    auto *dialog = new QDialog(this);
    dialog->setWindowTitle("Notes");
    dialog->setMinimumSize(600, 450);
    dialog->setStyleSheet("QDialog { background: #1e1e2e; color: #cdd6f4; }");

    auto *layout = new QVBoxLayout(dialog);

    auto *list = new QListWidget(dialog);
    list->setMinimumHeight(120);
    list->setStyleSheet("QListWidget { background: rgba(49,50,68,0.3); border-radius: 8px; }"
                        "QListWidget::item { padding: 6px; border-radius: 4px; }"
                        "QListWidget::item:selected { background: rgba(69,71,90,0.5); }");

    for (const auto &n : nm.notes()) {
        QString prefix = n.pinned ? "📌 " : "";
        auto *item = new QListWidgetItem(prefix + n.title + "\n" + n.modified.toString("yyyy-MM-dd hh:mm"));
        item->setData(Qt::UserRole, n.id);
        list->addItem(item);
    }

    auto *titleEdit = new QLineEdit;
    titleEdit->setPlaceholderText("Note title...");
    titleEdit->setStyleSheet("background: rgba(49,50,68,0.4); color: #cdd6f4; border: 1px solid #45475a; border-radius: 8px; padding: 8px;");

    auto *contentEdit = new QTextEdit;
    contentEdit->setPlaceholderText("Write your note here...");
    contentEdit->setMinimumHeight(150);
    contentEdit->setStyleSheet("background: rgba(49,50,68,0.4); color: #cdd6f4; border: 1px solid #45475a; border-radius: 8px; padding: 8px;");

    layout->addWidget(list);
    layout->addWidget(titleEdit);
    layout->addWidget(contentEdit);

    auto *btnLayout = new QHBoxLayout;
    auto *addBtn = new QPushButton("Add Note");
    auto *saveBtn = new QPushButton("Save");
    auto *delBtn = new QPushButton("Delete");
    auto *pinBtn = new QPushButton("Toggle Pin");
    auto *closeBtn = new QPushButton("Close");

    connect(list, &QListWidget::currentRowChanged, [&nm, titleEdit, contentEdit, list](int) {
        auto item = list->currentItem();
        if (item) {
            QString id = item->data(Qt::UserRole).toString();
            Note n = nm.findNote(id);
            titleEdit->setText(n.title);
            contentEdit->setPlainText(n.content);
        }
    });

    connect(addBtn, &QPushButton::clicked, [&nm, titleEdit, contentEdit, list]() {
        nm.addNote(titleEdit->text(), contentEdit->toPlainText());
        auto *item = new QListWidgetItem(titleEdit->text());
        // Get the last added note's id
        auto notes = nm.notes();
        if (!notes.isEmpty())
            item->setData(Qt::UserRole, notes.first().id);
        list->insertItem(0, item);
        titleEdit->clear();
        contentEdit->clear();
    });

    connect(saveBtn, &QPushButton::clicked, [&nm, list, titleEdit, contentEdit]() {
        auto item = list->currentItem();
        if (item) {
            nm.updateNote(item->data(Qt::UserRole).toString(), titleEdit->text(), contentEdit->toPlainText());
            item->setText(titleEdit->text());
        }
    });

    connect(delBtn, &QPushButton::clicked, [&nm, list]() {
        auto item = list->currentItem();
        if (item) {
            nm.removeNote(item->data(Qt::UserRole).toString());
            delete item;
        }
    });

    connect(pinBtn, &QPushButton::clicked, [&nm, list]() {
        auto item = list->currentItem();
        if (item) {
            nm.togglePin(item->data(Qt::UserRole).toString());
        }
    });

    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(delBtn);
    btnLayout->addWidget(pinBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

// ═══════════════════════════════════════════════════════════════════════
// SCREENSHOT
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onScreenshot()
{
    auto *wv = currentWebView();
    if (!wv) return;

    ScreenshotCapture &sc = ScreenshotCapture::instance();
    connect(&sc, &ScreenshotCapture::screenshotTaken, this,
        [this](const QString &path) {
            showNotification("Screenshot Saved", path);
            // Also copy to clipboard
            QApplication::clipboard()->setImage(QImage(path));
            m_statusLabel->setText("Screenshot saved: " + path);
        }, Qt::SingleShotConnection);

    sc.captureVisible(wv->engineView());
}

// ═══════════════════════════════════════════════════════════════════════
// PAGE TRANSLATION
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onTranslatePage()
{
    auto *wv = currentWebView();
    if (!wv || !wv->page()) return;

    // Prompt for target language
    QStringList langs = {"en", "tr", "de", "fr", "es", "it", "pt", "ru", "ja", "zh", "ar", "nl", "pl", "sv"};
    QStringList langNames = {"English", "Turkish", "German", "French", "Spanish", "Italian",
                             "Portuguese", "Russian", "Japanese", "Chinese", "Arabic", "Dutch",
                             "Polish", "Swedish"};
    bool ok;
    QString lang = QInputDialog::getItem(this, "Translate Page",
        "Select target language:", langNames, 0, false, &ok);
    if (!ok) return;

    QString langCode = langs[langNames.indexOf(lang)];

    // Use Google Translate via JavaScript injection
    QString js = R"(
(function(){
    var tl = document.createElement('script');
    tl.src = 'https://translate.google.com/translate_a/element.js?cb=googleTranslateElementInit';
    document.body.appendChild(tl);
    window.googleTranslateElementInit = function() {
        new google.translate.TranslateElement({
            pageLanguage: 'auto',
            targetLanguage: ')" + langCode + R"(',
            autoDisplay: true
        }, 'google_translate_element');
    };
    var div = document.createElement('div');
    div.id = 'google_translate_element';
    div.style.display = 'none';
    document.body.insertBefore(div, document.body.firstChild);
})();
)";
    wv->page()->runJavaScript(js);
    showNotification("Translating", "Translating page to " + lang);
}

// ═══════════════════════════════════════════════════════════════════════
// NIGHT MODE — Dark overlay on all pages
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onToggleNightMode()
{
    m_nightMode = !m_nightMode;
    applyNightMode(m_nightMode);
    m_nightModeAction->setChecked(m_nightMode);
}

void BrowserWindow::applyNightMode(bool enabled)
{
    QString js = enabled
        ? R"(document.documentElement.style.filter='invert(1) hue-rotate(180deg)';
             document.documentElement.style.backgroundColor='#000';)"
        : R"(document.documentElement.style.filter='';)";

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *wv = qobject_cast<WebView *>(m_tabWidget->widget(i));
        if (wv && wv->page()) {
            wv->page()->runJavaScript(js);
        }
    }
    showNotification(enabled ? "Night Mode On" : "Night Mode Off",
                     enabled ? "Dark filter applied to all pages" : "Normal mode restored");
}

// ═══════════════════════════════════════════════════════════════════════
// VOICE SEARCH
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onVoiceSearch()
{
    auto *wv = currentWebView();
    if (!wv || !wv->page()) return;

    // Use Web Speech API's SpeechRecognition via JavaScript
    QString js = R"(
(function() {
    if (!('webkitSpeechRecognition' in window) && !('SpeechRecognition' in window)) {
        alert('Voice search is not supported in this browser.');
        return;
    }
    var recognition = new (window.SpeechRecognition || window.webkitSpeechRecognition)();
    recognition.lang = navigator.language || 'en-US';
    recognition.interimResults = false;
    recognition.maxAlternatives = 1;
    recognition.start();
    recognition.onresult = function(event) {
        var transcript = event.results[0][0].transcript;
        var input = document.querySelector('input[type=text],input[type=search],#searchInput');
        if (input) {
            input.value = transcript;
            input.form && input.form.submit();
        } else {
            window.location.href = 'https://duckduckgo.com/?q=' + encodeURIComponent(transcript);
        }
    };
    recognition.onerror = function(event) {
        console.log('Voice recognition error: ' + event.error);
    };
})();
)";
    wv->page()->runJavaScript(js);
    showNotification("Voice Search", "Listening... Speak now");
    m_statusLabel->setText("Voice search activated - speak into your microphone");
}

// ═══════════════════════════════════════════════════════════════════════
// WINDOW TRANSPARENCY
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onSetWindowOpacity(int value)
{
    // value 0-100
    qreal opacity = qBound(0.1, value / 100.0, 1.0);
    setWindowOpacity(opacity);
    SettingsManager::instance().setValue("window_opacity", value);
}

// ═══════════════════════════════════════════════════════════════════════
// RESTART
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onRestartBrowser()
{
    auto reply = QMessageBox::question(this, "Restart Browser",
        "Are you sure you want to restart?\nAll open tabs will be lost.",
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    SettingsManager::instance().sync();
    QProcess::startDetached(QApplication::applicationFilePath());
    QApplication::quit();
}

// ═══════════════════════════════════════════════════════════════════════
// UPDATE CHECK
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onCheckForUpdates()
{
    showNotification("Check for Updates", "Frint Browser v1.0.0 — You're up to date!");
    QMessageBox::information(this, "Updates",
        "Frint Browser v1.0.0\n\nYour browser is up to date.\n\n"
        "Check https://github.com/qfdevel/Frint-Browser/releases for new versions.");
}

void BrowserWindow::onAutoUpdateCheck()
{
    if (SettingsManager::instance().isAutoUpdateCheck()) {
        // Simulate update check
        qDebug() << "[Frint] Auto-update check";
        // In production: fetch GitHub releases API
    }
}

// ═══════════════════════════════════════════════════════════════════════
// CONNECTION STATUS
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onConnectivityChanged(bool online)
{
    updateConnectionStatus();
}

void BrowserWindow::setupConnectivityMonitor()
{
    m_networkInfo = QNetworkInformation::instance();
    if (m_networkInfo) {
        connect(m_networkInfo, &QNetworkInformation::reachabilityChanged,
                this, [this](QNetworkInformation::Reachability reachability) {
            bool online = (reachability == QNetworkInformation::Reachability::Online);
            onConnectivityChanged(online);
        });
    }
}

void BrowserWindow::updateConnectionStatus()
{
    bool online = true;
    if (m_networkInfo) {
        online = m_networkInfo->reachability() == QNetworkInformation::Reachability::Online;
    }
    m_connectionStatusLabel->setText(online
        ? QString::fromUtf8("\xF0\x9F\x93\xA1 Online")
        : QString::fromUtf8("\xE2\x9D\x8C Offline"));
    m_connectionStatusLabel->setStyleSheet(online
        ? "color: #a6e3a1; font-size: 11px; padding: 0 6px;"
        : "color: #f38ba8; font-size: 11px; padding: 0 6px;");
}

// ═══════════════════════════════════════════════════════════════════════
// SHORTCUT LIST
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onShowShortcuts()
{
    auto *dialog = new QDialog(this);
    dialog->setWindowTitle("Keyboard Shortcuts");
    dialog->setMinimumSize(450, 450);
    dialog->setStyleSheet("QDialog { background: #1e1e2e; color: #cdd6f4; }");

    auto *layout = new QVBoxLayout(dialog);

    struct Shortcut { QString key; QString desc; };
    QList<Shortcut> shortcuts = {
        {"Ctrl+T", "New Tab"},
        {"Ctrl+W", "Close Tab"},
        {"Ctrl+1", "Quick Dial"},
        {"Ctrl+D", "Bookmark Page"},
        {"Ctrl+H", "History"},
        {"Ctrl+L", "Focus Address Bar"},
        {"Ctrl+R / F5", "Reload Page"},
        {"Ctrl+Q", "Quit Frint"},
        {"Ctrl+=", "Zoom In"},
        {"Ctrl+-", "Zoom Out"},
        {"Ctrl+0", "Reset Zoom"},
        {"Alt+Left", "Go Back"},
        {"Alt+Right", "Go Forward"},
        {"Alt+Home", "Home Page"},
        {"F11", "Full Screen"},
        {"F12", "Developer Tools"},
        {"F6", "Focus Address Bar"},
        {"Escape", "Stop Loading"},
        {"Ctrl+Shift+N", "Toggle Night Mode"},
        {"Ctrl+Shift+S", "Screenshot"},
        {"Ctrl+Shift+L", "Add to Reading List"},
        {"Ctrl+Shift+P", "Performance Status"},
        {"Ctrl+Shift+Delete", "Clear Data"},
    };

    auto *tree = new QTreeWidget(dialog);
    tree->setHeaderLabels({"Shortcut", "Action"});
    tree->setRootIsDecorated(false);
    tree->header()->setStretchLastSection(true);
    tree->setAlternatingRowColors(true);
    tree->setStyleSheet("QTreeWidget { background: rgba(49,50,68,0.3); border-radius: 8px; }"
                        "QTreeWidget::item { padding: 6px; }");

    for (const auto &s : shortcuts) {
        auto *item = new QTreeWidgetItem;
        item->setText(0, s.key);
        item->setText(1, s.desc);
        tree->addTopLevelItem(item);
    }

    tree->resizeColumnToContents(0);
    layout->addWidget(tree);

    auto *closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeBtn);

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

// ═══════════════════════════════════════════════════════════════════════
// CLEAR DATA SHORTCUT Ctrl+Shift+Delete
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onClearDataShortcut()
{
    auto reply = QMessageBox::question(this, "Clear Browsing Data",
        "Clear all browsing data?\n\nThis will remove:\n"
        "- Browsing history\n- Cookies\n- Cache\n- Site data\n- Downloads history",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        clearAllSessionData();
        HistoryManager::instance().clearHistory();
        CookieManager::instance().clearCookies();
        showNotification("Cleared", "All browsing data cleared");
        m_statusLabel->setText("All browsing data cleared");
    }
}

// ═══════════════════════════════════════════════════════════════════════
// AUTO COOKIE CLEAR TOGGLE
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onToggleAutoCookieClear()
{
    auto &cm = CookieManager::instance();
    if (cm.isAutoClearActive()) {
        cm.stopAutoClear();
        showNotification("Auto-Cookie Clear", "Disabled");
    } else {
        int interval = 30;
        cm.startAutoClear(interval);
        showNotification("Auto-Cookie Clear",
            "Cookies will be cleared every " + QString::number(interval) + " minutes");
    }
}

// ═══════════════════════════════════════════════════════════════════════
// PERFORMANCE
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::applyPerformanceSettings()
{
    auto &s = SettingsManager::instance();

    bool hwAccel = s.isHardwareAccelerationEnabled();
    bool sseAvx = s.isSSEAVXEnabled();
    QString perfMode = s.performanceMode();

    // Resolve mode from individual toggles and mode setting
    if (perfMode == "compatibility") {
        hwAccel = false;
        sseAvx = false;
        s.setHardwareAccelerationEnabled(false);
        s.setSSEAVXEnabled(false);
    } else if (perfMode == "balanced") {
        hwAccel = false;
        sseAvx = true;
        s.setHardwareAccelerationEnabled(false);
        s.setSSEAVXEnabled(true);
    }

    // On Qt WebEngine, hardware acceleration is handled by the runtime
    // Blink/V8 handles SSE/AVX automatically

    // Update menu actions from saved state
    if (m_hwAccelAction) {
        m_hwAccelAction->setChecked(hwAccel);
    }
    if (m_sseAvxAction) {
        m_sseAvxAction->setChecked(sseAvx);
    }

    // Update status bar
    updatePerformanceIndicator();

    qDebug() << "[Frint] Performance: HW Accel=" << hwAccel
             << "SSE/AVX=" << sseAvx << "Mode=" << perfMode;
}

void BrowserWindow::updatePerformanceIndicator()
{
    auto &s = SettingsManager::instance();
    bool hwAccel = s.isHardwareAccelerationEnabled();
    bool sseAvx = s.isSSEAVXEnabled();
    QString cpuFeatures = detectCPUFeatures();

    if (hwAccel && sseAvx) {
        m_performanceIndicator->setText(QString::fromUtf8("\xF0\x9F\x9F\xA2 GPU Active | %1").arg(cpuFeatures));
        m_performanceIndicator->setStyleSheet("color: #a6e3a1; font-size: 11px; padding: 0 6px;");
    } else if (!hwAccel && sseAvx) {
        m_performanceIndicator->setText(QString::fromUtf8("\xF0\x9F\x9F\xA1 GPU Off | %1").arg(cpuFeatures));
        m_performanceIndicator->setStyleSheet("color: #f9e2af; font-size: 11px; padding: 0 6px;");
    } else {
        m_performanceIndicator->setText(QString::fromUtf8("\xF0\x9F\x94\xB4 Compatibility | All off"));
        m_performanceIndicator->setStyleSheet("color: #f38ba8; font-size: 11px; padding: 0 6px;");
    }
}

void BrowserWindow::onToggleHardwareAcceleration()
{
    auto &s = SettingsManager::instance();
    bool enabled = !s.isHardwareAccelerationEnabled();
    s.setHardwareAccelerationEnabled(enabled);

    if (m_hwAccelAction) m_hwAccelAction->setChecked(enabled);

    applyPerformanceSettings();

    showNotification("Hardware Acceleration",
        enabled ? "GPU acceleration enabled" : "GPU acceleration disabled");
    m_statusLabel->setText(enabled
        ? "Hardware Acceleration enabled"
        : "Hardware Acceleration disabled");
}

void BrowserWindow::onToggleSSEAVX()
{
    auto &s = SettingsManager::instance();
    bool enabled = !s.isSSEAVXEnabled();
    s.setSSEAVXEnabled(enabled);

    if (m_sseAvxAction) m_sseAvxAction->setChecked(enabled);

    applyPerformanceSettings();

    showNotification("SSE/AVX Optimizations",
        enabled ? "CPU optimizations enabled" : "CPU optimizations disabled");
    m_statusLabel->setText(enabled
        ? "SSE/AVX optimizations enabled"
        : "SSE/AVX optimizations disabled");
}

void BrowserWindow::onShowPerformanceStatus()
{
    auto &s = SettingsManager::instance();
    bool hwAccel = s.isHardwareAccelerationEnabled();
    bool sseAvx = s.isSSEAVXEnabled();
    QString perfMode = s.performanceMode();
    QString cpuFeatures = detectCPUFeatures();
    QString gpuInfo = detectGPUInfo();

    QString status = QString(
        "<h3>Performance Status</h3>"
        "<table>"
        "<tr><td><b>Hardware Acceleration:</b></td><td>%1</td></tr>"
        "<tr><td><b>SSE/AVX Optimizations:</b></td><td>%2</td></tr>"
        "<tr><td><b>Performance Mode:</b></td><td>%3</td></tr>"
        "<tr><td><b>CPU Features:</b></td><td>%4</td></tr>"
        "<tr><td><b>GPU:</b></td><td>%5</td></tr>"
        "</table>"
    ).arg(hwAccel ? "\u2705 Enabled" : "\u274C Disabled",
          sseAvx ? "\u2705 Enabled" : "\u274C Disabled",
          perfMode.toUpper(),
          cpuFeatures,
          gpuInfo);

    // Determine FPS estimate
    QString fpsText;
    if (hwAccel && sseAvx) fpsText = "\xE2\x9A\xA1 60 FPS (Maximum)";
    else if (!hwAccel && sseAvx) fpsText = "\xE2\x9A\xA1 30 FPS (Reduced)";
    else fpsText = "\xE2\x9A\xA1 15 FPS (Compatibility)";

    status += "<p style='font-size:16px; font-weight:bold; text-align:center;'>"
              + fpsText + "</p>";

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Performance Status");
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(status);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setStyleSheet(
        "QMessageBox { background: #1e1e2e; color: #cdd6f4; }"
        "QPushButton { background: rgba(69,71,90,0.35); color: #cdd6f4;"
        "  border: 1px solid rgba(255,255,255,0.06); border-radius: 10px;"
        "  padding: 8px 20px; }"
        "QPushButton:hover { background: rgba(69,71,90,0.6); }");
    msgBox.exec();
}

// ═══════════════════════════════════════════════════════════════════════
// SETTINGS
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onShowSettings()
{
    auto *dialog = new SettingsDialog(this);
    connect(dialog, &SettingsDialog::settingsApplied, this, [this]() {
        auto &s = SettingsManager::instance();
        setupStyleSheet();
        applyTheme(s.theme());
        m_profileLabel->setText("👤 " + s.activeProfile());
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            auto *wv = qobject_cast<WebView *>(m_tabWidget->widget(i));
            if (wv) wv->updatePrivacySettings();
        }
        updatePrivacyIndicators();

        // Apply window transparency
        int opacity = s.getValue("window_opacity", 100).toInt();
        onSetWindowOpacity(opacity);

        // Re-apply performance settings
        applyPerformanceSettings();

        // Apply exit confirmation setting from dialog
    });
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void BrowserWindow::onShowDownloads()
{
    QString dlDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QDesktopServices::openUrl(QUrl::fromLocalFile(dlDir));
}

void BrowserWindow::onShowHistory()
{
    auto &hm = HistoryManager::instance();
    auto *dialog = new QDialog(this);
    dialog->setWindowTitle("History");
    dialog->setMinimumSize(550, 400);
    dialog->setStyleSheet("QDialog { background: #1e1e2e; color: #cdd6f4; }");

    auto *layout = new QVBoxLayout(dialog);

    auto *searchEdit = new QLineEdit;
    searchEdit->setPlaceholderText("Search history...");
    searchEdit->setStyleSheet("background: rgba(49,50,68,0.4); color: #cdd6f4; border: 1px solid #45475a; border-radius: 8px; padding: 8px;");
    layout->addWidget(searchEdit);

    auto *tree = new QTreeWidget(dialog);
    tree->setHeaderLabels({"Title", "URL", "Visits", "Last Visit"});
    tree->setRootIsDecorated(false);
    tree->header()->setStretchLastSection(true);
    tree->setAlternatingRowColors(true);
    tree->setStyleSheet("QTreeWidget { background: rgba(49,50,68,0.3); border-radius: 8px; }"
                        "QTreeWidget::item { padding: 4px; }"
                        "QTreeWidget::item:selected { background: rgba(69,71,90,0.5); }");

    auto populateList = [&hm, tree](const QString &search) {
        tree->clear();
        auto entries = search.isEmpty() ? hm.recentHistory() : hm.searchHistory(search);
        for (const auto &e : entries) {
            auto *item = new QTreeWidgetItem;
            item->setText(0, e.title);
            item->setText(1, e.url.toString());
            item->setText(2, QString::number(e.visitCount));
            item->setText(3, e.visitTime.toString("yyyy-MM-dd hh:mm"));
            item->setData(1, Qt::UserRole, e.url);
            tree->addTopLevelItem(item);
        }
    };

    populateList("");

    connect(searchEdit, &QLineEdit::textChanged, [&hm, populateList](const QString &text) {
        populateList(text);
    });

    layout->addWidget(tree);

    auto *btnLayout = new QHBoxLayout;
    auto *openBtn = new QPushButton("Open");
    auto *clearBtn = new QPushButton("Clear All");
    auto *closeBtn = new QPushButton("Close");

    connect(openBtn, &QPushButton::clicked, [this, dialog, tree]() {
        auto item = tree->currentItem();
        if (item) {
            QUrl url = item->data(1, Qt::UserRole).toUrl();
            if (url.isValid()) addTab(url);
            dialog->accept();
        }
    });
    connect(clearBtn, &QPushButton::clicked, [&hm, populateList]() {
        hm.clearHistory();
        populateList("");
    });
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    btnLayout->addWidget(openBtn);
    btnLayout->addWidget(clearBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

// ═══════════════════════════════════════════════════════════════════════
// PROFILE SWITCH
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onSwitchProfile()
{
    auto &s = SettingsManager::instance();
    m_profileLabel->setText("👤 " + s.activeProfile());
    QString dir = s.profileDir();

    QList<QUrl> urls;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *wv = qobject_cast<WebView *>(m_tabWidget->widget(i));
        if (wv) urls.append(wv->currentUrl());
    }

    while (m_tabWidget->count() > 0) {
        QWidget *w = m_tabWidget->widget(0);
        m_tabWidget->removeTab(0);
        w->deleteLater();
    }

    for (const auto &url : urls)
        addTab(url);

    m_statusLabel->setText("Switched to profile: " + s.activeProfile());
}

void BrowserWindow::onToggleExtensions()
{
    m_extensionsVisible = !m_extensionsVisible;
    if (m_extensionsVisible) {
        m_extensionsBtn->setStyleSheet("background: #45475a; border-radius: 8px;");
        m_extensionsMenu->exec(m_extensionsBtn->mapToGlobal(QPoint(0, m_extensionsBtn->height())));
    } else {
        m_extensionsBtn->setStyleSheet("");
    }
}

void BrowserWindow::onAboutFrint()
{
    QMessageBox::about(this, "About Frint Browser",
        "<h2>Frint Browser</h2>"
        "<p>Version 1.0.0</p>"
        "<p>Privacy-first. Telemetry-free. Yours.</p>"
        "<p>Built with Qt WebEngine + Frint Privacy Engine</p>"
        "<hr>"
        "<p><b>Features:</b> Bookmarks, History, Reading List, Notes,<br>"
        "Quick Dial, Voice Search, Page Translation,<br>"
        "Night Mode, Screenshot, AutoFill, Spell Check,<br>"
        "Auto Cookie Clearing, Desktop Notifications,<br>"
        "Download Progress, Exit Confirmation, Restart</p>"
        "<p style='color:#585b70; font-size:11px;'>"
        "HTTPS-Only | Tracker Blocker | GPC | Fingerprint Defense | DoH |"
        " Anti-Fingerprint JS | WebRTC Block | ETag Strip</p>");
}

// ═══════════════════════════════════════════════════════════════════════
// PRIVACY ACTIONS
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::onToggleTrackingBlocker()
{
    auto &tb = TrackingBlocker::instance();
    tb.setEnabled(!tb.isEnabled());
    SettingsManager::instance().setTrackingBlockerEnabled(tb.isEnabled());
    updatePrivacyIndicators();
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *wv = qobject_cast<WebView *>(m_tabWidget->widget(i));
        if (wv) wv->updatePrivacySettings();
    }
}

void BrowserWindow::onToggleHttpsOnly()
{
    auto &s = SettingsManager::instance();
    s.setHttpsOnly(!s.isHttpsOnly());
    updatePrivacyIndicators();
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *wv = qobject_cast<WebView *>(m_tabWidget->widget(i));
        if (wv) wv->updatePrivacySettings();
    }
}

void BrowserWindow::onToggleGpc()
{
    auto &gpc = GpcHeader::instance();
    gpc.setEnabled(!gpc.isEnabled());
    SettingsManager::instance().setGpcEnabled(gpc.isEnabled());
    updatePrivacyIndicators();
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *wv = qobject_cast<WebView *>(m_tabWidget->widget(i));
        if (wv) wv->updatePrivacySettings();
    }
}

void BrowserWindow::onToggleFingerprinting()
{
    auto &fp = FingerprintingDefender::instance();
    fp.setEnabled(!fp.isEnabled());
    SettingsManager::instance().setFingerprintingProtection(fp.isEnabled());
    updatePrivacyIndicators();
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *wv = qobject_cast<WebView *>(m_tabWidget->widget(i));
        if (wv) wv->updatePrivacySettings();
    }
}

void BrowserWindow::onClearStorage()
{
    clearAllSessionData();
    statusBar()->showMessage("All storage cleared", 5000);
}

void BrowserWindow::onClearSiteData()
{
    auto *wv = currentWebView();
    if (wv && wv->page()) {
        wv->page()->profile()->clearHttpCache();
        wv->page()->profile()->cookieStore()->deleteAllCookies();
        statusBar()->showMessage("Cache & cookies cleared for current session", 5000);
    }
}

void BrowserWindow::onToggleBookmarksBar()
{
    m_bookmarksBarVisible = !m_bookmarksBarVisible;
    m_statusLabel->setText(m_bookmarksBarVisible ? "Bookmarks bar shown" : "Bookmarks bar hidden");
}

// ═══════════════════════════════════════════════════════════════════════
// KEYBOARD EVENTS
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (auto *w = currentWebView()) w->stop();
    } else if (event->key() == Qt::Key_F5) {
        if (auto *w = currentWebView()) w->reload();
    } else if (event->key() == Qt::Key_F6) {
        m_addressBar->setFocus();
        m_addressBar->selectAll();
    } else if (event->key() == Qt::Key_F11) {
        if (isFullScreen()) showNormal(); else showFullScreen();
    } else if (event->key() == Qt::Key_F12) {
        auto *wv = currentWebView();
        if (wv && wv->page()) {
            if (!m_devToolsWindow) {
                m_devToolsWindow = new QWidget(this, Qt::Window);
                m_devToolsWindow->setWindowTitle("Frint DevTools");
                m_devToolsWindow->resize(900, 600);
                auto *dl = new QVBoxLayout(m_devToolsWindow);
                dl->setContentsMargins(0, 0, 0, 0);
                auto *devProfile = new QWebEngineProfile(m_devToolsWindow);
                m_devToolsView = new QWebEngineView(m_devToolsWindow);
                auto *devPage = new QWebEnginePage(devProfile, m_devToolsView);
                m_devToolsView->setPage(devPage);
                dl->addWidget(m_devToolsView);
                m_devToolsWindow->setLayout(dl);
                m_devToolsWindow->setStyleSheet("QWidget { background: #1e1e2e; }");
            }
            m_devToolsWindow->setWindowTitle("Frint DevTools - " + wv->title());
            wv->page()->setDevToolsPage(m_devToolsView->page());
            m_devToolsWindow->show();
            m_devToolsWindow->raise();
            m_devToolsWindow->activateWindow();
        }
    }
    QMainWindow::keyPressEvent(event);
}

// ═══════════════════════════════════════════════════════════════════════
// UI HELPERS
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::updateAddressBar(const QUrl &url)
{
    m_addressBar->blockSignals(true);
    m_addressBar->setText(url.toString());
    m_addressBar->setCursorPosition(0);
    m_addressBar->blockSignals(false);

    // Update bookmark star
    m_bookmarkBtn->setStyleSheet(BookmarkManager::instance().hasBookmark(url)
        ? "color: #f9e2af;" : "");

    // Security lock
    if (url.scheme() == "https") {
        m_securityLock->setText(QString::fromUtf8("\xF0\x9F\x94\x92"));
        m_securityLock->setToolTip("Connection secure (HTTPS)");
        m_securityLock->setStyleSheet("font-size: 15px; padding: 0 4px;");
    } else if (url.scheme() == "http") {
        m_securityLock->setText(QString::fromUtf8("\xE2\x9A\xA0\xEF\xB8\x8F"));
        m_securityLock->setToolTip("Connection not secure (HTTP)");
        m_securityLock->setStyleSheet("font-size: 15px; padding: 0 4px; color: #f9e2af;");
    } else {
        m_securityLock->setText(QString::fromUtf8("\xF0\x9F\x94\x90"));
        m_securityLock->setToolTip("Local page");
        m_securityLock->setStyleSheet("font-size: 15px; padding: 0 4px; color: #585b70;");
    }
}

void BrowserWindow::updateNavButtons()
{
    auto *wv = currentWebView();
    if (wv) {
        m_backBtn->setEnabled(wv->canGoBack());
        m_forwardBtn->setEnabled(wv->canGoForward());
    }
}

void BrowserWindow::updatePrivacyIndicators()
{
    auto &s = SettingsManager::instance();
    m_trackingAction->setChecked(s.isTrackingBlockerEnabled());
    m_httpsAction->setChecked(s.isHttpsOnly());
    m_gpcAction->setChecked(s.isGpcEnabled());
    m_fingerprintingAction->setChecked(s.isFingerprintingProtectionEnabled());

    if (m_antiFingerprintAction) m_antiFingerprintAction->setChecked(s.isAntiFingerprintEnabled());
    if (m_webrtcAction) m_webrtcAction->setChecked(s.isWebrtcBlocked());
    if (m_etagAction) m_etagAction->setChecked(s.isEtagTrackingBlocked());

    bool allSecure = s.isTrackingBlockerEnabled() && s.isHttpsOnly()
                  && s.isGpcEnabled() && s.isFingerprintingProtectionEnabled()
                  && s.isDohEnabled() && s.isAntiFingerprintEnabled()
                  && s.isWebrtcBlocked() && s.isEtagTrackingBlocked();

    if (allSecure) {
        m_privacyIndicator->setText("\u2705 Privacy Active");
        m_privacyIndicator->setStyleSheet("color: #a6e3a1; padding: 0 8px;");
    } else {
        m_privacyIndicator->setText("\u26A0\uFE0F Privacy Reduced");
        m_privacyIndicator->setStyleSheet("color: #f9e2af; padding: 0 8px;");
    }
}

void BrowserWindow::updateTabCountTitle()
{
    int tabCount = m_tabWidget->count();
    auto *wv = currentWebView();
    QString title = (wv && !wv->title().isEmpty()) ? wv->title() : "Frint Browser";
    if (tabCount > 1)
        title += " (" + QString::number(tabCount) + " tabs)";
    if (!title.startsWith("Frint Browser"))
        title += " - Frint Browser";
    setWindowTitle(title);
}

// ═══════════════════════════════════════════════════════════════════════
// NOTIFICATIONS
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::setupNotifications()
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon));
    m_trayIcon->setToolTip("Frint Browser");
    m_trayIcon->show();
}

void BrowserWindow::showNotification(const QString &title, const QString &message,
                                       QSystemTrayIcon::MessageIcon icon)
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->showMessage(title, message, icon, 3000);
    }
}

// ═══════════════════════════════════════════════════════════════════════
// AUTO UPDATE CHECK
// ═══════════════════════════════════════════════════════════════════════
void BrowserWindow::setupAutoUpdateCheck()
{
    m_autoUpdateTimer = new QTimer(this);
    connect(m_autoUpdateTimer, &QTimer::timeout, this, &BrowserWindow::onAutoUpdateCheck);

    if (SettingsManager::instance().isAutoUpdateCheck()) {
        // Check every 24 hours
        m_autoUpdateTimer->start(24 * 60 * 60 * 1000);
        // Also check on startup after a short delay
        QTimer::singleShot(5000, this, &BrowserWindow::onAutoUpdateCheck);
    }
}

} // namespace Frint
