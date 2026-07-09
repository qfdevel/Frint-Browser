#ifndef FRINT_BROWSER_WINDOW_H
#define FRINT_BROWSER_WINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QToolBar>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStatusBar>
#include <QProgressBar>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QPropertyAnimation>
#include <QToolButton>
#include <QGraphicsOpacityEffect>
#include <QEvent>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QNetworkInformation>
#include <QElapsedTimer>
#include <QSlider>
#include <QDateTime>

namespace Frint {

class WebView;

class BrowserWindow : public QMainWindow {
    Q_OBJECT
    Q_PROPERTY(qreal animationProgress READ animationProgress WRITE setAnimationProgress)

public:
    explicit BrowserWindow(QWidget *parent = nullptr);
    ~BrowserWindow() override;

    bool eventFilter(QObject *obj, QEvent *event) override;

    void addTab(const QUrl &url = QUrl());
    void removeTab(int index);
    qreal animationProgress() const { return m_animProgress; }
    void setAnimationProgress(qreal p) { m_animProgress = p; update(); }

private slots:
    // Tab management
    void onNewTab();
    void onCloseTab(int index);
    void onTabChanged(int index);
    void onUrlChanged(const QUrl &url);
    void onTitleChanged(const QString &title);
    void onLoadStarted();
    void onLoadFinished(bool ok);
    void onLoadProgress(int progress);

    // Navigation
    void onAddressBarReturnPressed();
    void onBack();
    void onForward();
    void onReload();
    void onHome();

    // Privacy
    void onClearStorage();
    void onClearSiteData();
    void onToggleTrackingBlocker();
    void onToggleHttpsOnly();
    void onToggleGpc();
    void onToggleFingerprinting();

    // Settings / UI
    void onShowSettings();
    void onZoomIn();
    void onZoomOut();
    void onResetZoom();
    void onToggleBookmarksBar();
    void onShowDownloads();
    void onShowHistory();
    void onSwitchProfile();
    void onToggleExtensions();
    void onAboutFrint();

    // === NEW FEATURES ===
    // Bookmark
    void onAddBookmark();
    void onShowBookmarks();

    // Reading List
    void onAddToReadingList();
    void onShowReadingList();

    // Notes
    void onShowNotes();

    // Screenshot
    void onScreenshot();

    // Translation
    void onTranslatePage();

    // Night Mode
    void onToggleNightMode();

    // Voice Search
    void onVoiceSearch();

    // Window Transparency
    void onSetWindowOpacity(int value);

    // Restart
    void onRestartBrowser();

    // Check for updates
    void onCheckForUpdates();
    void onAutoUpdateCheck();

    // Connection status change
    void onConnectivityChanged(bool online);

    // Show Quick Dial
    void onShowQuickDial();

    // Show shortcuts
    void onShowShortcuts();

    // Clear data with shortcut
    void onClearDataShortcut();

    // Cookie auto-clear
    void onToggleAutoCookieClear();

    // === PERFORMANCE ===
    void applyPerformanceSettings();
    void onToggleHardwareAcceleration();
    void onToggleSSEAVX();
    void onShowPerformanceStatus();
    void updatePerformanceIndicator();

    void clearAllSessionData();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupToolBar();
    void setupStatusBar();
    void setupMenuBar();
    void setupStyleSheet();
    void animateProperty(QWidget *target, const QByteArray &prop,
                         const QVariant &from, const QVariant &to, int duration = 300);
    void applyTheme(const QString &themeName);
    WebView *currentWebView() const;
    void updateAddressBar(const QUrl &url);
    void updateNavButtons();
    void updatePrivacyIndicators();
    void updateTabCountTitle();
    void updateConnectionStatus();
    void setupNotifications();
    void setupAutoUpdateCheck();
    void setupConnectivityMonitor();
    void applyNightMode(bool enabled);
    void showNotification(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information);

    // Widgets
    QTabWidget *m_tabWidget;
    QToolBar *m_toolBar;
    QLineEdit *m_addressBar;
    QToolButton *m_newTabBtn;
    QToolButton *m_backBtn;
    QToolButton *m_forwardBtn;
    QToolButton *m_reloadBtn;
    QToolButton *m_homeBtn;
    QToolButton *m_extensionsBtn;
    QToolButton *m_menuBtn;
    QToolButton *m_bookmarkBtn;
    QToolButton *m_nightModeBtn;
    QToolButton *m_readingListBtn;

    // Status bar
    QLabel *m_statusLabel;
    QLabel *m_performanceIndicator;
    QProgressBar *m_progressBar;
    QLabel *m_privacyIndicator;
    QLabel *m_profileLabel;
    QLabel *m_securityLock;
    QLabel *m_trackerCountLabel;
    QLabel *m_pageLoadTimeLabel;
    QLabel *m_connectionStatusLabel;
    QProgressBar *m_downloadProgressBar;

    // Menus
    QMenu *m_fileMenu;
    QMenu *m_privacyMenu;
    QMenu *m_hamburgerMenu;
    QMenu *m_extensionsMenu;
    QMenu *m_profileMenu;
    QMenu *m_bookmarksMenu;
    QMenu *m_toolsMenu;

    // Actions
    QAction *m_hwAccelAction;
    QAction *m_sseAvxAction;
    QAction *m_trackingAction;
    QAction *m_httpsAction;
    QAction *m_gpcAction;
    QAction *m_fingerprintingAction;
    QAction *m_antiFingerprintAction;
    QAction *m_webrtcAction;
    QAction *m_etagAction;
    QAction *m_bookmarksBarAction;
    QAction *m_nightModeAction;
    QAction *m_translateAction;
    QAction *m_spellCheckAction;

    // System tray
    QSystemTrayIcon *m_trayIcon;

    // Timers
    QTimer *m_autoUpdateTimer;
    QTimer *m_pageLoadTimer;

    // Connectivity
    QNetworkInformation *m_networkInfo;

    // State
    qreal m_animProgress = 0;
    bool m_extensionsVisible = false;
    bool m_nightMode = false;
    bool m_bookmarksBarVisible = false;
    int m_blockedTrackerCount = 0;
    QElapsedTimer m_loadTimer;
    QWidget *m_devToolsWindow = nullptr;
    QWebEngineView *m_devToolsView = nullptr;
};

} // namespace Frint

#endif
