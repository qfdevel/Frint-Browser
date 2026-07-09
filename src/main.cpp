#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

#include "browser_window.h"
#include "settings_manager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("FrintBrowser");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("FrintBrowser");
    app.setOrganizationDomain("frintbrowser.dev");

    // Command line options
    QCommandLineParser parser;
    parser.setApplicationDescription("Frint Browser - Privacy-first web browser");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("url", "URL to open", "[url]");
    parser.process(app);

    // Initialize settings (secure defaults loaded in BrowserWindow constructor)
    auto &settings = Frint::SettingsManager::instance();
    Q_UNUSED(settings);

    // Create and show browser window
    Frint::BrowserWindow browserWindow;
    browserWindow.show();

    // Open URL from command line if provided
    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        QUrl url(args.first());
        if (!url.scheme().isEmpty()) {
            browserWindow.addTab(url);
        } else {
            browserWindow.addTab(QUrl("https://" + args.first()));
        }
    }

    return app.exec();
}
