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
#include <QMap>

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
    void onNewTabClicked();
    void onCloseTabClicked(int index);
    void onTabChanged(int index);
    void onUrlChanged(const QUrl &url);
    void onTitleChanged(const QString &title);
    void onLoadStarted();
    void onLoadFinished(bool ok);
    void onLoadProgress(int progress);
    void onNavigation(const QUrl &url);
    void onAddressBarReturnPressed();

private:
    void setupToolBar();
    void setupStatusBar();
    void setupMenuBar();
    WebView *currentWebView() const;
    void updateAddressBar(const QUrl &url);

    QTabWidget *m_tabWidget;
    QToolBar *m_toolBar;
    QLineEdit *m_addressBar;
    QPushButton *m_newTabButton;
    QPushButton *m_backButton;
    QPushButton *m_forwardButton;
    QPushButton *m_reloadButton;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;

    QMenu *m_fileMenu;
    QMenu *m_privacyMenu;

    // Keep track of tab URL mappings
    QMap<int, QUrl> m_tabUrls;
};

} // namespace Frint

#endif // FRINT_BROWSER_WINDOW_H
