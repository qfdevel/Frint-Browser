#ifndef FRINT_GPC_HEADER_H
#define FRINT_GPC_HEADER_H

#include <QByteArray>
#include <QPair>

namespace Frint {

class GpcHeader {
public:
    static GpcHeader &instance();

    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Returns the "Sec-GPC: 1" header
    QPair<QByteArray, QByteArray> header() const;

    // Returns HTTP header name
    static QByteArray headerName();

    // Returns HTTP header value
    static QByteArray headerValue();

private:
    GpcHeader();
    bool m_enabled = true;
};

} // namespace Frint

#endif // FRINT_GPC_HEADER_H
