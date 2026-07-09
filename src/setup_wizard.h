#ifndef FRINT_SETUP_WIZARD_H
#define FRINT_SETUP_WIZARD_H

#include <QWizard>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>

namespace Frint {

class SetupWizard : public QWizard {
    Q_OBJECT

public:
    explicit SetupWizard(QWidget *parent = nullptr);
    ~SetupWizard() override;

    QString selectedLanguage() const;
    QString selectedTheme() const;
    QString profileName() const;
    bool showTutorial() const;
    QString configPath() const;

signals:
    void setupComplete(const QString &profileName, const QString &configPath);

private:
    QWizardPage *createWelcomePage();
    QWizardPage *createLanguagePage();
    QWizardPage *createThemePage();
    QWizardPage *createProfilePage();
    QWizardPage *createTutorialPage();
    QWizardPage *createFinishPage();

    void applyCurrentTheme();

    QComboBox *m_languageCombo;
    QComboBox *m_themeCombo;
    QLineEdit *m_profileNameEdit;
    QCheckBox *m_tutorialCheck;
    QCheckBox *m_setAsDefaultCheck;
    QLabel *m_logoLabel;
    QLabel *m_summaryLabel;

    static constexpr const char *s_defaultProfile = "Default";
};

} // namespace Frint

#endif
