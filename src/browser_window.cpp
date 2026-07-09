#include "browser_window.h"
#include "web_view.h"
#include "settings_manager.h"
#include "storage/storage_partition.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>

namespace Frint {

BrowserWindow::BrowserWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Frint Browser");
    resize(1280, 800);
    setMinimumSize(640, 480);

    // Central tab widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    setCentralWidget(m_tabWidget);

    setupMenuBar();
    setupToolBar();
    setupStatusBar();

    // Signals
    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &BrowserWindow::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested,
            this, &BrowserWindow::onCloseTabClicked);

    // Load settings
    auto &settings = SettingsManager::instance();
#ifdef FRINT_SECURE_DEFAULTS
    settings.loadDefaults();
#endif

    // Open default tab
    addTab(QUrl(settings.homePage()));
}

BrowserWindow::~BrowserWindow()
{
    auto &settings = SettingsManager::instance();
    if (settings.isClearOnExit()) {
        StoragePartition::instance().clearAll();
    }
    settings.sync();
}

void BrowserWindow::setupMenuBar()
{
    // File menu
    m_fileMenu = menuBar()->addMenu("&File");

    QAction *newTabAction = m_fileMenu->addAction("&New Tab");
    newTabAction->setShortcut(QKeySequence::AddTab);
    connect(newTabAction, &QAction::triggered, this, &BrowserWindow::onNewTabClicked);

    QAction *closeTabAction = m_fileMenu->addAction("&Close Tab");
    closeTabAction->setShortcut(QKeySequence::Close);
    connect(closeTabAction, &QAction::triggered, this, [this]() {
        int idx = m_tabWidget->currentIndex();
        if (idx >= 0) {
            onCloseTabClicked(idx);
        }
    });

    QAction *quitAction = m_fileMenu->addAction("&Quit");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    // Privacy menu
    m_privacyMenu = menuBar()->addMenu("&Privacy");

    QAction *clearStorageAction = m_privacyMenu->addAction("Clear All &Storage");
    connect(clearStorageAction, &QAction::triggered, this, [this]() {
        StoragePartition::instance().clearAll();
        statusBar()->showMessage("All storage cleared", 3000);
    });

    QAction *clearSiteAction = m_privacyMenu->addAction("Clear Site &Data");
    connect(clearSiteAction, &QAction::triggered, this, [this]() {
        WebView *wv = currentWebView();
        if (wv) {
            StoragePartition::instance().clearForOrigin(wv->currentUrl());
            statusBar()->showMessage("Site data cleared for "
                                     + wv->currentUrl().host(), 3000);
        }
    });

    m_privacyMenu->addSeparator();

    QAction *settingsAction = m_privacyMenu->addAction("&Privacy Settings...");
    connect(settingsAction, &QAction::triggered, this, [this]() {
        QMessageBox::information(this, "Privacy Settings",
            "Frint Browser Privacy Features:\n\n"
            "\u2713 HTTPS-Only Mode\n"
            "\u2713 Third-Party Cookie Blocking\n"
            "\u2713 Tracking Parameter Removal\n"
            "\u2713 DNS-over-HTTPS (Cloudflare)\n"
            "\u2713 Global Privacy Control (Sec-GPC: 1)\n"
            "\u2713 Referrer Policy (Strict Origin)\n"
            "\u2713 Fingerprinting Protection\n"
            "\u2713 Tracker Blocker\n"
            "\u2713 Storage Partitioning\n\n"
            "Configure via configs/default_prefs.json");
    });
}

void BrowserWindow::setupToolBar()
{
    m_toolBar = new QToolBar("Navigation", this);
    m_toolBar->setMovable(false);
    m_toolBar->setIconSize(QSize(16, 16));
    addToolBar(m_toolBar);

    // Navigation buttons
    m_backButton = new QPushButton("\u2190", this);
    m_backButton->setToolTip("Back");
    m_backButton->setEnabled(false);
    m_toolBar->addWidget(m_backButton);

    m_forwardButton = new QPushButton("\u2192", this);
    m_forwardButton->setToolTip("Forward");
    m_forwardButton->setEnabled(false);
    m_toolBar->addWidget(m_forwardButton);

    m_reloadButton = new QPushButton("\u21BB", this);
    m_reloadButton->setToolTip("Reload");
    m_toolBar->addWidget(m_reloadButton);

    m_toolBar->addSeparator();

    // Address bar
    m_addressBar = new QLineEdit(this);
    m_addressBar->setPlaceholderText("Enter a URL or search...");
    m_addressBar->setClearButtonEnabled(true);
    m_addressBar->setMinimumWidth(400);
    m_toolBar->addWidget(m_addressBar);

    // New tab button
    m_newTabButton = new QPushButton("+", this);
    m_newTabButton->setToolTip("New Tab");
    m_newTabButton->setFixedWidth(30);
    m_toolBar->addWidget(m_newTabButton);

    // Signals
    connect(m_addressBar, &QLineEdit::returnPressed,
            this, &BrowserWindow::onAddressBarReturnPressed);
    connect(m_newTabButton, &QPushButton::clicked,
            this, &BrowserWindow::onNewTabClicked);
    connect(m_reloadButton, &QPushButton::clicked, this, [this]() {
        WebView *wv = currentWebView();
        if (wv) wv->reload();
    });
}

void BrowserWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("Ready", this);
    statusBar()->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumWidth(150);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    statusBar()->addPermanentWidget(m_progressBar);
}

void BrowserWindow::addTab(const QUrl &url)
{
    WebView *wv = new WebView(this);
    int index = m_tabWidget->addTab(wv, "New Tab");

    connect(wv, &WebView::urlChanged,
            this, &BrowserWindow::onUrlChanged);
    connect(wv, &WebView::titleChanged,
            this, &BrowserWindow::onTitleChanged);
    connect(wv, &WebView::loadStarted,
            this, &BrowserWindow::onLoadStarted);
    connect(wv, &WebView::loadFinished,
            this, &BrowserWindow::onLoadFinished);
    connect(wv, &WebView::loadProgress,
            this, &BrowserWindow::onLoadProgress);
    connect(wv, &WebView::statusBarMessage,
            m_statusLabel, &QLabel::setText);

    m_tabWidget->setCurrentIndex(index);

    if (url.isValid()) {
        wv->loadUrl(url);
    }
}

void BrowserWindow::removeTab(int index)
{
    if (m_tabWidget->count() <= 1) {
        // Don't close the last tab, just load blank
        WebView *wv = qobject_cast<WebView *>(m_tabWidget->widget(index));
        if (wv) {
            wv->loadUrl(QUrl("about:blank"));
        }
        return;
    }

    QWidget *widget = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    widget->deleteLater();
}

void BrowserWindow::onNewTabClicked()
{
    addTab(QUrl("about:blank"));
}

void BrowserWindow::onCloseTabClicked(int index)
{
    removeTab(index);
}

void BrowserWindow::onTabChanged(int index)
{
    WebView *wv = currentWebView();
    if (wv) {
        updateAddressBar(wv->currentUrl());
        setWindowTitle(wv->title() + " - Frint Browser");
    }
}

void BrowserWindow::onUrlChanged(const QUrl &url)
{
    WebView *wv = qobject_cast<WebView *>(sender());
    if (wv && wv == currentWebView()) {
        updateAddressBar(url);
    }
}

void BrowserWindow::onTitleChanged(const QString &title)
{
    WebView *wv = qobject_cast<WebView *>(sender());
    if (wv) {
        int index = m_tabWidget->indexOf(wv);
        if (index >= 0) {
            QString tabTitle = title.left(30);
            if (tabTitle.isEmpty()) tabTitle = "New Tab";
            m_tabWidget->setTabText(index, tabTitle);
            m_tabWidget->setTabToolTip(index, title);
        }
        if (wv == currentWebView()) {
            setWindowTitle(title + " - Frint Browser");
        }
    }
}

void BrowserWindow::onLoadStarted()
{
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText("Loading...");
}

void BrowserWindow::onLoadFinished(bool ok)
{
    m_progressBar->setVisible(false);
    m_statusLabel->setText(ok ? "Done" : "Error loading page");
}

void BrowserWindow::onLoadProgress(int progress)
{
    m_progressBar->setValue(progress);
}

void BrowserWindow::onNavigation(const QUrl &url)
{
    WebView *wv = currentWebView();
    if (wv) {
        wv->loadUrl(url);
    }
}

void BrowserWindow::onAddressBarReturnPressed()
{
    QString text = m_addressBar->text().trimmed();
    if (text.isEmpty()) return;

    QUrl url(text);
    if (!url.isValid()) {
        // Try as search query via DuckDuckGo
        url = QUrl("https://duckduckgo.com/?q=" + QUrl::toPercentEncoding(text));
    } else if (url.scheme().isEmpty()) {
        // Default to https
        url = QUrl("https://" + text);
    }

    onNavigation(url);
}

WebView *BrowserWindow::currentWebView() const
{
    return qobject_cast<WebView *>(m_tabWidget->currentWidget());
}

void BrowserWindow::updateAddressBar(const QUrl &url)
{
    m_addressBar->setText(url.toString());
    m_addressBar->setCursorPosition(0);
}

} // namespace Frint
