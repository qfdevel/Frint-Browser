#ifndef FRINT_REFERRER_POLICY_H
#define FRINT_REFERRER_POLICY_H

#include <QUrl>
#include <QString>

namespace Frint {

class ReferrerPolicy {
public:
    enum class Policy {
        StrictOriginWhenCrossOrigin,
        NoReferrer,
        SameOrigin,
        OriginWhenCrossOrigin,
        StrictOrigin,
        Origin,
        NoReferrerWhenDowngrade
    };

    static ReferrerPolicy &instance();

    void setPolicy(Policy policy);
    Policy policy() const;
    QString policyName() const;

    // Get the referrer for a navigation from sourceUrl to targetUrl
    QString getReferrer(const QUrl &targetUrl, const QUrl &sourceUrl) const;

private:
    ReferrerPolicy();
    Policy m_policy = Policy::StrictOriginWhenCrossOrigin;
};

} // namespace Frint

#endif // FRINT_REFERRER_POLICY_H
