
#pragma once

#include <QColorSpace>

/** Extract transfer tables from an color space
 *
 * The transfer tables can be put back into space.setTransferFunctions and
 * (approximately) get the same color space back.
 *
 * Note the color space cannot be reconstructed from primary points and
 * transfer tables alone.
 *
 * @param space The source color space
 * @param redTransferFunctionTable A reference to a QList receiving the red transfer table
 * @param greenTransferFunctionTable A reference to a QList receiving the green transfer table
 * @param blueTransferFunctionTable A reference to a QList receiving the blue transfer table
 */
void extractColorSpaceTransferTables(QColorSpace const &space,
                                     QList<uint16_t> &redTransferFunctionTable,
                                     QList<uint16_t> &greenTransferFunctionTable,
                                     QList<uint16_t> &blueTransferFunctionTable);
