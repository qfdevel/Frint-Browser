#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <cpuid.h>
#include <GL/gl.h>

#include <sys/resource.h>

#include "browser_window.h"
#include "settings_manager.h"
#include "setup_wizard.h"

using namespace Frint;

int main(int argc, char *argv[])
{
    // Increase stack size for deep DOM trees
    const rlim_t stackSize = 32 * 1024 * 1024;
    struct rlimit rl;
    getrlimit(RLIMIT_STACK, &rl);
    if (rl.rlim_cur < stackSize) {
        rl.rlim_cur = stackSize;
        setrlimit(RLIMIT_STACK, &rl);
    }

    QApplication app(argc, argv);
    app.setApplicationName("Frint Browser");
    app.setOrganizationName("FrintBrowser");
    app.setOrganizationDomain("frintbrowser.dev");
    app.setApplicationVersion("0.1.0");

    // ── Command line ──
    QCommandLineParser parser;
    parser.setApplicationDescription("Frint Browser - Privacy-first, telemetry-free web browser");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({{"n", "new-tab"}, "Open a new tab with the specified URL", "url"});
    parser.addOption({{"p", "private"}, "Open a private browsing window"});
    parser.addPositionalArgument("url", "URL to open", "[url]");
    parser.process(app);

    // ── Init settings ──
    auto &settings = SettingsManager::instance();
    settings.loadDefaults();

    // ── Setup wizard (first run OR new profile) ──
    // Check if we should show setup wizard
    QString frintDir = settings.frintDataDir();
    QString profilesDir = frintDir + "/profiles";
    QDir().mkpath(profilesDir);

    bool firstRun = !QFile::exists(frintDir + "/.initialized");
    bool forceSetup = parser.isSet("private") || parser.isSet("new-tab");
    QString requestedProfile;

    // Parse any profile argument
    for (const auto &arg : parser.positionalArguments()) {
        if (arg.startsWith("--profile=")) {
            requestedProfile = arg.mid(10);
        }
    }

    // Show setup wizard if first run, or if creating a new profile
    if (firstRun) {
        SetupWizard wizard;
        if (wizard.exec() == QWizard::Accepted) {
            QString profileName = wizard.profileName();
            QString configPath = wizard.configPath();
            QString theme = wizard.selectedTheme();

            // Create profile config
            QSettings config(configPath, QSettings::IniFormat);
            config.setValue("profile/name", profileName);
            config.setValue("profile/language", wizard.selectedLanguage());
            config.setValue("profile/theme", theme);
            config.setValue("profile/created", QDateTime::currentDateTime().toString(Qt::ISODate));
            config.setValue("profile/tutorial_shown", wizard.showTutorial());
            config.sync();

            // Update global settings
            settings.setActiveProfile(profileName);
            settings.setTheme(theme);
            settings.setValue("first_run_complete", true);

            // Mark initialized
            QFile initFile(frintDir + "/.initialized");
            if (initFile.open(QIODevice::WriteOnly)) {
                initFile.write("Frint Browser initialized\n");
            }
            initFile.close();

            qDebug() << "[Frint] Setup complete for profile:" << profileName
                     << "config:" << configPath;
        } else {
            // User cancelled - use defaults
            settings.setActiveProfile("Default");
            settings.setTheme("catppuccin-mocha");
        }
    }

    // ── CPU/GPU Detection & Performance Settings ──
    {
        auto &s = SettingsManager::instance();

        // Detect CPU features
        QStringList cpuFeatures;
#ifdef __x86_64__
        unsigned int eax, ebx, ecx, edx;
        if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
            if (ecx & (1 << 20)) cpuFeatures << "SSE4.2";
        }
        if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
            if (ebx & (1 << 5))  cpuFeatures << "AVX2";
            if (ebx & (1 << 16)) cpuFeatures << "AVX-512F";
        }
        unsigned int eax1, ebx1, ecx1, edx1;
        if (__get_cpuid(1, &eax1, &ebx1, &ecx1, &edx1)) {
            if (ecx1 & (1 << 28) && !cpuFeatures.contains("AVX2")) cpuFeatures << "AVX";
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
            if (flags.contains("sse4_2")) cpuFeatures << "SSE4.2";
            if (flags.contains("avx"))   cpuFeatures << "AVX";
            if (flags.contains("avx2"))  cpuFeatures << "AVX2";
            if (flags.contains("avx512f")) cpuFeatures << "AVX-512F";
            cpuinfo.close();
        }
#endif

        // Detect GPU
        QString gpuModel = "Unknown";
        {
            QOpenGLContext ctx;
            if (ctx.create()) {
                const char *renderer = (const char *)glGetString(GL_RENDERER);
                if (renderer) gpuModel = QString::fromUtf8(renderer);
            }
        }

        qDebug() << "[Frint] CPU features:" << cpuFeatures.join(", ");
        qDebug() << "[Frint] GPU:" << gpuModel;

        // Store detected features for reference
        s.setValue("detected_cpu_features", cpuFeatures.join(", "));
        s.setValue("detected_gpu", gpuModel);

        // Compatibility check: disable HW accel if no GPU detected
        bool hasGpu = !gpuModel.contains("Unknown") && !gpuModel.isEmpty();
        if (!hasGpu && s.isHardwareAccelerationEnabled()) {
            qDebug() << "[Frint] No GPU detected, disabling hardware acceleration";
            s.setHardwareAccelerationEnabled(false);
            s.setPerformanceMode("compatibility");
        }

        // If CPU is very old (no SSE4.2), disable SSE/AVX
        bool hasModernCpu = cpuFeatures.contains("SSE4.2") || cpuFeatures.contains("AVX");
        if (!hasModernCpu && s.isSSEAVXEnabled()) {
            qDebug() << "[Frint] Old CPU detected, disabling SSE/AVX optimizations";
            s.setSSEAVXEnabled(false);
        }

        s.sync();
    }

    // ── Create main window ──
    BrowserWindow window;
    window.show();

    // ── Load initial URL ──
    QStringList args = parser.positionalArguments();
    QUrl initialUrl;

    if (!args.isEmpty() && !args[0].startsWith("--")) {
        QString arg = args[0];
        if (arg.startsWith("http://") || arg.startsWith("https://") ||
            arg.startsWith("file://") || arg.startsWith("about:")) {
            initialUrl = QUrl(arg);
        } else {
            initialUrl = QUrl("https://" + arg);
        }
    }

    if (initialUrl.isValid()) {
        window.addTab(initialUrl);
    } else {
        window.addTab(QUrl(settings.homePage()));
    }

    return app.exec();
}
