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

namespace Frint {

class WebView;

class BrowserWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit BrowserWindow(QWidget *parent = nullptr);
    ~BrowserWindow() override;

    void addTab(const QUrl &url = QUrl("about:blank"));
    void removeTab(int index);

private slots:
    void onNewTab();
    void onCloseTab(int index);
    void onTabChanged(int index);
    void onUrlChanged(const QUrl &url);
    void onTitleChanged(const QString &title);
    void onLoadStarted();
    void onLoadFinished(bool ok);
    void onLoadProgress(int progress);
    void onAddressBarReturnPressed();
    void onBack();
    void onForward();
    void onReload();
    void onClearStorage();
    void onClearSiteData();
    void onToggleTrackingBlocker();
    void onToggleHttpsOnly();
    void onToggleGpc();
    void onToggleFingerprinting();

private:
    void setupToolBar();
    void setupStatusBar();
    void setupMenuBar();
    WebView *currentWebView() const;
    void updateAddressBar(const QUrl &url);
    void updateNavButtons();
    void updatePrivacyIndicators();

    QTabWidget *m_tabWidget;
    QToolBar *m_toolBar;
    QLineEdit *m_addressBar;
    QPushButton *m_newTabBtn;
    QPushButton *m_backBtn;
    QPushButton *m_forwardBtn;
    QPushButton *m_reloadBtn;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    QLabel *m_privacyIndicator;

    QMenu *m_fileMenu;
    QMenu *m_privacyMenu;

    // Privacy menu actions (toggleable)
    QAction *m_trackingAction;
    QAction *m_httpsAction;
    QAction *m_gpcAction;
    QAction *m_fingerprintingAction;
};

} // namespace Frint

#endif // FRINT_BROWSER_WINDOW_H
