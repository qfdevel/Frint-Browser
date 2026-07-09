#include "setup_wizard.h"
#include "settings_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QDebug>
#include <QStyle>
#include <QFont>
#include <QTextBrowser>
#include <QDateTime>

namespace Frint {

SetupWizard::SetupWizard(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle("Welcome to Frint Browser");
    setMinimumSize(640, 520);
    resize(700, 560);
    setWizardStyle(QWizard::ModernStyle);

    addPage(createWelcomePage());
    addPage(createLanguagePage());
    addPage(createThemePage());
    addPage(createProfilePage());
    addPage(createTutorialPage());
    addPage(createFinishPage());

    setStyleSheet(
        "QWizard { background: #1e1e2e; }"
        "QWizardPage { background: transparent; }"
        "QLabel { color: #cdd6f4; }"
        "QLineEdit { background: rgba(49, 50, 68, 0.4); color: #cdd6f4; border: 2px solid rgba(69, 71, 90, 0.5);"
        "  border-radius: 10px; padding: 10px 16px; font-size: 14px; }"
        "QLineEdit:focus { border-color: rgba(137, 180, 250, 0.6); background: rgba(49, 50, 68, 0.6); }"
        "QComboBox { background: rgba(49, 50, 68, 0.4); color: #cdd6f4; border: 2px solid rgba(69, 71, 90, 0.5);"
        "  border-radius: 10px; padding: 10px 16px; font-size: 14px; }"
        "QComboBox:focus { border-color: rgba(137, 180, 250, 0.6); }"
        "QComboBox QAbstractItemView { background: rgba(49, 50, 68, 0.85); color: #cdd6f4;"
        "  selection-background-color: rgba(69, 71, 90, 0.6); border-radius: 10px; border: 1px solid rgba(255,255,255,0.06); }"
        "QCheckBox { color: #cdd6f4; spacing: 10px; font-size: 14px; }"
        "QCheckBox::indicator { width: 20px; height: 20px; border-radius: 5px;"
        "  border: 2px solid rgba(88, 91, 112, 0.6); background: rgba(49, 50, 68, 0.3); }"
        "QCheckBox::indicator:checked { background: rgba(166, 227, 161, 0.7); border-color: rgba(166, 227, 161, 0.8); }"
        "QCheckBox::indicator:hover { border-color: rgba(137, 180, 250, 0.5); }"
        "QPushButton { background: rgba(69, 71, 90, 0.35); color: #cdd6f4; border: 1px solid rgba(255,255,255,0.06);"
        "  border-radius: 10px; padding: 10px 24px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: rgba(69, 71, 90, 0.6); border-color: rgba(255,255,255,0.12); }"
        "QPushButton:pressed { background: rgba(69, 71, 90, 0.8); }"
        "QTextBrowser { background: rgba(30, 30, 46, 0.6); color: #bac2de; border: 1px solid rgba(255,255,255,0.06);"
        "  border-radius: 10px; padding: 12px; font-size: 13px; }"
        "QWizard QTitleLabel { color: #cdd6f4; font-size: 20px; font-weight: bold; }"
    );
}

SetupWizard::~SetupWizard() = default;

QWizardPage *SetupWizard::createWelcomePage()
{
    auto *page = new QWizardPage;
    page->setTitle("Welcome to Frint Browser");
    page->setSubTitle("Privacy-first. Telemetry-free. Yours.");
    auto *l = new QVBoxLayout(page);
    l->setAlignment(Qt::AlignCenter);
    auto *logo = new QLabel(page);
    QPixmap pix("logo/logo.png");
    if (!pix.isNull())
        logo->setPixmap(pix.scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    else
        logo->setText(QString::fromUtf8("\xF0\x9F\x9B\xA1\xEF\xB8\x8F"));
    logo->setStyleSheet("font-size: 80px;");
    logo->setAlignment(Qt::AlignCenter);
    l->addWidget(logo);
    l->addSpacing(16);
    auto *t = new QLabel("<h2 style='color:#cba6f7;'>Frint Browser</h2>"
        "<p style='color:#bac2de;'>Thanks for choosing Frint.<br>"
        "Let's set up your browser in a few steps.</p>", page);
    t->setWordWrap(true);
    t->setAlignment(Qt::AlignCenter);
    l->addWidget(t);
    l->addStretch();
    return page;
}

QWizardPage *SetupWizard::createLanguagePage()
{
    auto *page = new QWizardPage;
    page->setTitle("Choose Your Language");
    page->setSubTitle("Select interface language");
    auto *l = new QVBoxLayout(page);
    auto *f = new QFormLayout;
    m_languageCombo = new QComboBox(page);
    for (auto &p : {"en_US","en_GB","tr_TR","de_DE","fr_FR","es_ES","it_IT","pt_BR","ru_RU","ja_JP","zh_CN"})
        m_languageCombo->addItem(p, p);
    m_languageCombo->setCurrentIndex(0);
    f->addRow("Language:", m_languageCombo);
    l->addLayout(f);
    l->addStretch();
    return page;
}

QWizardPage *SetupWizard::createThemePage()
{
    auto *page = new QWizardPage;
    page->setTitle("Select Your Theme");
    page->setSubTitle("Choose how Frint looks");
    auto *l = new QVBoxLayout(page);
    m_themeCombo = new QComboBox(page);
    m_themeCombo->addItem("Catppuccin Mocha (Dark)", "catppuccin-mocha");
    m_themeCombo->addItem("Nord Dark", "nord-dark");
    m_themeCombo->addItem("Gruvbox Material", "gruvbox-material");
    m_themeCombo->setCurrentIndex(0);
    auto *preview = new QLabel(page);
    preview->setMinimumHeight(100);
    preview->setStyleSheet("background: #313244; border-radius: 10px; border: 2px solid #45475a;");
    preview->setText("  A preview will appear here");
    l->addWidget(m_themeCombo);
    l->addWidget(preview);
    l->addStretch();
    return page;
}

QWizardPage *SetupWizard::createProfilePage()
{
    auto *page = new QWizardPage;
    page->setTitle("Create Your Profile");
    page->setSubTitle("Keep your browsing data separate");
    auto *l = new QVBoxLayout(page);
    auto *f = new QFormLayout;
    m_profileNameEdit = new QLineEdit(page);
    m_profileNameEdit->setText("Default");
    m_profileNameEdit->selectAll();
    f->addRow("Profile Name:", m_profileNameEdit);
    m_setAsDefaultCheck = new QCheckBox("Set as default profile");
    m_setAsDefaultCheck->setChecked(true);
    f->addRow("", m_setAsDefaultCheck);
    l->addLayout(f);
    auto *info = new QLabel(
        "<p style='color:#585b70; font-size:11px;'>Stored at: "
        "<code>Documents/Frint/profiles/{name}/cfgs/frint.conf</code></p>");
    info->setWordWrap(true);
    l->addWidget(info);
    l->addStretch();
    return page;
}

QWizardPage *SetupWizard::createTutorialPage()
{
    auto *page = new QWizardPage;
    page->setTitle("Quick Tutorial");
    page->setSubTitle("Learn about Frint's privacy features");
    auto *l = new QVBoxLayout(page);
    m_tutorialCheck = new QCheckBox("Show interactive tutorial on first launch");
    m_tutorialCheck->setChecked(true);
    auto *tb = new QTextBrowser(page);
    tb->setHtml(
        "<h3 style='color:#cba6f7;'>Privacy Features</h3>"
        "<ul><li><b>Tracker Blocker</b> — 150+ domains blocked</li>"
        "<li><b>HTTPS-Only</b> — Auto HTTP→HTTPS upgrade</li>"
        "<li><b>GPC Signal</b> — Do Not Sell on every request</li>"
        "<li><b>Fingerprinting Defense</b> — Canvas, fonts, UA spoofing</li>"
        "<li><b>DNS-over-HTTPS</b> — Encrypted DNS via Cloudflare</li></ul>"
        "<h3 style='color:#89b4fa;'>Shortcuts</h3>"
        "<table><tr><td>Ctrl+T</td><td>New Tab</td></tr>"
        "<tr><td>Ctrl+W</td><td>Close Tab</td></tr>"
        "<tr><td>Ctrl+L</td><td>Focus Address Bar</td></tr>"
        "<tr><td>F12</td><td>DevTools</td></tr></table>"
    );
    l->addWidget(m_tutorialCheck);
    l->addWidget(tb);
    return page;
}

QWizardPage *SetupWizard::createFinishPage()
{
    auto *page = new QWizardPage;
    page->setTitle("All Set!");
    page->setSubTitle("Your browser is ready");
    auto *l = new QVBoxLayout(page);
    l->setAlignment(Qt::AlignCenter);
    auto *icon = new QLabel(page);
    icon->setText(QString::fromUtf8("\xE2\x9C\x85"));
    icon->setStyleSheet("font-size: 64px;");
    icon->setAlignment(Qt::AlignCenter);
    l->addWidget(icon);
    l->addSpacing(12);
    m_summaryLabel = new QLabel(page);
    m_summaryLabel->setWordWrap(true);
    m_summaryLabel->setAlignment(Qt::AlignCenter);
    l->addWidget(m_summaryLabel);
    l->addStretch();

    int lastPage = pageIds().size() - 1;
    connect(this, &QWizard::currentIdChanged, this, [this, lastPage]() {
        if (m_summaryLabel && currentId() == lastPage) {
            QString t;
            switch (m_themeCombo->currentIndex()) {
            case 0: t = "Catppuccin Mocha (Dark)"; break;
            case 1: t = "Nord Dark"; break;
            case 2: t = "Gruvbox Material"; break;
            default: t = "Catppuccin Mocha (Dark)";
            }
            m_summaryLabel->setText(
                QString("<h3 style='color:#a6e3a1;'>Summary</h3>"
                        "<p><b>Profile:</b> %1<br><b>Theme:</b> %2<br>"
                        "<b>Tutorial:</b> %3</p>")
                .arg(m_profileNameEdit->text().trimmed().isEmpty()
                     ? "Default" : m_profileNameEdit->text().trimmed())
                .arg(t)
                .arg(m_tutorialCheck->isChecked() ? "Yes" : "No"));
        }
    });

    return page;
}

QString SetupWizard::selectedLanguage() const { return m_languageCombo->currentData().toString(); }
QString SetupWizard::selectedTheme() const { return m_themeCombo->currentData().toString(); }
QString SetupWizard::profileName() const { auto n = m_profileNameEdit->text().trimmed(); return n.isEmpty() ? "Default" : n; }
bool SetupWizard::showTutorial() const { return m_tutorialCheck->isChecked(); }

QString SetupWizard::configPath() const
{
    auto base = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                + "/Frint/profiles/" + profileName() + "/cfgs";
    QDir().mkpath(base);
    return base + "/frint.conf";
}

} // namespace Frint
