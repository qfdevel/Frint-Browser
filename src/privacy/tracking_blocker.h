#ifndef FRINT_TRACKING_BLOCKER_H
#define FRINT_TRACKING_BLOCKER_H

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QSet>

namespace Frint {

class TrackingBlocker {
public:
    static TrackingBlocker &instance();

    // Check if a request URL is a known tracker
    bool isTracker(const QUrl &requestUrl, const QUrl &pageUrl) const;

    // Check if a cookie domain should be blocked
    bool isTrackingCookie(const QString &cookieDomain, const QUrl &pageUrl) const;

    // Get all blocked domains
    QStringList blockedDomains() const;

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void addCustomDomain(const QString &domain);
    void removeCustomDomain(const QString &domain);

private:
    TrackingBlocker();
    bool m_enabled = true;

    static QStringList s_knownTrackers;
    QStringList m_customDomains;
};

} // namespace Frint

#endif // FRINT_TRACKING_BLOCKER_H
