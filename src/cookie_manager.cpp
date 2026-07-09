#include "cookie_manager.h"
#include "settings_manager.h"
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QDebug>
#include <QWebEngineSettings>

namespace Frint {

CookieManager &CookieManager::instance()
{
    static CookieManager s_instance;
    return s_instance;
}

CookieManager::CookieManager()
    : m_timer(new QTimer(this))
    , m_intervalMinutes(30)
{
    connect(m_timer, &QTimer::timeout, this, &CookieManager::clearCookies);

    // Auto-start based on saved preference
    int interval = SettingsManager::instance().getValue("auto_cookie_clear_interval", 0).toInt();
    if (interval > 0)
        startAutoClear(interval);
}

void CookieManager::startAutoClear(int intervalMinutes)
{
    m_intervalMinutes = intervalMinutes;
    if (m_intervalMinutes < 1) m_intervalMinutes = 30;
    m_timer->start(m_intervalMinutes * 60 * 1000);
    scheduleNextClear();
    SettingsManager::instance().setValue("auto_cookie_clear_interval", m_intervalMinutes);
    emit autoClearToggled(true);
    qDebug() << "[Frint CookieManager] Auto-clear every" << m_intervalMinutes << "minutes";
}

void CookieManager::stopAutoClear()
{
    m_timer->stop();
    m_nextClear = QDateTime();
    SettingsManager::instance().setValue("auto_cookie_clear_interval", 0);
    emit autoClearToggled(false);
}

bool CookieManager::isAutoClearActive() const { return m_timer->isActive(); }
int CookieManager::autoClearInterval() const { return m_intervalMinutes; }

void CookieManager::clearCookies()
{
    // Clear cookies from all profiles
    // Default profile
    auto *defaultProfile = QWebEngineProfile::defaultProfile();
    if (defaultProfile) {
        defaultProfile->cookieStore()->deleteAllCookies();
        defaultProfile->clearHttpCache();
    }

    qDebug() << "[Frint CookieManager] Auto-cleared all cookies" << QDateTime::currentDateTime().toString();
    scheduleNextClear();
    emit cookiesCleared();
}

void CookieManager::scheduleNextClear()
{
    m_nextClear = QDateTime::currentDateTime().addSecs(m_intervalMinutes * 60);
}

QDateTime CookieManager::nextClearTime() const { return m_nextClear; }

} // namespace Frint
