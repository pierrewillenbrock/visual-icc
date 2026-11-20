

#include <QColorSpace>
#include <QRgbaFloat>
#include "fix-colorspace.h"


void extractColorSpaceTransferTables(QColorSpace const &space,
                                     QList<uint16_t> &redTransferFunctionTable,
                                     QList<uint16_t> &greenTransferFunctionTable,
                                     QList<uint16_t> &blueTransferFunctionTable) {
    //QColorSpace::PrimaryPoints pp;
    //the tables can be any size, the numeric range is spread evenly about first to last element.
    //the values are scaled by 1/65535 back to 0..1
    /*
    const QList<uint16_t> redTransferFunctionTable;
    const QList<uint16_t> greenTransferFunctionTable;
    const QList<uint16_t> blueTransferFunctionTable;
    QColorSpace newspace(pp.whitePoint, pp.redPoint, pp.greenPoint, pp.bluePoint,
                         redTransferFunctionTable,
                         greenTransferFunctionTable,
                         blueTransferFunctionTable);
                         */
    //for table extraction purposes, i guess it is best to transfer from @a space
    //to one with the same primaries but linear transfer functions.
    redTransferFunctionTable.clear();
    greenTransferFunctionTable.clear();
    blueTransferFunctionTable.clear();
    QColorSpace linear(space);
    linear.setTransferFunction(QColorSpace::TransferFunction::Linear, 0.0);
    auto transform = space.transformationToColorSpace(linear);
    for(unsigned int i = 0; i < 256; i++) {
        auto result = transform.map(QRgbaFloat32{i * 1.0f / 255, i * 1.0f / 255, i * 1.0f / 255, 0});
        redTransferFunctionTable.append(round(result.r * 65535));
        greenTransferFunctionTable.append(round(result.g * 65535));
        blueTransferFunctionTable.append(round(result.b * 65535));
    }
    /*
     this can be checked by recreating(copying) the same basic colorspace and
     attaching the transferfunctions extracted here. some color spaces
     are special in a way that is not available through the interface, so
     they cannot be recreated with primaries and the transferfunctions alone.
     */
}
