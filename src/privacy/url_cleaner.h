#ifndef FRINT_URL_CLEANER_H
#define FRINT_URL_CLEANER_H

#include <QUrl>
#include <QString>
#include <QStringList>

namespace Frint {

class UrlCleaner {
public:
    // Remove all known tracking query parameters from the URL
    static QUrl cleanTrackingParams(const QUrl &url);

    // Check if a query parameter is a known tracking param
    static bool isTrackingParam(const QString &paramName);

    // Get the full list of known tracking parameters
    static QStringList knownTrackingParams();

private:
    UrlCleaner() = delete;

    static QStringList s_trackingParams;
    static QStringList s_utmVariants;
    static QStringList s_adPlatformParams;
};

} // namespace Frint

#endif // FRINT_URL_CLEANER_H
