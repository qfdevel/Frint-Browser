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

    // ── Command line parsing ──────────────────────────────────────────
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Frint Browser - Privacy-first, telemetry-free web browser");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOption(QCommandLineOption("new-tab", "Open a new tab with the specified URL",
                                        "url"));

    parser.addOption(QCommandLineOption("private", "Open a private browsing window"));

    parser.addPositionalArgument("url", "URL to open", "[url]");

    parser.process(app);

    // ── Initialize settings ───────────────────────────────────────────
    auto &settings = Frint::SettingsManager::instance();
    Q_UNUSED(settings);

    // ── Create window ─────────────────────────────────────────────────
    auto *window = new Frint::BrowserWindow();
    window->show();

    // ── Handle CLI URL ────────────────────────────────────────────────
    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        QUrl url(args.first());
        if (!url.scheme().isEmpty()) {
            window->addTab(url);
        } else {
            window->addTab(QUrl("https://" + args.first()));
        }
    }

    return app.exec();
}
