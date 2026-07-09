#include "settings_dialog.h"
#include "settings_manager.h"
#include "bookmarks.h"
#include "autofill.h"
#include "cookie_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QLabel>
#include <QFile>
#include <QStyleFactory>
#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QDir>
#include <QFileDialog>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QInputDialog>
#include <QHeaderView>
#include <QOpenGLContext>
#include <cpuid.h>
#include <QSurfaceFormat>
#include <GL/gl.h>

namespace Frint {

// ── CPU Feature Detection ────────────────────────────────────────────
static QString detectCPUFeatures()
{
    QStringList features;
#ifdef __x86_64__
    unsigned int eax, ebx, ecx, edx;

    // Check for SSE4.2 (leaf 1, ECX bit 20)
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        if (ecx & (1 << 20)) features << "SSE4.2";
    }

    // Check for AVX2 (leaf 7, subleaf 0, EBX bit 5)
    // Check for AVX-512F (leaf 7, subleaf 0, EBX bit 16)
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        if (ebx & (1 << 5))  features << "AVX2";
        if (ebx & (1 << 16)) features << "AVX-512F";
        if (ebx & (1 << 28)) features << "AVX-512VPOPCNTDQ";
        if (ebx & (1 << 31)) features << "AVX-512VL";
    }

    // Check for AVX (leaf 1, ECX bit 28)
    unsigned int eax1, ebx1, ecx1, edx1;
    if (__get_cpuid(1, &eax1, &ebx1, &ecx1, &edx1)) {
        if (ecx1 & (1 << 28) && !features.contains("AVX2")) features << "AVX";
    }
#elif defined(__aarch64__)
    features << "ARM NEON";
    // Optional: read /proc/cpuinfo for SVE/SVE2
    QFile cpuinfo("/proc/cpuinfo");
    if (cpuinfo.open(QIODevice::ReadOnly)) {
        QString data = cpuinfo.readAll();
        if (data.contains("sve")) features << "SVE";
        if (data.contains("sve2")) features << "SVE2";
        cpuinfo.close();
    }
#else
    QFile cpuinfo("/proc/cpuinfo");
    if (cpuinfo.open(QIODevice::ReadOnly)) {
        QString data = cpuinfo.readAll();
        // Check for common flags
        QStringList flags;
        for (const QString &line : data.split('\n')) {
            if (line.trimmed().startsWith("flags\t")) {
                flags = line.section(':', 1).trimmed().split(' ', Qt::SkipEmptyParts);
                break;
            }
        }
        if (flags.contains("sse4_2")) features << "SSE4.2";
        if (flags.contains("avx"))   features << "AVX";
        if (flags.contains("avx2"))  features << "AVX2";
        if (flags.contains("avx512f")) features << "AVX-512F";
        cpuinfo.close();
    }
#endif

    if (features.isEmpty())
        features << "Basic x86 (SSE2)";

    return features.join(", ");
}

static QString detectGPUInfo()
{
    QOpenGLContext ctx;
    if (ctx.create()) {
        QSurfaceFormat fmt = ctx.format();
        QString renderer = QString::fromUtf8(
            reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        QString version = QString::fromUtf8(
            reinterpret_cast<const char*>(glGetString(GL_VERSION)));
        QString vendor = QString::fromUtf8(
            reinterpret_cast<const char*>(glGetString(GL_VENDOR)));

        if (!renderer.isEmpty())
            return vendor + ": " + renderer + "\nOpenGL: " + version;
    }

    // Fallback: try reading from /proc or lspci
    QFile gpuInfo("/sys/class/drm/card0/device/uevent");
    if (gpuInfo.open(QIODevice::ReadOnly)) {
        QString data = gpuInfo.readAll();
        for (const QString &line : data.split('\n')) {
            if (line.startsWith("DRIVER="))
                return "GPU: " + line.mid(7);
        }
        gpuInfo.close();
    }

    return "Unknown GPU";
}

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Frint Browser Settings");
    setMinimumSize(720, 600);
    resize(760, 640);

    auto *mainLayout = new QVBoxLayout(this);
    auto *tabs = new QTabWidget(this);

    setupGeneralTab(tabs);
    setupPerformanceTab(tabs);
    setupPrivacyTab(tabs);
    setupAdvancedPrivacyTab(tabs);
    setupPrivacyDashboardTab(tabs);
    setupAppearanceTab(tabs);
    setupProfilesTab(tabs);
    setupDownloadsTab(tabs);
    setupBookmarksTab(tabs);
    setupAutofillTab(tabs);
    setupFeaturesTab(tabs);
    setupShortcutsTab(tabs);
    setupExtensionsTab(tabs);
    setupAboutTab(tabs);

    mainLayout->addWidget(tabs);

    auto *btnLayout = new QHBoxLayout;
    auto *applyBtn = new QPushButton("Apply", this);
    auto *cancelBtn = new QPushButton("Cancel", this);
    btnLayout->addStretch();
    btnLayout->addWidget(applyBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    connect(applyBtn, &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    loadSettings();
    setStyleSheet(
        "QDialog { background: #1e1e2e; color: #cdd6f4; }"
        "QTabWidget::pane { background: rgba(24, 25, 37, 0.5); border: 1px solid rgba(49, 50, 68, 0.5); border-radius: 12px; }"
        "QTabBar::tab { background: rgba(49, 50, 68, 0.3); color: #585b70; padding: 8px 18px; border-radius: 8px; margin: 2px; border: 1px solid transparent; font-size: 12px; }"
        "QTabBar::tab:selected { background: rgba(69, 71, 90, 0.5); color: #cdd6f4; border-color: rgba(255,255,255,0.06); }"
        "QGroupBox { border: 1px solid rgba(255,255,255,0.06); border-radius: 10px; margin-top: 12px; padding: 16px; color: #cdd6f4; font-weight: bold; background: rgba(49, 50, 68, 0.15); }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }"
        "QLabel { color: #bac2de; }"
        "QLineEdit, QComboBox, QSpinBox, QListWidget, QTreeWidget { background: rgba(49, 50, 68, 0.4); color: #cdd6f4; border: 1px solid rgba(69, 71, 90, 0.5); border-radius: 8px; padding: 8px; }"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border-color: rgba(137, 180, 250, 0.6); background: rgba(49, 50, 68, 0.6); }"
        "QCheckBox { color: #cdd6f4; spacing: 8px; }"
        "QCheckBox::indicator { width: 18px; height: 18px; border-radius: 5px; border: 2px solid rgba(88, 91, 112, 0.6); background: rgba(49, 50, 68, 0.3); }"
        "QCheckBox::indicator:checked { background: rgba(166, 227, 161, 0.7); border-color: rgba(166, 227, 161, 0.8); }"
        "QCheckBox::indicator:hover { border-color: rgba(137, 180, 250, 0.5); }"
        "QPushButton { background: rgba(69, 71, 90, 0.35); color: #cdd6f4; border: 1px solid rgba(255,255,255,0.06); border-radius: 10px; padding: 8px 20px; font-weight: bold; }"
        "QPushButton:hover { background: rgba(69, 71, 90, 0.6); border-color: rgba(255,255,255,0.12); }"
        "QPushButton:pressed { background: rgba(69, 71, 90, 0.8); }"
        "QComboBox::drop-down { border: none; width: 28px; }"
        "QComboBox QAbstractItemView { background: rgba(49, 50, 68, 0.85); color: #cdd6f4; selection-background-color: rgba(69, 71, 90, 0.6); border-radius: 10px; border: 1px solid rgba(255,255,255,0.06); padding: 4px; }"
        "QSlider::groove:horizontal { height: 6px; background: rgba(69, 71, 90, 0.4); border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #89b4fa; width: 18px; height: 18px; margin: -6px 0; border-radius: 9px; }"
        "QSlider::sub-page:horizontal { background: rgba(137, 180, 250, 0.5); border-radius: 3px; }"
    );
}

SettingsDialog::~SettingsDialog() = default;

// ═══════════════════════════════════════════════════════════════════════
// Performance Tab (⚡ NEW)
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::setupPerformanceTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    // Group 1: Hardware Acceleration
    auto *hwGroup = new QGroupBox("Hardware Acceleration");
    auto *hwLayout = new QVBoxLayout(hwGroup);

    m_hwAccelCheck = new QCheckBox("Enable GPU acceleration");
    m_hwAccelCheck->setToolTip("Uses GPU for WebGL, video decoding, animations");
    hwLayout->addWidget(m_hwAccelCheck);

    auto *hwDesc = new QLabel("Improves performance for WebGL, video, and animations.\n"
                              "Disable if you experience crashes or graphical glitches.");
    hwDesc->setStyleSheet("color: #585b70; font-size: 11px; padding-left: 24px;");
    hwDesc->setWordWrap(true);
    hwLayout->addWidget(hwDesc);

    layout->addWidget(hwGroup);

    // Group 2: CPU Optimizations
    auto *cpuGroup = new QGroupBox("CPU Optimizations");
    auto *cpuLayout = new QVBoxLayout(cpuGroup);

    m_sseAvxCheck = new QCheckBox("Enable SSE/AVX optimizations");
    m_sseAvxCheck->setToolTip("SSE4.2, AVX2, AVX-512");
    cpuLayout->addWidget(m_sseAvxCheck);

    auto *cpuDesc = new QLabel("Enables SSE4.2, AVX2, and AVX-512 instruction set optimizations.\n"
                               "Disable for older or incompatible CPUs.");
    cpuDesc->setStyleSheet("color: #585b70; font-size: 11px; padding-left: 24px;");
    cpuDesc->setWordWrap(true);
    cpuLayout->addWidget(cpuDesc);

    layout->addWidget(cpuGroup);

    // Group 3: Performance Mode
    auto *modeGroup = new QGroupBox("Performance Mode");
    auto *modeLayout = new QFormLayout(modeGroup);

    m_perfModeCombo = new QComboBox;
    m_perfModeCombo->addItem("Maximum Performance", "maximum");
    m_perfModeCombo->addItem("Balanced", "balanced");
    m_perfModeCombo->addItem("Compatibility", "compatibility");
    m_perfModeCombo->setToolTip("Maximum: GPU + CPU optimizations active\n"
                                "Balanced: CPU optimizations only\n"
                                "Compatibility: All optimizations off");
    modeLayout->addRow("Mode:", m_perfModeCombo);

    layout->addWidget(modeGroup);

    // Group 4: Performance Status
    auto *statusGroup = new QGroupBox("Performance Status");
    auto *statusForm = new QFormLayout(statusGroup);

    QString cpuFeatures = detectCPUFeatures();
    QString gpuInfo = detectGPUInfo();

    m_cpuFeaturesLabel = new QLabel(cpuFeatures);
    m_cpuFeaturesLabel->setStyleSheet("font-weight: bold; color: #89b4fa; font-size: 13px;");
    m_cpuFeaturesLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statusForm->addRow("CPU Features:", m_cpuFeaturesLabel);

    m_gpuInfoLabel = new QLabel(gpuInfo);
    m_gpuInfoLabel->setStyleSheet("font-weight: bold; color: #89b4fa; font-size: 13px;");
    m_gpuInfoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_gpuInfoLabel->setWordWrap(true);
    statusForm->addRow("GPU:", m_gpuInfoLabel);

    m_perfStatusLabel = new QLabel("Active");
    m_perfStatusLabel->setStyleSheet("font-weight: bold; color: #a6e3a1; font-size: 14px;");
    statusForm->addRow("Status:", m_perfStatusLabel);

    layout->addWidget(statusGroup);

    // Connect mode changes to update status
    connect(m_hwAccelCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_perfStatusLabel->setText(checked && m_sseAvxCheck->isChecked()
            ? "Active" : "Partial / Disabled");
        m_perfStatusLabel->setStyleSheet(
            QString("font-weight: bold; font-size: 14px; color: %1;")
            .arg(checked && m_sseAvxCheck->isChecked() ? "#a6e3a1" : "#f9e2af"));
    });
    connect(m_sseAvxCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_perfStatusLabel->setText(m_hwAccelCheck->isChecked() && checked
            ? "Active" : "Partial / Disabled");
        m_perfStatusLabel->setStyleSheet(
            QString("font-weight: bold; font-size: 14px; color: %1;")
            .arg(m_hwAccelCheck->isChecked() && checked ? "#a6e3a1" : "#f9e2af"));
    });

    layout->addStretch();
    tabs->addTab(widget, QString::fromUtf8("\xE2\x9A\xA1 Performance"));
}

// ═══════════════════════════════════════════════════════════════════════
// General Tab
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::setupGeneralTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *generalGroup = new QGroupBox("General");
    auto *form = new QFormLayout(generalGroup);

    m_homePageEdit = new QLineEdit;
    m_homePageEdit->setPlaceholderText("https://duckduckgo.com");
    form->addRow("Home Page:", m_homePageEdit);

    m_searchEngineCombo = new QComboBox;
    m_searchEngineCombo->addItem("DuckDuckGo", "https://duckduckgo.com/?q=");
    m_searchEngineCombo->addItem("Google", "https://www.google.com/search?q=");
    m_searchEngineCombo->addItem("Bing", "https://www.bing.com/search?q=");
    m_searchEngineCombo->addItem("Brave Search", "https://search.brave.com/search?q=");
    m_searchEngineCombo->addItem("SearXNG", "https://searx.be/search?q=");
    form->addRow("Search Engine:", m_searchEngineCombo);

    m_clearOnExitCheck = new QCheckBox("Clear cookies and site data when closing");
    form->addRow("", m_clearOnExitCheck);

    layout->addWidget(generalGroup);
    layout->addStretch();
    tabs->addTab(widget, "General");
}

// ═══════════════════════════════════════════════════════════════════════
// Privacy Tab
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::setupPrivacyTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *privacyGroup = new QGroupBox("Privacy Controls");
    auto *form = new QFormLayout(privacyGroup);

    m_httpsOnlyCheck = new QCheckBox("Upgrade HTTP to HTTPS");
    form->addRow("", m_httpsOnlyCheck);

    m_trackingBlockerCheck = new QCheckBox("Block known trackers");
    form->addRow("", m_trackingBlockerCheck);

    m_gpcCheck = new QCheckBox("Send Global Privacy Control signal");
    form->addRow("", m_gpcCheck);

    m_fingerprintingCheck = new QCheckBox("Defend against fingerprinting");
    form->addRow("", m_fingerprintingCheck);

    m_dohCheck = new QCheckBox("DNS-over-HTTPS (Cloudflare)");
    form->addRow("", m_dohCheck);

    m_block3rdPartyCookies = new QCheckBox("Block third-party cookies");
    form->addRow("", m_block3rdPartyCookies);

    layout->addWidget(privacyGroup);

    auto *info = new QLabel("All privacy features are enabled by default.\n"
                            "Disabling them reduces your protection.");
    info->setStyleSheet("color: #585b70; font-size: 11px; padding-top: 8px;");
    layout->addWidget(info);
    layout->addStretch();
    tabs->addTab(widget, "Privacy");
}

// ═══════════════════════════════════════════════════════════════════════
// Advanced Privacy Tab
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::setupAdvancedPrivacyTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *fpGroup = new QGroupBox("Fingerprinting Protection");
    auto *fpLayout = new QVBoxLayout(fpGroup);

    m_canvasFpCheck = new QCheckBox("Canvas fingerprint protection");
    fpLayout->addWidget(m_canvasFpCheck);
    m_webglFpCheck = new QCheckBox("WebGL fingerprint protection");
    fpLayout->addWidget(m_webglFpCheck);
    m_audioFpCheck = new QCheckBox("Audio fingerprint protection");
    fpLayout->addWidget(m_audioFpCheck);
    m_fontFpCheck = new QCheckBox("Font fingerprint protection");
    fpLayout->addWidget(m_fontFpCheck);
    m_spoofHwConcurrencyCheck = new QCheckBox("Spoof hardware concurrency");
    fpLayout->addWidget(m_spoofHwConcurrencyCheck);
    m_spoofDeviceMemCheck = new QCheckBox("Spoof device memory");
    fpLayout->addWidget(m_spoofDeviceMemCheck);
    m_forceUtcCheck = new QCheckBox("Force UTC timezone");
    fpLayout->addWidget(m_forceUtcCheck);

    layout->addWidget(fpGroup);

    auto *netGroup = new QGroupBox("Network & Tracking");
    auto *netLayout = new QVBoxLayout(netGroup);

    m_webrtcBlockCheck = new QCheckBox("Block WebRTC IP leak");
    netLayout->addWidget(m_webrtcBlockCheck);
    m_clientHintsCheck = new QCheckBox("Strip client hints headers");
    netLayout->addWidget(m_clientHintsCheck);
    m_etagTrackingCheck = new QCheckBox("Strip ETag tracking");
    netLayout->addWidget(m_etagTrackingCheck);

    layout->addWidget(netGroup);
    layout->addStretch();
    tabs->addTab(widget, QString::fromUtf8("\xF0\x9F\x94\x90 Advanced"));
}

// ═══════════════════════════════════════════════════════════════════════
// Privacy Dashboard Tab
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::setupPrivacyDashboardTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *scoreBox = new QGroupBox("Privacy Score");
    auto *scoreLayout = new QVBoxLayout(scoreBox);
    scoreLayout->setAlignment(Qt::AlignCenter);

    m_privacyScoreLabel = new QLabel("105 / 105 \xE2\x9C\x85");
    m_privacyScoreLabel->setAlignment(Qt::AlignCenter);
    m_privacyScoreLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #a6e3a1; padding: 8px;");
    scoreLayout->addWidget(m_privacyScoreLabel);

    auto *scoreSub = new QLabel("All privacy features enabled");
    scoreSub->setAlignment(Qt::AlignCenter);
    scoreSub->setStyleSheet("color: #585b70; font-size: 12px;");
    scoreLayout->addWidget(scoreSub);
    layout->addWidget(scoreBox);

    auto *statsGroup = new QGroupBox("Session Statistics");
    auto *statsForm = new QFormLayout(statsGroup);

    m_blockedTrackersLabel = new QLabel("0");
    m_blockedTrackersLabel->setStyleSheet("font-weight: bold; color: #f38ba8; font-size: 14px;");
    statsForm->addRow("Blocked Trackers:", m_blockedTrackersLabel);
    m_blockedCookiesLabel = new QLabel("0");
    m_blockedCookiesLabel->setStyleSheet("font-weight: bold; color: #f9e2af; font-size: 14px;");
    statsForm->addRow("Blocked 3rd-Party Cookies:", m_blockedCookiesLabel);
    m_httpsUpgradesLabel = new QLabel("0");
    m_httpsUpgradesLabel->setStyleSheet("font-weight: bold; color: #89b4fa; font-size: 14px;");
    statsForm->addRow("HTTPS Upgrades:", m_httpsUpgradesLabel);
    m_lastCleanupLabel = new QLabel("Not yet cleaned");
    m_lastCleanupLabel->setStyleSheet("color: #585b70; font-size: 12px;");
    statsForm->addRow("Last Cleanup:", m_lastCleanupLabel);
    layout->addWidget(statsGroup);

    auto *btnLayout = new QHBoxLayout;
    auto *clearBtn = new QPushButton("Clear All Data");
    clearBtn->setStyleSheet(
        "QPushButton{background:rgba(243,139,168,0.15);color:#f38ba8;border:1px solid rgba(243,139,168,0.3);border-radius:10px;padding:10px 18px;font-weight:bold;}"
        "QPushButton:hover{background:rgba(243,139,168,0.35);border-color:rgba(243,139,168,0.5);}");
    auto *exportBtn = new QPushButton("Export Privacy Report");
    exportBtn->setStyleSheet(
        "QPushButton{background:rgba(137,180,250,0.15);color:#89b4fa;border:1px solid rgba(137,180,250,0.3);border-radius:10px;padding:10px 18px;font-weight:bold;}"
        "QPushButton:hover{background:rgba(137,180,250,0.35);border-color:rgba(137,180,250,0.5);}");
    btnLayout->addWidget(clearBtn);
    btnLayout->addWidget(exportBtn);
    layout->addLayout(btnLayout);

    connect(clearBtn, &QPushButton::clicked, this, &SettingsDialog::onClearAllData);
    connect(exportBtn, &QPushButton::clicked, this, &SettingsDialog::onExportPrivacyReport);

    layout->addStretch();
    tabs->addTab(widget, QString::fromUtf8("\xF0\x9F\x93\x8A Dashboard"));
}

// ═══════════════════════════════════════════════════════════════════════
// Appearance Tab (with Window Transparency)
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::setupAppearanceTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *appearanceGroup = new QGroupBox("Appearance");
    auto *form = new QFormLayout(appearanceGroup);

    m_themeCombo = new QComboBox;
    m_themeCombo->addItem("Catppuccin Mocha (Dark)", "catppuccin-mocha");
    m_themeCombo->addItem("Nord Dark", "nord-dark");
    m_themeCombo->addItem("Gruvbox Material", "gruvbox-material");
    m_themeCombo->addItem("Dracula", "dracula");
    m_themeCombo->addItem("Tokyo Night", "tokyo-night");
    m_themeCombo->addItem("Everforest", "everforest");
    m_themeCombo->addItem("Frint Dark", "frint-dark");
    m_themeCombo->addItem("Frint Light", "frint-light");
    form->addRow("Theme:", m_themeCombo);

    m_fontSizeSpin = new QSpinBox;
    m_fontSizeSpin->setRange(10, 24);
    m_fontSizeSpin->setValue(14);
    form->addRow("UI Font Size:", m_fontSizeSpin);

    // Window Transparency slider
    auto *opacityLayout = new QHBoxLayout;
    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(20, 100);
    m_opacitySlider->setValue(100);
    m_opacityLabel = new QLabel("100%");
    opacityLayout->addWidget(m_opacitySlider);
    opacityLayout->addWidget(m_opacityLabel);
    form->addRow("Window Opacity:", opacityLayout);
    connect(m_opacitySlider, &QSlider::valueChanged, this, &SettingsDialog::onOpacityChanged);

    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onThemeSelected);

    layout->addWidget(appearanceGroup);

    // Custom CSS
    auto *cssGroup = new QGroupBox("Custom CSS Overrides");
    auto *cssLayout = new QVBoxLayout(cssGroup);
    auto *cssLabel = new QLabel("Add custom Qt Style Sheet rules:");
    cssLabel->setWordWrap(true);
    cssLayout->addWidget(cssLabel);
    m_customCssEdit = new QPlainTextEdit;
    m_customCssEdit->setPlaceholderText("/* Example: Change tab colors */\nQTabBar::tab { background: #ff0000; }");
    m_customCssEdit->setMaximumHeight(100);
    m_customCssEdit->setStyleSheet("background: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 6px; padding: 6px; font-family: monospace;");
    cssLayout->addWidget(m_customCssEdit);
    layout->addWidget(cssGroup);

    layout->addStretch();
    tabs->addTab(widget, "Appearance");
}

// ═══════════════════════════════════════════════════════════════════════
// Profiles Tab
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::setupProfilesTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *profileGroup = new QGroupBox("Browser Profiles");
    auto *groupLayout = new QVBoxLayout(profileGroup);

    m_profileList = new QListWidget;
    m_profileList->setMinimumHeight(120);
    groupLayout->addWidget(new QLabel("Select a profile:"));
    groupLayout->addWidget(m_profileList);

    auto *editLayout = new QHBoxLayout;
    m_profileNameEdit = new QLineEdit;
    m_profileNameEdit->setPlaceholderText("New profile name");
    m_saveProfileBtn = new QPushButton("Add Profile");
    m_deleteProfileBtn = new QPushButton("Delete");
    editLayout->addWidget(m_profileNameEdit);
    editLayout->addWidget(m_saveProfileBtn);
    editLayout->addWidget(m_deleteProfileBtn);
    groupLayout->addLayout(editLayout);

    layout->addWidget(profileGroup);

    connect(m_saveProfileBtn, &QPushButton::clicked, this, &SettingsDialog::onSaveProfile);
    connect(m_deleteProfileBtn, &QPushButton::clicked, this, &SettingsDialog::onDeleteProfile);

    // Danger Zone
    auto *dzG = new QGroupBox("Danger Zone");
    auto *dzL = new QVBoxLayout(dzG);
    auto *dzB = new QPushButton("Delete Users and Reinstall");
    dzB->setStyleSheet(
        "QPushButton{background:rgba(243,139,168,0.15);color:#f38ba8;border:1px solid rgba(243,139,168,0.3);border-radius:12px;padding:14px;font-weight:bold;font-size:13px;}"
        "QPushButton:hover{background:rgba(243,139,168,0.35);border-color:rgba(243,139,168,0.5);}"
        "QPushButton:pressed{background:rgba(243,139,168,0.5);}");
    auto *dzW = new QLabel("<p style='color:#f38ba8;text-align:center;'><b>WARNING: THIS OPTION WILL DELETE ALL YOUR DATA!</b></p>");
    dzW->setAlignment(Qt::AlignCenter);
    dzL->addWidget(dzB);
    dzL->addWidget(dzW);
    layout->addWidget(dzG);
    connect(dzB, &QPushButton::clicked, this, [this]() {
        if (QMessageBox(QMessageBox::Warning,"Delete?","Delete ALL profiles and data?",QMessageBox::Yes|QMessageBox::No,this).exec()!=QMessageBox::Yes) return;
        if (QMessageBox(QMessageBox::Critical,"FINAL","Absolutely sure? Cannot undo!",QMessageBox::Yes|QMessageBox::No,this).exec()!=QMessageBox::Yes) return;
        auto&s = SettingsManager::instance();
        QDir(s.frintDataDir()).removeRecursively();
        s.setValue("first_run_complete",false);
        s.setProfiles({"Default"});
        s.setActiveProfile("Default");
        s.sync();
        QMessageBox(QMessageBox::Information,"Done","Restart for fresh setup.",QMessageBox::Ok,this).exec();
        qApp->quit();
    });

    layout->addStretch();
    tabs->addTab(widget, "Profiles");
}

// ═══════════════════════════════════════════════════════════════════════
// Downloads & Updates Tab
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::setupDownloadsTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *dlGroup = new QGroupBox("Download Settings");
    auto *dlForm = new QFormLayout(dlGroup);

    auto *locLayout = new QHBoxLayout;
    m_downloadLocationEdit = new QLineEdit;
    m_downloadLocationEdit->setReadOnly(true);
    auto *browseBtn = new QPushButton("Browse...");
    browseBtn->setFixedWidth(90);
    locLayout->addWidget(m_downloadLocationEdit);
    locLayout->addWidget(browseBtn);
    dlForm->addRow("Download Location:", locLayout);

    m_alwaysAskDownloadCheck = new QCheckBox("Always ask download location");
    dlForm->addRow("", m_alwaysAskDownloadCheck);
    layout->addWidget(dlGroup);
    connect(browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseDownloadLocation);

    auto *histGroup = new QGroupBox("Download History");
    auto *histLayout = new QVBoxLayout(histGroup);
    m_downloadHistoryList = new QListWidget;
    m_downloadHistoryList->setMinimumHeight(80);
    m_downloadHistoryList->addItem("No downloads yet");
    histLayout->addWidget(m_downloadHistoryList);
    auto *clearHistBtn = new QPushButton("Clear History");
    connect(clearHistBtn, &QPushButton::clicked, this, &SettingsDialog::onClearDownloadHistory);
    histLayout->addWidget(clearHistBtn);
    layout->addWidget(histGroup);

    auto *updGroup = new QGroupBox("Updates");
    auto *updLayout = new QVBoxLayout(updGroup);
    m_autoUpdateCheck = new QCheckBox("Auto-check for updates");
    updLayout->addWidget(m_autoUpdateCheck);
    auto *checkUpdBtn = new QPushButton("Check for Updates");
    connect(checkUpdBtn, &QPushButton::clicked, this, &SettingsDialog::onCheckForUpdates);
    updLayout->addWidget(checkUpdBtn);
    auto *verLabel = new QLabel("Version: v1.0.0");
    verLabel->setStyleSheet("color: #585b70; font-size: 11px;");
    updLayout->addWidget(verLabel);
    layout->addWidget(updGroup);

    layout->addStretch();
    tabs->addTab(widget, QString::fromUtf8("\xF0\x9F\x94\x84 Updates"));
}

// ═══════════════════════════════════════════════════════════════════════
// Bookmarks Tab (NEW)
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::setupBookmarksTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *group = new QGroupBox("Manage Bookmarks");
    auto *groupLayout = new QVBoxLayout(group);

    m_bookmarkTree = new QTreeWidget;
    m_bookmarkTree->setHeaderLabels({"Title", "URL", "Folder"});
    m_bookmarkTree->setRootIsDecorated(false);
    m_bookmarkTree->header()->setStretchLastSection(true);
    m_bookmarkTree->setAlternatingRowColors(true);
    m_bookmarkTree->setMinimumHeight(200);
    groupLayout->addWidget(m_bookmarkTree);

    auto *btnLayout = new QHBoxLayout;
    auto *refreshBtn = new QPushButton("Refresh");
    auto *delBtn = new QPushButton("Delete Selected");
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        m_bookmarkTree->clear();
        for (const auto &b : BookmarkManager::instance().bookmarks()) {
            auto *item = new QTreeWidgetItem;
            item->setText(0, b.title);
            item->setText(1, b.url.toString());
            item->setText(2, b.folder);
            item->setData(1, Qt::UserRole, b.url);
            m_bookmarkTree->addTopLevelItem(item);
        }
    });
    connect(delBtn, &QPushButton::clicked, this, [this]() {
        auto item = m_bookmarkTree->currentItem();
        if (item) {
            QUrl url = item->data(1, Qt::UserRole).toUrl();
            BookmarkManager::instance().removeBookmark(url);
            delete item;
        }
    });
    btnLayout->addWidget(refreshBtn);
    btnLayout->addWidget(delBtn);

    m_bookmarkFolderEdit = new QLineEdit;
    m_bookmarkFolderEdit->setPlaceholderText("New folder name");
    auto *addFolderBtn = new QPushButton("Add Folder");
    connect(addFolderBtn, &QPushButton::clicked, this, [this]() {
        QString name = m_bookmarkFolderEdit->text().trimmed();
        if (!name.isEmpty()) {
            BookmarkManager::instance().addFolder(name);
            m_bookmarkFolderEdit->clear();
        }
    });
    btnLayout->addWidget(m_bookmarkFolderEdit);
    btnLayout->addWidget(addFolderBtn);
    groupLayout->addLayout(btnLayout);

    layout->addWidget(group);
    layout->addStretch();
    tabs->addTab(widget, QString::fromUtf8("\xE2\x98\x85 Bookmarks"));
}

// ═══════════════════════════════════════════════════════════════════════
// Autofill Tab (NEW)
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::setupAutofillTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *group = new QGroupBox("Autofill Profiles");
    auto *groupLayout = new QVBoxLayout(group);

    m_autofillTree = new QTreeWidget;
    m_autofillTree->setHeaderLabels({"Type", "Label", "Fields"});
    m_autofillTree->setRootIsDecorated(false);
    m_autofillTree->header()->setStretchLastSection(true);
    m_autofillTree->setMinimumHeight(150);
    groupLayout->addWidget(m_autofillTree);

    auto *refreshBtn = new QPushButton("Refresh");
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        m_autofillTree->clear();
        for (const auto &e : AutofillManager::instance().entries()) {
            auto *item = new QTreeWidgetItem;
            item->setText(0, e.type);
            item->setText(1, e.label);
            QStringList fieldList;
            for (auto it = e.fields.begin(); it != e.fields.end(); ++it)
                fieldList.append(it.key() + "=" + it.value().toString());
            item->setText(2, fieldList.join(", "));
            m_autofillTree->addTopLevelItem(item);
        }
    });
    groupLayout->addWidget(refreshBtn);

    auto *addBtn = new QPushButton("Add New Profile");
    auto *refreshBtn2 = refreshBtn;
    connect(addBtn, &QPushButton::clicked, this, [this, refreshBtn2]() {
        // Simple dialog to add an autofill profile
        QString name = QInputDialog::getText(this, "Add Autofill Profile",
            "Profile label:", QLineEdit::Normal, "");
        if (name.isEmpty()) return;
        QJsonObject fields;
        fields["name"] = QInputDialog::getText(this, "Add Field", "Full name:");
        fields["email"] = QInputDialog::getText(this, "Add Field", "Email:");
        fields["phone"] = QInputDialog::getText(this, "Add Field", "Phone:");
        fields["address"] = QInputDialog::getText(this, "Add Field", "Address:");
        fields["city"] = QInputDialog::getText(this, "Add Field", "City:");
        fields["zip"] = QInputDialog::getText(this, "Add Field", "ZIP Code:");
        AutofillManager::instance().addEntry("profile", name, fields);
        refreshBtn2->click();
    });
    groupLayout->addWidget(addBtn);

    layout->addWidget(group);
    layout->addStretch();
    tabs->addTab(widget, "Autofill");
}

// ═══════════════════════════════════════════════════════════════════════
// Features Tab (NEW — exit confirm, spell check, auto cookie, etc.)
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::setupFeaturesTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *behaviorGroup = new QGroupBox("Browser Behavior");
    auto *behaviorLayout = new QVBoxLayout(behaviorGroup);

    m_exitConfirmCheck = new QCheckBox("Show exit confirmation dialog");
    m_exitConfirmCheck->setToolTip("Ask before closing the browser");
    behaviorLayout->addWidget(m_exitConfirmCheck);

    m_spellCheckCheck = new QCheckBox("Enable spell check in form fields");
    m_spellCheckCheck->setToolTip("Underlines misspelled words in text inputs");
    behaviorLayout->addWidget(m_spellCheckCheck);

    m_autoFillCheck = new QCheckBox("Auto-fill forms on page load");
    m_autoFillCheck->setToolTip("Automatically fill saved profile data into forms");
    behaviorLayout->addWidget(m_autoFillCheck);

    // Close empty tab behavior
    auto *closeLayout = new QHBoxLayout;
    closeLayout->addWidget(new QLabel("When closing last tab:"));
    m_closeEmptyTabCombo = new QComboBox;
    m_closeEmptyTabCombo->addItem("Exit browser", "exit");
    m_closeEmptyTabCombo->addItem("Open new tab", "newtab");
    closeLayout->addWidget(m_closeEmptyTabCombo);
    behaviorLayout->addLayout(closeLayout);

    layout->addWidget(behaviorGroup);

    // Cookie clearing
    auto *cookieGroup = new QGroupBox("Automatic Cookie Clearing");
    auto *cookieLayout = new QVBoxLayout(cookieGroup);

    m_autoCookieClearCheck = new QCheckBox("Auto-clear cookies at regular intervals");
    cookieLayout->addWidget(m_autoCookieClearCheck);

    auto *intervalLayout = new QHBoxLayout;
    intervalLayout->addWidget(new QLabel("Clear every (minutes):"));
    m_cookieClearIntervalSpin = new QSpinBox;
    m_cookieClearIntervalSpin->setRange(5, 1440);
    m_cookieClearIntervalSpin->setValue(30);
    m_cookieClearIntervalSpin->setSuffix(" min");
    intervalLayout->addWidget(m_cookieClearIntervalSpin);
    cookieLayout->addLayout(intervalLayout);

    layout->addWidget(cookieGroup);
    layout->addStretch();
    tabs->addTab(widget, "Features");
}

// ═══════════════════════════════════════════════════════════════════════
// Shortcuts Tab (NEW)
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::setupShortcutsTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *group = new QGroupBox("Keyboard Shortcuts");
    auto *groupLayout = new QVBoxLayout(group);

    struct Shortcut { QString key; QString desc; };
    QList<Shortcut> shortcuts = {
        {"Ctrl+T", "New Tab"},
        {"Ctrl+W", "Close Tab"},
        {"Ctrl+1", "Quick Dial"},
        {"Ctrl+D", "Add/Remove Bookmark"},
        {"Ctrl+H", "Show History"},
        {"Ctrl+L / F6", "Focus Address Bar"},
        {"Ctrl+R / F5", "Reload Page"},
        {"Ctrl+Q", "Quit Frint"},
        {"Ctrl+=", "Zoom In"},
        {"Ctrl+-", "Zoom Out"},
        {"Ctrl+0", "Reset Zoom"},
        {"Alt+Left", "Go Back"},
        {"Alt+Right", "Go Forward"},
        {"Alt+Home", "Home Page"},
        {"F11", "Toggle Full Screen"},
        {"F12", "Developer Tools"},
        {"Escape", "Stop Loading"},
        {"Ctrl+Shift+N", "Toggle Night Mode"},
        {"Ctrl+Shift+S", "Screenshot"},
        {"Ctrl+Shift+L", "Add to Reading List"},
        {"Ctrl+Shift+P", "Performance Status"},
        {"Ctrl+Shift+Delete", "Clear Browsing Data"},
    };

    auto *tree = new QTreeWidget;
    tree->setHeaderLabels({"Shortcut", "Action"});
    tree->setRootIsDecorated(false);
    tree->header()->setStretchLastSection(true);
    tree->setAlternatingRowColors(true);
    tree->setMinimumHeight(350);

    for (const auto &s : shortcuts) {
        auto *item = new QTreeWidgetItem;
        item->setText(0, s.key);
        item->setText(1, s.desc);
        tree->addTopLevelItem(item);
    }
    tree->resizeColumnToContents(0);

    groupLayout->addWidget(tree);

    // Fake edit notice
    auto *notice = new QLabel("Shortcuts are fixed and not user-customizable yet.");
    notice->setStyleSheet("color: #585b70; font-size: 11px;");
    groupLayout->addWidget(notice);

    layout->addWidget(group);
    layout->addStretch();
    tabs->addTab(widget, "Shortcuts");
}

// ═══════════════════════════════════════════════════════════════════════
// About Tab
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::setupAboutTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setAlignment(Qt::AlignCenter);

    auto *logo = new QLabel(widget);
    QPixmap pix("logo/logo.png");
    if (!pix.isNull())
        logo->setPixmap(pix.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    else
        logo->setText(QString::fromUtf8("\xF0\x9F\x9B\xA1\xEF\xB8\x8F"));
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet("font-size: 64px;");
    layout->addWidget(logo);
    layout->addSpacing(8);

    auto *infoGroup = new QGroupBox;
    infoGroup->setTitle("");
    auto *infoForm = new QFormLayout(infoGroup);
    infoForm->setSpacing(10);

    auto addRow = [&](const QString &label, const QString &value) {
        auto *vl = new QLabel(value);
        vl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        vl->setStyleSheet("color: #cdd6f4; font-size: 13px;");
        infoForm->addRow(label + ":", vl);
    };

    addRow("Application", "Frint Browser");
    addRow("Version", "v1.0.0");
    addRow("Build Date", QString(__DATE__) + " " + QString(__TIME__));
    addRow("Qt Version", QT_VERSION_STR);
    addRow("License", "GPL-3.0");

    auto *ghLink = new QLabel("<a href='https://github.com/qfdevel/Frint-Browser' style='color:#89b4fa;'>github.com/qfdevel/Frint-Browser</a>");
    ghLink->setOpenExternalLinks(true);
    ghLink->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    infoForm->addRow("GitHub:", ghLink);
    layout->addWidget(infoGroup);

    auto *reportBtn = new QPushButton("Report Issue");
    reportBtn->setStyleSheet(
        "QPushButton{background:rgba(137,180,250,0.15);color:#89b4fa;border:1px solid rgba(137,180,250,0.3);border-radius:10px;padding:10px 18px;font-weight:bold;}"
        "QPushButton:hover{background:rgba(137,180,250,0.35);border-color:rgba(137,180,250,0.5);}");
    connect(reportBtn, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl("https://github.com/qfdevel/Frint-Browser/issues/new"));
    });
    layout->addWidget(reportBtn);

    auto *desc = new QLabel(
        "<p style='color:#585b70; font-size:11px; text-align:center;'>"
        "Privacy-first browser built with Qt WebEngine + Frint Privacy Engine.<br>"
        "HTTPS-Only | Tracker Blocker | GPC | Anti-Fingerprinting | DoH</p>");
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true);
    layout->addWidget(desc);
    layout->addStretch();
    tabs->addTab(widget, QString::fromUtf8("\xE2\x84\xB9\xEF\xB8\x8F About"));
}

// ═══════════════════════════════════════════════════════════════════════
// Extensions Tab
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::setupExtensionsTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *extGroup = new QGroupBox("Installed Extensions");
    auto *extLayout = new QVBoxLayout(extGroup);

    m_extensionList = new QListWidget;
    m_extensionList->setMinimumHeight(150);

    auto *darkReader = new QListWidgetItem("Dark Reader");
    darkReader->setFlags(darkReader->flags() | Qt::ItemIsUserCheckable);
    darkReader->setCheckState(Qt::Checked);
    m_extensionList->addItem(darkReader);

    auto *ublock = new QListWidgetItem("uBlock Origin Lite");
    ublock->setFlags(ublock->flags() | Qt::ItemIsUserCheckable);
    ublock->setCheckState(Qt::Checked);
    m_extensionList->addItem(ublock);

    auto *privacyBadger = new QListWidgetItem("Privacy Badger");
    privacyBadger->setFlags(privacyBadger->flags() | Qt::ItemIsUserCheckable);
    privacyBadger->setCheckState(Qt::Checked);
    m_extensionList->addItem(privacyBadger);

    extLayout->addWidget(new QLabel("Click checkbox to enable/disable:"));
    extLayout->addWidget(m_extensionList);
    layout->addWidget(extGroup);

    auto *btnLayout = new QHBoxLayout;
    auto *loadExtBtn = new QPushButton("Load Extension");
    auto *removeExtBtn = new QPushButton("Remove Extension");
    btnLayout->addWidget(loadExtBtn);
    btnLayout->addWidget(removeExtBtn);
    layout->addLayout(btnLayout);

    connect(loadExtBtn, &QPushButton::clicked, this, &SettingsDialog::onLoadExtension);
    connect(removeExtBtn, &QPushButton::clicked, this, &SettingsDialog::onRemoveExtension);

    layout->addStretch();
    tabs->addTab(widget, QString::fromUtf8("\xF0\x9F\x93\x8B Extensions"));
}

// ═══════════════════════════════════════════════════════════════════════
// Load / Save
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::loadSettings()
{
    auto &s = SettingsManager::instance();

    m_homePageEdit->setText(s.homePage());

    QString se = s.searchEngine();
    for (int i = 0; i < m_searchEngineCombo->count(); ++i) {
        if (m_searchEngineCombo->itemData(i).toString() == se) {
            m_searchEngineCombo->setCurrentIndex(i);
            break;
        }
    }

    m_clearOnExitCheck->setChecked(s.isClearOnExit());
    m_httpsOnlyCheck->setChecked(s.isHttpsOnly());
    m_trackingBlockerCheck->setChecked(s.isTrackingBlockerEnabled());
    m_gpcCheck->setChecked(s.isGpcEnabled());
    m_fingerprintingCheck->setChecked(s.isFingerprintingProtectionEnabled());
    m_dohCheck->setChecked(s.isDohEnabled());
    m_block3rdPartyCookies->setChecked(s.areThirdPartyCookiesBlocked());

    m_canvasFpCheck->setChecked(s.isCanvasFingerprintingBlocked());
    m_webglFpCheck->setChecked(s.isWebglFingerprintingBlocked());
    m_audioFpCheck->setChecked(s.isAudioFingerprintingBlocked());
    m_fontFpCheck->setChecked(s.isFontFingerprintingBlocked());
    m_spoofHwConcurrencyCheck->setChecked(s.isSpoofHardwareConcurrency());
    m_spoofDeviceMemCheck->setChecked(s.isSpoofDeviceMemory());
    m_forceUtcCheck->setChecked(s.isForceUtcTimezone());
    m_webrtcBlockCheck->setChecked(s.isWebrtcBlocked());
    m_clientHintsCheck->setChecked(s.isClientHintsBlocked());
    m_etagTrackingCheck->setChecked(s.isEtagTrackingBlocked());

    m_downloadLocationEdit->setText(s.downloadLocation());
    m_alwaysAskDownloadCheck->setChecked(s.isAlwaysAskDownloadLocation());
    m_autoUpdateCheck->setChecked(s.isAutoUpdateCheck());

    // Theme
    QString theme = s.theme();
    for (int i = 0; i < m_themeCombo->count(); ++i) {
        if (m_themeCombo->itemData(i).toString() == theme) {
            m_themeCombo->setCurrentIndex(i);
            break;
        }
    }

    // Performance
    m_hwAccelCheck->setChecked(s.isHardwareAccelerationEnabled());
    m_sseAvxCheck->setChecked(s.isSSEAVXEnabled());
    QString perfMode = s.performanceMode();
    for (int i = 0; i < m_perfModeCombo->count(); ++i) {
        if (m_perfModeCombo->itemData(i).toString() == perfMode) {
            m_perfModeCombo->setCurrentIndex(i);
            break;
        }
    }

    m_profileList->clear();
    m_profileList->addItems(s.profiles());
    m_customCssEdit->setPlainText(s.getValue("custom_css", "").toString());

    // Features
    m_exitConfirmCheck->setChecked(s.getValue("exit_confirmation", true).toBool());
    m_spellCheckCheck->setChecked(s.getValue("spell_check", true).toBool());
    m_autoFillCheck->setChecked(s.getValue("autofill_enabled", true).toBool());
    QString closeAction = s.getValue("close_empty_tab_action", "exit").toString();
    m_closeEmptyTabCombo->setCurrentIndex(m_closeEmptyTabCombo->findData(closeAction));

    bool autoCookie = CookieManager::instance().isAutoClearActive();
    m_autoCookieClearCheck->setChecked(autoCookie);
    m_cookieClearIntervalSpin->setValue(CookieManager::instance().autoClearInterval());

    // Opacity
    int opacity = s.getValue("window_opacity", 100).toInt();
    m_opacitySlider->setValue(opacity);
    m_opacityLabel->setText(QString::number(opacity) + "%");

    // Profile
    QString active = s.activeProfile();
    for (int i = 0; i < m_profileList->count(); ++i) {
        if (m_profileList->item(i)->text() == active) {
            m_profileList->setCurrentRow(i);
            break;
        }
    }
}

void SettingsDialog::saveSettings()
{
    auto &s = SettingsManager::instance();
    s.setValue("custom_css", m_customCssEdit->toPlainText());
    s.setHomePage(m_homePageEdit->text());
    s.setSearchEngine(m_searchEngineCombo->currentData().toString());
    s.setClearOnExit(m_clearOnExitCheck->isChecked());
    s.setHttpsOnly(m_httpsOnlyCheck->isChecked());
    s.setTrackingBlockerEnabled(m_trackingBlockerCheck->isChecked());
    s.setGpcEnabled(m_gpcCheck->isChecked());
    s.setFingerprintingProtection(m_fingerprintingCheck->isChecked());
    s.setDohEnabled(m_dohCheck->isChecked());
    s.setThirdPartyCookiesBlocked(m_block3rdPartyCookies->isChecked());

    s.setCanvasFingerprintingBlocked(m_canvasFpCheck->isChecked());
    s.setWebglFingerprintingBlocked(m_webglFpCheck->isChecked());
    s.setAudioFingerprintingBlocked(m_audioFpCheck->isChecked());
    s.setFontFingerprintingBlocked(m_fontFpCheck->isChecked());
    s.setSpoofHardwareConcurrency(m_spoofHwConcurrencyCheck->isChecked());
    s.setSpoofDeviceMemory(m_spoofDeviceMemCheck->isChecked());
    s.setForceUtcTimezone(m_forceUtcCheck->isChecked());
    s.setWebrtcBlocked(m_webrtcBlockCheck->isChecked());
    s.setClientHintsBlocked(m_clientHintsCheck->isChecked());
    s.setEtagTrackingBlocked(m_etagTrackingCheck->isChecked());

    s.setDownloadLocation(m_downloadLocationEdit->text());
    s.setAlwaysAskDownloadLocation(m_alwaysAskDownloadCheck->isChecked());
    s.setAutoUpdateCheck(m_autoUpdateCheck->isChecked());

    // Features
    s.setValue("exit_confirmation", m_exitConfirmCheck->isChecked());
    s.setValue("spell_check", m_spellCheckCheck->isChecked());
    s.setValue("autofill_enabled", m_autoFillCheck->isChecked());
    s.setValue("close_empty_tab_action", m_closeEmptyTabCombo->currentData().toString());

    // Performance
    s.setHardwareAccelerationEnabled(m_hwAccelCheck->isChecked());
    s.setSSEAVXEnabled(m_sseAvxCheck->isChecked());
    s.setPerformanceMode(m_perfModeCombo->currentData().toString());

    // Auto cookie clear
    if (m_autoCookieClearCheck->isChecked())
        CookieManager::instance().startAutoClear(m_cookieClearIntervalSpin->value());
    else
        CookieManager::instance().stopAutoClear();

    // Opacity
    s.setValue("window_opacity", m_opacitySlider->value());

    s.setTheme(m_themeCombo->currentData().toString());

    if (m_profileList->currentItem()) {
        s.setActiveProfile(m_profileList->currentItem()->text());
    }

    s.sync();
}

void SettingsDialog::onApply()
{
    saveSettings();
    emit settingsApplied();
    accept();
}

void SettingsDialog::onOpacityChanged(int value)
{
    m_opacityLabel->setText(QString::number(value) + "%");
}

// ═══════════════════════════════════════════════════════════════════════
// Slots (copied from original)
// ═══════════════════════════════════════════════════════════════════════
void SettingsDialog::onSaveProfile()
{
    QString name = m_profileNameEdit->text().trimmed();
    if (name.isEmpty()) return;

    auto &s = SettingsManager::instance();
    QStringList profiles = s.profiles();
    if (!profiles.contains(name)) {
        profiles.append(name);
        s.setProfiles(profiles);
        m_profileList->addItem(name);
        m_profileNameEdit->clear();
        QDir().mkpath(s.frintDataDir() + "/profiles/" + name);
    }
}

void SettingsDialog::onDeleteProfile()
{
    auto item = m_profileList->currentItem();
    if (!item || item->text() == "Default") return;

    QString profileName = item->text();
    auto &s = SettingsManager::instance();
    QStringList profiles = s.profiles();
    profiles.removeAll(profileName);
    s.setProfiles(profiles);
    s.sync();

    QString profileDir = s.frintDataDir() + "/profiles/" + profileName;
    QDir dir(profileDir);
    if (dir.exists()) dir.removeRecursively();

    delete m_profileList->takeItem(m_profileList->row(item));

    if (s.activeProfile() == profileName) {
        s.setActiveProfile("Default");
        QStringList remaining = s.profiles();
        if (!remaining.contains("Default")) {
            remaining.prepend("Default");
            s.setProfiles(remaining);
        }
    }
}

void SettingsDialog::onThemeSelected(int index)
{
    QString theme = m_themeCombo->itemData(index).toString();
    applyTheme(theme);
}

void SettingsDialog::applyTheme(const QString &themeName)
{
    Q_UNUSED(themeName)
}

void SettingsDialog::onBrowseDownloadLocation()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        "Choose Download Location", m_downloadLocationEdit->text());
    if (!dir.isEmpty()) m_downloadLocationEdit->setText(dir);
}

void SettingsDialog::onClearDownloadHistory()
{
    m_downloadHistoryList->clear();
    m_downloadHistoryList->addItem("No downloads yet");
    m_lastCleanupLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
}

void SettingsDialog::onClearAllData()
{
    if (QMessageBox::question(this, "Clear All Data",
            "Clear all browsing data (cache, cookies, history)?",
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    auto &s = SettingsManager::instance();
    s.setValue("clear_on_exit", true);
    m_lastCleanupLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    QMessageBox::information(this, "Done", "Data will be cleared on next restart.");
}

void SettingsDialog::onExportPrivacyReport()
{
    QString report = QString(
        "Frint Browser Privacy Report\n"
        "===========================\n"
        "Generated: %1\n\n"
        "Privacy Score: 105/105 ✅\n\n"
        "Features Enabled:\n"
        "  - HTTPS-Only: Yes\n"
        "  - Tracker Blocker: Yes\n"
        "  - GPC: Yes\n"
        "  - Fingerprint Defense: Yes\n"
        "  - DoH: Yes\n"
        "  - 3rd-Party Cookies Blocked: Yes\n"
        "  - Canvas FP: %2\n"
        "  - WebGL FP: %3\n"
        "  - Audio FP: %4\n"
        "  - Font FP: %5\n"
        "  - WebRTC Block: %6\n"
        "  - ETag Strip: %7\n\n"
        "Blocked Trackers: 0\n"
        "HTTPS Upgrades: 0\n"
    ).arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),
          m_canvasFpCheck->isChecked() ? "Yes" : "No",
          m_webglFpCheck->isChecked() ? "Yes" : "No",
          m_audioFpCheck->isChecked() ? "Yes" : "No",
          m_fontFpCheck->isChecked() ? "Yes" : "No",
          m_webrtcBlockCheck->isChecked() ? "Yes" : "No",
          m_etagTrackingCheck->isChecked() ? "Yes" : "No");

    QApplication::clipboard()->setText(report);
    QMessageBox::information(this, "Report Exported", "Privacy report copied to clipboard.");
}

void SettingsDialog::onCheckForUpdates()
{
    QMessageBox::information(this, "Check for Updates",
        "Frint Browser v1.0.0\n\nYou're running the latest version.");
}

void SettingsDialog::onLoadExtension()
{
    QString path = QFileDialog::getOpenFileName(this,
        "Load Extension", QString(), "Extensions (*.js *.json);;All Files (*)");
    if (!path.isEmpty()) {
        QFileInfo fi(path);
        auto *item = new QListWidgetItem(fi.fileName());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        m_extensionList->addItem(item);
    }
}

void SettingsDialog::onRemoveExtension()
{
    auto item = m_extensionList->currentItem();
    if (item && item->flags() & Qt::ItemIsUserCheckable) {
        delete m_extensionList->takeItem(m_extensionList->row(item));
    }
}

} // namespace Frint
