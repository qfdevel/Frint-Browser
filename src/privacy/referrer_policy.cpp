#include "privacy/referrer_policy.h"

namespace Frint {

ReferrerPolicy &ReferrerPolicy::instance()
{
    static ReferrerPolicy s_instance;
    return s_instance;
}

ReferrerPolicy::ReferrerPolicy()
    : m_policy(Policy::StrictOriginWhenCrossOrigin)
{
}

void ReferrerPolicy::setPolicy(Policy policy)
{
    m_policy = policy;
}

ReferrerPolicy::Policy ReferrerPolicy::policy() const
{
    return m_policy;
}

QString ReferrerPolicy::getReferrer(const QUrl &targetUrl, const QUrl &sourceUrl) const
{
    if (!sourceUrl.isValid()) {
        return QString();
    }

    switch (m_policy) {
    case Policy::NoReferrer:
        return QString();

    case Policy::SameOrigin:
        if (targetUrl.scheme() == sourceUrl.scheme() &&
            targetUrl.host() == sourceUrl.host() &&
            targetUrl.port() == sourceUrl.port()) {
            return sourceUrl.toString();
        }
        return QString();

    case Policy::StrictOriginWhenCrossOrigin:
        if (targetUrl.scheme() == sourceUrl.scheme() &&
            targetUrl.host() == sourceUrl.host() &&
            targetUrl.port() == sourceUrl.port()) {
            return sourceUrl.toString();
        }
        // Send only origin for cross-origin, but only if same scheme
        if (targetUrl.scheme() == sourceUrl.scheme()) {
            return sourceUrl.scheme() + "://" + sourceUrl.host();
        }
        // Downgrade: HTTPS -> HTTP → no referrer
        if (sourceUrl.scheme() == "https" && targetUrl.scheme() != "https") {
            return QString();
        }
        return sourceUrl.scheme() + "://" + sourceUrl.host();

    case Policy::OriginWhenCrossOrigin:
        if (targetUrl.scheme() == sourceUrl.scheme() &&
            targetUrl.host() == sourceUrl.host() &&
            targetUrl.port() == sourceUrl.port()) {
            return sourceUrl.toString();
        }
        return sourceUrl.scheme() + "://" + sourceUrl.host();
    }

    return QString();
}

} // namespace Frint
