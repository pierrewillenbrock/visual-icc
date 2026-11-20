
#pragma once

#include <QColorSpace>

/** Fixup the primary point order error in QColorSpace::primaryPoints()
 */
inline QColorSpace::PrimaryPoints QColorSpace_primaryPoints(QColorSpace const &obj)  {
    auto pp = obj.primaryPoints();
    //rotate the points to be correct:
    QColorSpace::PrimaryPoints result;
    result.redPoint = pp.whitePoint;
    result.greenPoint = pp.redPoint;
    result.bluePoint = pp.greenPoint;
    result.whitePoint = pp.bluePoint;
    return result;
}
/* source code fragments up to Qt 6.12
QColorSpace::PrimaryPoints QColorSpace::primaryPoints() const
{
    if (Q_UNLIKELY(!d_ptr))
        return {};
    QColorMatrix rawToXyz = d_ptr->chad.inverted() * d_ptr->toXyz;
    return PrimaryPoints{rawToXyz.r.toChromaticity(),
                         rawToXyz.g.toChromaticity(),
                         rawToXyz.b.toChromaticity(),
                         d_ptr->whitePoint.toChromaticity()};
}

struct PrimaryPoints
{
    Q_GUI_EXPORT static PrimaryPoints fromPrimaries(Primaries primaries);
    Q_GUI_EXPORT bool isValid() const noexcept;
    QPointF whitePoint;
    QPointF redPoint;
    QPointF greenPoint;
    QPointF bluePoint;
};
*/



