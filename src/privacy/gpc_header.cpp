#include "privacy/gpc_header.h"

namespace Frint {

GpcHeader &GpcHeader::instance()
{
    static GpcHeader s_instance;
    return s_instance;
}

GpcHeader::GpcHeader() = default;

void GpcHeader::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool GpcHeader::isEnabled() const
{
    return m_enabled;
}

QByteArray GpcHeader::headerName()
{
    return QByteArrayLiteral("Sec-GPC");
}

QByteArray GpcHeader::headerValue()
{
    return QByteArrayLiteral("1");
}

QPair<QByteArray, QByteArray> GpcHeader::header() const
{
    if (!m_enabled) {
        return {};
    }
    return { headerName(), headerValue() };
}

} // namespace Frint
