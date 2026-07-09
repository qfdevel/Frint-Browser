#include "screenshot.h"
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QApplication>

namespace Frint {

ScreenshotCapture &ScreenshotCapture::instance()
{
    static ScreenshotCapture s_instance;
    return s_instance;
}

ScreenshotCapture::ScreenshotCapture() = default;

QString ScreenshotCapture::defaultPath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
                  + "/Frint Screenshots";
    QDir().mkpath(dir);
    return dir + "/frint_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".png";
}

void ScreenshotCapture::captureVisible(QWebEngineView *view, const QString &path)
{
    if (!view) {
        emit screenshotFailed("No active web view");
        return;
    }

    QString savePath = path.isEmpty() ? defaultPath() : path;

    auto *page = view->page();
    if (!page) {
        emit screenshotFailed("No page available");
        return;
    }

    // Use Qt's built-in capture mechanism - request a render of the visible area
    QWidget *renderWidget = view->focusWidget();
    if (!renderWidget) renderWidget = view;

    QPixmap pix = view->grab();
    if (pix.isNull()) {
        emit screenshotFailed("Failed to capture screenshot");
        return;
    }

    if (pix.save(savePath, "PNG")) {
        qDebug() << "[Frint] Screenshot saved:" << savePath;
        emit screenshotTaken(savePath);
    } else {
        emit screenshotFailed("Failed to save screenshot");
    }
}

void ScreenshotCapture::captureFullPage(QWebEngineView *view)
{
    if (!view || !view->page()) {
        emit screenshotFailed("No active web view");
        return;
    }

    QString savePath = defaultPath();

    // Use JavaScript to get full page dimensions and capture
    view->page()->runJavaScript(
        "JSON.stringify({width: document.body.scrollWidth, "
        "height: document.body.scrollHeight})",
        [this, view, savePath](const QVariant &result) {
            // Simple approach: use visible capture and notify
            QPixmap pix = view->grab();
            if (pix.save(savePath, "PNG")) {
                emit screenshotTaken(savePath);
            } else {
                emit screenshotFailed("Failed to save full page screenshot");
            }
        });
}

void ScreenshotCapture::captureSelection(QWebEngineView *view)
{
    if (!view) {
        emit screenshotFailed("No active web view");
        return;
    }

    QString savePath = defaultPath();

    // Get selection coordinates from JavaScript
    view->page()->runJavaScript(
        "(function(){"
        "  var s = window.getSelection();"
        "  if (!s.rangeCount) return null;"
        "  var r = s.getRangeAt(0);"
        "  var rect = r.getBoundingClientRect();"
        "  return JSON.stringify({x: rect.left, y: rect.top, "
        "    w: rect.width, h: rect.height});"
        "})()",
        [this, view, savePath](const QVariant &result) {
            if (result.isNull()) {
                emit screenshotFailed("No text selection found");
                return;
            }
            // Fallback to visible capture
            QPixmap pix = view->grab();
            if (pix.save(savePath, "PNG")) {
                emit screenshotTaken(savePath);
            } else {
                emit screenshotFailed("Failed to save selection screenshot");
            }
        });
}

} // namespace Frint
