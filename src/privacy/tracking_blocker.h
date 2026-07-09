#ifndef FRINT_TRACKING_BLOCKER_H
#define FRINT_TRACKING_BLOCKER_H

#include <QString>
#include <QStringList>
#include <QUrl>

namespace Frint {

class TrackingBlocker {
public:
    static TrackingBlocker &instance();

    bool isTracker(const QUrl &requestUrl, const QUrl &pageUrl) const;
    QStringList blockedDomains() const;
    void addCustomDomain(const QString &domain);
    void removeCustomDomain(const QString &domain);

    void setEnabled(bool enabled);
    bool isEnabled() const;

private:
    TrackingBlocker();
    bool m_enabled = true;

    static QStringList s_knownTrackers;
    QStringList m_customDomains;
};

} // namespace Frint

#endif // FRINT_TRACKING_BLOCKER_H
