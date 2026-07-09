#include "privacy/referrer_policy.h"
#include <QDebug>

namespace Frint {

ReferrerPolicy &ReferrerPolicy::instance()
{
    static ReferrerPolicy s_instance;
    return s_instance;
}

ReferrerPolicy::ReferrerPolicy() = default;

void ReferrerPolicy::setPolicy(Policy policy)
{
    m_policy = policy;
    qDebug() << "[Frint ReferrerPolicy] Set policy to" << policyName();
}

ReferrerPolicy::Policy ReferrerPolicy::policy() const
{
    return m_policy;
}

QString ReferrerPolicy::policyName() const
{
    switch (m_policy) {
    case Policy::NoReferrer:
        return "no-referrer";
    case Policy::SameOrigin:
        return "same-origin";
    case Policy::OriginWhenCrossOrigin:
        return "origin-when-cross-origin";
    case Policy::StrictOriginWhenCrossOrigin:
        return "strict-origin-when-cross-origin";
    case Policy::StrictOrigin:
        return "strict-origin";
    case Policy::Origin:
        return "origin";
    case Policy::NoReferrerWhenDowngrade:
        return "no-referrer-when-downgrade";
    }
    return "unknown";
}

QString ReferrerPolicy::getReferrer(const QUrl &targetUrl, const QUrl &sourceUrl) const
{
    if (!sourceUrl.isValid() || !targetUrl.isValid()) {
        return QString();
    }

    bool sameOrigin = (targetUrl.scheme() == sourceUrl.scheme() &&
                       targetUrl.host() == sourceUrl.host() &&
                       targetUrl.port(sourceUrl.scheme() == "https" ? 443 : 80) ==
                           sourceUrl.port(sourceUrl.scheme() == "https" ? 443 : 80));

    bool downgrade = (sourceUrl.scheme() == "https" && targetUrl.scheme() != "https");

    // Helper: extract origin
    auto origin = [](const QUrl &url) -> QString {
        QString port = url.port() > 0 ? ":" + QString::number(url.port()) : "";
        return url.scheme() + "://" + url.host() + port;
    };

    switch (m_policy) {
    case Policy::NoReferrer:
        return QString();

    case Policy::SameOrigin:
        if (sameOrigin) return sourceUrl.toString();
        return QString();

    case Policy::OriginWhenCrossOrigin:
        if (sameOrigin) return sourceUrl.toString();
        return origin(sourceUrl);

    case Policy::StrictOriginWhenCrossOrigin:
        if (downgrade) return QString();
        if (sameOrigin) return sourceUrl.toString();
        return origin(sourceUrl);

    case Policy::StrictOrigin:
        if (downgrade) return QString();
        return origin(sourceUrl);

    case Policy::Origin:
        return origin(sourceUrl);

    case Policy::NoReferrerWhenDowngrade:
        if (downgrade) return QString();
        return sourceUrl.toString();
    }

    return QString();
}

} // namespace Frint
