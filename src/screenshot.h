#ifndef FRINT_SCREENSHOT_H
#define FRINT_SCREENSHOT_H

#include <QObject>
#include <QString>
#include <QDate>

class QWebEngineView;

namespace Frint {

class ScreenshotCapture : public QObject {
    Q_OBJECT

public:
    static ScreenshotCapture &instance();

    void captureVisible(QWebEngineView *view, const QString &path = QString());
    void captureFullPage(QWebEngineView *view);
    void captureSelection(QWebEngineView *view);

signals:
    void screenshotTaken(const QString &path);
    void screenshotFailed(const QString &error);

private:
    ScreenshotCapture();
    ~ScreenshotCapture() = default;
    ScreenshotCapture(const ScreenshotCapture &) = delete;
    ScreenshotCapture &operator=(const ScreenshotCapture &) = delete;

    QString defaultPath() const;
};

} // namespace Frint

#endif // FRINT_SCREENSHOT_H
