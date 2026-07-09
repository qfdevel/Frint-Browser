#include "browser_window.h"
#include "web_view.h"
#include "settings_manager.h"
#include "privacy/tracking_blocker.h"
#include "privacy/gpc_header.h"
#include "privacy/fingerprinting_defender.h"
#include "storage/storage_partition.h"

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QClipboard>

namespace Frint {

BrowserWindow::BrowserWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Frint Browser");
    resize(1280, 800);
    setMinimumSize(640, 480);

    // ── Tab widget ─────────────────────────────────────────────────────
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setElideMode(Qt::ElideRight);
    setCentralWidget(m_tabWidget);

    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &BrowserWindow::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested,
            this, &BrowserWindow::onCloseTab);

    // ── UI setup ──────────────────────────────────────────────────────
    setupMenuBar();
    setupToolBar();
    setupStatusBar();

    // ── Load settings ──────────────────────────────────────────────────
    auto &settings = SettingsManager::instance();
    settings.loadDefaults();

    // ── Initial tab ────────────────────────────────────────────────────
    addTab(QUrl(settings.homePage()));

    updatePrivacyIndicators();
}

BrowserWindow::~BrowserWindow()
{
    auto &settings = SettingsManager::instance();
    if (settings.isClearOnExit()) {
        StoragePartition::instance().clearAll();
    }
    settings.sync();
}

// ── Menu bar ────────────────────────────────────────────────────────────

void BrowserWindow::setupMenuBar()
{
    // File menu
    m_fileMenu = menuBar()->addMenu("&File");

    QAction *newTab = m_fileMenu->addAction("&New Tab");
    newTab->setShortcut(QKeySequence::AddTab);
    connect(newTab, &QAction::triggered, this, &BrowserWindow::onNewTab);

    QAction *closeTab = m_fileMenu->addAction("&Close Tab");
    closeTab->setShortcut(QKeySequence::Close);
    connect(closeTab, &QAction::triggered, this, [this]() {
        int idx = m_tabWidget->currentIndex();
        if (idx >= 0) onCloseTab(idx);
    });

    m_fileMenu->addSeparator();

    QAction *quit = m_fileMenu->addAction("&Quit");
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, qApp, &QApplication::quit);

    // Privacy menu
    m_privacyMenu = menuBar()->addMenu("&Privacy");

    m_trackingAction = m_privacyMenu->addAction("&Tracker Blocker");
    m_trackingAction->setCheckable(true);
    connect(m_trackingAction, &QAction::triggered,
            this, &BrowserWindow::onToggleTrackingBlocker);

    m_httpsAction = m_privacyMenu->addAction("HTTPS-&Only Mode");
    m_httpsAction->setCheckable(true);
    connect(m_httpsAction, &QAction::triggered,
            this, &BrowserWindow::onToggleHttpsOnly);

    m_gpcAction = m_privacyMenu->addAction("&Global Privacy Control");
    m_gpcAction->setCheckable(true);
    connect(m_gpcAction, &QAction::triggered,
            this, &BrowserWindow::onToggleGpc);

    m_fingerprintingAction = m_privacyMenu->addAction("&Fingerprinting Defense");
    m_fingerprintingAction->setCheckable(true);
    connect(m_fingerprintingAction, &QAction::triggered,
            this, &BrowserWindow::onToggleFingerprinting);

    m_privacyMenu->addSeparator();

    QAction *clearStorage = m_privacyMenu->addAction("Clear All &Storage");
    connect(clearStorage, &QAction::triggered,
            this, &BrowserWindow::onClearStorage);

    QAction *clearSite = m_privacyMenu->addAction("Clear Site &Data");
    connect(clearSite, &QAction::triggered,
            this, &BrowserWindow::onClearSiteData);
}

// ── Toolbar ─────────────────────────────────────────────────────────────

void BrowserWindow::setupToolBar()
{
    m_toolBar = new QToolBar("Navigation", this);
    m_toolBar->setMovable(false);
    m_toolBar->setIconSize(QSize(16, 16));
    addToolBar(m_toolBar);

    // Navigation buttons
    m_backBtn = new QPushButton(QChar(0x2190), this);
    m_backBtn->setToolTip("Back");
    m_backBtn->setFixedWidth(32);
    m_backBtn->setEnabled(false);
    connect(m_backBtn, &QPushButton::clicked, this, &BrowserWindow::onBack);
    m_toolBar->addWidget(m_backBtn);

    m_forwardBtn = new QPushButton(QChar(0x2192), this);
    m_forwardBtn->setToolTip("Forward");
    m_forwardBtn->setFixedWidth(32);
    m_forwardBtn->setEnabled(false);
    connect(m_forwardBtn, &QPushButton::clicked, this, &BrowserWindow::onForward);
    m_toolBar->addWidget(m_forwardBtn);

    m_reloadBtn = new QPushButton(QChar(0x21BB), this);
    m_reloadBtn->setToolTip("Reload");
    m_reloadBtn->setFixedWidth(32);
    connect(m_reloadBtn, &QPushButton::clicked, this, &BrowserWindow::onReload);
    m_toolBar->addWidget(m_reloadBtn);

    m_toolBar->addSeparator();

    // Address bar
    m_addressBar = new QLineEdit(this);
    m_addressBar->setPlaceholderText("Enter URL or search...");
    m_addressBar->setClearButtonEnabled(true);
    m_addressBar->setMinimumWidth(400);
    m_addressBar->setMaximumWidth(800);
    m_addressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_addressBar, &QLineEdit::returnPressed,
            this, &BrowserWindow::onAddressBarReturnPressed);
    m_toolBar->addWidget(m_addressBar);

    // New tab button
    m_newTabBtn = new QPushButton("+", this);
    m_newTabBtn->setToolTip("New Tab");
    m_newTabBtn->setFixedWidth(30);
    connect(m_newTabBtn, &QPushButton::clicked, this, &BrowserWindow::onNewTab);
    m_toolBar->addWidget(m_newTabBtn);
}

// ── Status bar ──────────────────────────────────────────────────────────

void BrowserWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("Ready", this);
    statusBar()->addWidget(m_statusLabel, 1);

    m_privacyIndicator = new QLabel("\u2705 Privacy Active", this);
    m_privacyIndicator->setStyleSheet("color: #a6e3a1; padding: 0 8px;");
    statusBar()->addPermanentWidget(m_privacyIndicator);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumWidth(120);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    m_progressBar->setTextVisible(false);
    statusBar()->addPermanentWidget(m_progressBar);
}

// ── Tab management ──────────────────────────────────────────────────────

void BrowserWindow::addTab(const QUrl &url)
{
    auto *wv = new WebView(this);
    int idx = m_tabWidget->addTab(wv, "New Tab");

    connect(wv, &WebView::urlChanged, this, &BrowserWindow::onUrlChanged);
    connect(wv, &WebView::titleChanged, this, &BrowserWindow::onTitleChanged);
    connect(wv, &WebView::loadStarted, this, &BrowserWindow::onLoadStarted);
    connect(wv, &WebView::loadFinished, this, &BrowserWindow::onLoadFinished);
    connect(wv, qOverload<int>(&WebView::loadProgress), this, [this](int progress) {
        onLoadProgress(progress);
    });
    connect(wv, &WebView::statusBarMessage, m_statusLabel, &QLabel::setText);

    m_tabWidget->setCurrentIndex(idx);

    if (url.isValid()) {
        wv->loadUrl(url);
    }
}

void BrowserWindow::removeTab(int index)
{
    if (m_tabWidget->count() <= 1) {
        // Keep at least one tab (load blank)
        auto *wv = qobject_cast<WebView *>(m_tabWidget->widget(index));
        if (wv) wv->loadUrl(QUrl("about:blank"));
        return;
    }

    QWidget *w = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    w->deleteLater();
}

WebView *BrowserWindow::currentWebView() const
{
    return qobject_cast<WebView *>(m_tabWidget->currentWidget());
}

// ── Navigation slots ────────────────────────────────────────────────────

void BrowserWindow::onNewTab()
{
    addTab(QUrl("about:blank"));
}

void BrowserWindow::onCloseTab(int index)
{
    removeTab(index);
}

void BrowserWindow::onTabChanged(int /*index*/)
{
    auto *wv = currentWebView();
    if (wv) {
        updateAddressBar(wv->currentUrl());
        QString title = wv->title();
        setWindowTitle((title.isEmpty() ? "Frint Browser"
                                        : title + " - Frint Browser"));
        updateNavButtons();
    }
}

void BrowserWindow::onUrlChanged(const QUrl &url)
{
    auto *wv = qobject_cast<WebView *>(sender());
    if (wv && wv == currentWebView()) {
        updateAddressBar(url);
    }
}

void BrowserWindow::onTitleChanged(const QString &title)
{
    auto *wv = qobject_cast<WebView *>(sender());
    if (!wv) return;

    int idx = m_tabWidget->indexOf(wv);
    if (idx >= 0) {
        QString tabTitle = title.left(24);
        if (tabTitle.isEmpty()) tabTitle = "New Tab";
        m_tabWidget->setTabText(idx, tabTitle);
        m_tabWidget->setTabToolTip(idx, title);
    }

    if (wv == currentWebView()) {
        setWindowTitle((title.isEmpty() ? "Frint Browser"
                                        : title + " - Frint Browser"));
    }
}

void BrowserWindow::onLoadStarted()
{
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
}

void BrowserWindow::onLoadFinished(bool ok)
{
    m_progressBar->setVisible(false);
    m_statusLabel->setText(ok ? "Done" : "Error loading page");
    updateNavButtons();
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
    if (!url.isValid()) {
        // Search
        QString searchUrl = SettingsManager::instance().searchEngine();
        url = QUrl(searchUrl + QUrl::toPercentEncoding(text));
    } else if (url.scheme().isEmpty()) {
        // Default to HTTPS
        url = QUrl("https://" + text);
    }

    auto *wv = currentWebView();
    if (wv) wv->loadUrl(url);
}

void BrowserWindow::onBack()
{
    auto *wv = currentWebView();
    if (wv) wv->goBack();
}

void BrowserWindow::onForward()
{
    auto *wv = currentWebView();
    if (wv) wv->goForward();
}

void BrowserWindow::onReload()
{
    auto *wv = currentWebView();
    if (wv) wv->reload();
}

// ── Privacy actions ─────────────────────────────────────────────────────

void BrowserWindow::onToggleTrackingBlocker()
{
    auto &tb = TrackingBlocker::instance();
    tb.setEnabled(!tb.isEnabled());
    SettingsManager::instance().setTrackingBlockerEnabled(tb.isEnabled());
    updatePrivacyIndicators();
}

void BrowserWindow::onToggleHttpsOnly()
{
    auto &s = SettingsManager::instance();
    s.setHttpsOnly(!s.isHttpsOnly());
    updatePrivacyIndicators();
}

void BrowserWindow::onToggleGpc()
{
    auto &gpc = GpcHeader::instance();
    gpc.setEnabled(!gpc.isEnabled());
    SettingsManager::instance().setGpcEnabled(gpc.isEnabled());
    updatePrivacyIndicators();
}

void BrowserWindow::onToggleFingerprinting()
{
    auto &fp = FingerprintingDefender::instance();
    fp.setEnabled(!fp.isEnabled());
    SettingsManager::instance().setFingerprintingProtection(fp.isEnabled());
    updatePrivacyIndicators();
}

void BrowserWindow::onClearStorage()
{
    StoragePartition::instance().clearAll();
    statusBar()->showMessage("All storage partitions cleared", 5000);
}

void BrowserWindow::onClearSiteData()
{
    auto *wv = currentWebView();
    if (wv) {
        StoragePartition::instance().clearForOrigin(wv->currentUrl());
        statusBar()->showMessage("Storage cleared for "
                                 + wv->currentUrl().host(), 5000);
    }
}

// ── UI helpers ─────────────────────────────────────────────────────────

void BrowserWindow::updateAddressBar(const QUrl &url)
{
    m_addressBar->blockSignals(true);
    m_addressBar->setText(url.toString());
    m_addressBar->setCursorPosition(0);
    m_addressBar->blockSignals(false);
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

    bool allSecure = s.isTrackingBlockerEnabled()
                   && s.isHttpsOnly()
                   && s.isGpcEnabled()
                   && s.isFingerprintingProtectionEnabled()
                   && s.isDohEnabled();

    if (allSecure) {
        m_privacyIndicator->setText("\u2705 Privacy Active");
        m_privacyIndicator->setStyleSheet("color: #a6e3a1; padding: 0 8px;");
    } else {
        m_privacyIndicator->setText("\u26A0\uFE0F Privacy Reduced");
        m_privacyIndicator->setStyleSheet("color: #f9e2af; padding: 0 8px;");
    }
}

} // namespace Frint
