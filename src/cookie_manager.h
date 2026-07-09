#ifndef FRINT_COOKIE_MANAGER_H
#define FRINT_COOKIE_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>

namespace Frint {

class CookieManager : public QObject {
    Q_OBJECT

public:
    static CookieManager &instance();

    void startAutoClear(int intervalMinutes = 30);
    void stopAutoClear();
    bool isAutoClearActive() const;
    int autoClearInterval() const;

    void clearCookies();
    void scheduleNextClear();

    QDateTime nextClearTime() const;

signals:
    void cookiesCleared();
    void autoClearToggled(bool active);

private:
    CookieManager();
    ~CookieManager() = default;
    CookieManager(const CookieManager &) = delete;
    CookieManager &operator=(const CookieManager &) = delete;

    QTimer *m_timer;
    int m_intervalMinutes;
    QDateTime m_nextClear;
};

} // namespace Frint

#endif // FRINT_COOKIE_MANAGER_H
