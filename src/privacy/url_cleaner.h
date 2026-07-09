#ifndef FRINT_URL_CLEANER_H
#define FRINT_URL_CLEANER_H

#include <QUrl>
#include <QString>
#include <QStringList>

namespace Frint {

class UrlCleaner {
public:
    static QUrl cleanTrackingParams(const QUrl &url);

private:
    static const QStringList s_trackingParams;
};

} // namespace Frint

#endif // FRINT_URL_CLEANER_H
