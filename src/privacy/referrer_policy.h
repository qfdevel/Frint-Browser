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
        OriginWhenCrossOrigin
    };

    static ReferrerPolicy &instance();

    void setPolicy(Policy policy);
    Policy policy() const;

    QString getReferrer(const QUrl &targetUrl, const QUrl &sourceUrl) const;

private:
    ReferrerPolicy();
    Policy m_policy = Policy::StrictOriginWhenCrossOrigin;
};

} // namespace Frint

#endif // FRINT_REFERRER_POLICY_H
