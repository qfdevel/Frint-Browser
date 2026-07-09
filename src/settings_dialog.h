#ifndef FRINT_SETTINGS_DIALOG_H
#define FRINT_SETTINGS_DIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QTreeWidget>

namespace Frint {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;

signals:
    void settingsApplied();

private slots:
    void onApply();
    void onSaveProfile();
    void onDeleteProfile();
    void onThemeSelected(int index);
    void onBrowseDownloadLocation();
    void onClearDownloadHistory();
    void onClearAllData();
    void onExportPrivacyReport();
    void onCheckForUpdates();
    void onLoadExtension();
    void onRemoveExtension();
    void onOpacityChanged(int value);

private:
    void setupGeneralTab(QTabWidget *tabs);
    void setupPrivacyTab(QTabWidget *tabs);
    void setupAdvancedPrivacyTab(QTabWidget *tabs);
    void setupPrivacyDashboardTab(QTabWidget *tabs);
    void setupAppearanceTab(QTabWidget *tabs);
    void setupProfilesTab(QTabWidget *tabs);
    void setupDownloadsTab(QTabWidget *tabs);
    void setupAboutTab(QTabWidget *tabs);
    void setupExtensionsTab(QTabWidget *tabs);
    // NEW TABS
    void setupBookmarksTab(QTabWidget *tabs);
    void setupShortcutsTab(QTabWidget *tabs);
    void setupAutofillTab(QTabWidget *tabs);
    void setupFeaturesTab(QTabWidget *tabs);
    void setupPerformanceTab(QTabWidget *tabs);

    void loadSettings();
    void saveSettings();
    void applyTheme(const QString &themeName);

    // General
    QLineEdit *m_homePageEdit;
    QComboBox *m_searchEngineCombo;
    QCheckBox *m_clearOnExitCheck;

    // Privacy
    QCheckBox *m_httpsOnlyCheck;
    QCheckBox *m_trackingBlockerCheck;
    QCheckBox *m_gpcCheck;
    QCheckBox *m_fingerprintingCheck;
    QCheckBox *m_dohCheck;
    QCheckBox *m_block3rdPartyCookies;

    // Advanced Privacy
    QCheckBox *m_antiFingerprintCheck;
    QCheckBox *m_canvasFpCheck;
    QCheckBox *m_webglFpCheck;
    QCheckBox *m_audioFpCheck;
    QCheckBox *m_fontFpCheck;
    QCheckBox *m_spoofHwConcurrencyCheck;
    QCheckBox *m_spoofDeviceMemCheck;
    QCheckBox *m_forceUtcCheck;
    QCheckBox *m_webrtcBlockCheck;
    QCheckBox *m_clientHintsCheck;
    QCheckBox *m_etagTrackingCheck;

    // Privacy Dashboard
    QLabel *m_privacyScoreLabel;
    QLabel *m_blockedTrackersLabel;
    QLabel *m_blockedCookiesLabel;
    QLabel *m_httpsUpgradesLabel;
    QLabel *m_lastCleanupLabel;

    // Appearance
    QComboBox *m_themeCombo;
    QSpinBox *m_fontSizeSpin;
    QPlainTextEdit *m_customCssEdit;
    QSlider *m_opacitySlider;
    QLabel *m_opacityLabel;

    // Profiles
    QListWidget *m_profileList;
    QLineEdit *m_profileNameEdit;
    QPushButton *m_saveProfileBtn;
    QPushButton *m_deleteProfileBtn;

    // Downloads & Updates
    QLineEdit *m_downloadLocationEdit;
    QCheckBox *m_alwaysAskDownloadCheck;
    QListWidget *m_downloadHistoryList;
    QCheckBox *m_autoUpdateCheck;

    // Extensions
    QListWidget *m_extensionList;

    // Features
    QCheckBox *m_exitConfirmCheck;
    QCheckBox *m_spellCheckCheck;
    QCheckBox *m_autoCookieClearCheck;
    QSpinBox *m_cookieClearIntervalSpin;
    QComboBox *m_closeEmptyTabCombo;
    QCheckBox *m_autoFillCheck;
    QCheckBox *m_nightModeCheck;

    // Bookmarks Tab
    QTreeWidget *m_bookmarkTree;
    QLineEdit *m_bookmarkFolderEdit;

    // Autofill Tab
    QTreeWidget *m_autofillTree;

    // Performance Tab
    QCheckBox *m_hwAccelCheck;
    QCheckBox *m_sseAvxCheck;
    QComboBox *m_perfModeCombo;
    QLabel *m_cpuFeaturesLabel;
    QLabel *m_gpuInfoLabel;
    QLabel *m_perfStatusLabel;
};

} // namespace Frint

#endif
