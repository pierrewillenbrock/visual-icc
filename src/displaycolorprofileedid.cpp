
#include <QFile>
#include <QByteArray>
#include <QColorSpace>
#include <QPointF>
#include <QDebug>

QColorSpace parseEdidToColorSpace(const QString &edidPath) {
    QFile file(edidPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open EDID file:" << edidPath;
        return QColorSpace(QColorSpace::SRgb);
    }

    QByteArray edid = file.readAll();
    // A base EDID block must be at least 128 bytes
    if (edid.size() < 128) {
        qWarning() << "Invalid EDID data length:" << edid.size();
        return QColorSpace(QColorSpace::SRgb);
    }

    // Verify EDID Magic Header: 00 FF FF FF FF FF FF 00
    const char magicHeader[] = { 0x00, (char)0xFF, (char)0xFF, (char)0xFF,
                                 (char)0xFF, (char)0xFF, (char)0xFF, 0x00 };
    if (memcmp(edid.constData(), magicHeader, 8) != 0) {
        qWarning() << "EDID magic header mismatch.";
        return QColorSpace(QColorSpace::SRgb);
    }

    // --- 1. Parse Gamma ---
    // Byte 0x17 (offset 23): Stored as (Gamma * 100) - 100. Value 0xFF means defined in DI extension.
    unsigned char rawGamma = static_cast<unsigned char>(edid[0x17]);
    double gamma = 2.2; // Fallback standard
    if (rawGamma != 0xFF && rawGamma != 0x00) {
        gamma = 1.0 + (rawGamma / 100.0);
    }

    // --- 2. Parse Chromaticity Coordinates (10 Bytes total: offsets 25 to 34) ---
    // Extract the shared fractional bits block (Byte 0x19 / offset 25)
    unsigned char rx_ry_low = static_cast<unsigned char>(edid[25]);
    unsigned char gx_gy_low = static_cast<unsigned char>(edid[26]);
    unsigned char bx_by_low = static_cast<unsigned char>(edid[27]);
    unsigned char wx_wy_low = static_cast<unsigned char>(edid[28]);

    // Reconstruct 10-bit values combining high bytes with bit-shifted fractional bits
    unsigned int red_x   = (static_cast<unsigned char>(edid[29]) << 2) | ((rx_ry_low >> 6) & 0x03);
    unsigned int red_y   = (static_cast<unsigned char>(edid[30]) << 2) | ((rx_ry_low >> 4) & 0x03);
    unsigned int green_x = (static_cast<unsigned char>(edid[31]) << 2) | ((gx_gy_low >> 6) & 0x03);
    unsigned int green_y = (static_cast<unsigned char>(edid[32]) << 2) | ((gx_gy_low >> 4) & 0x03);
    unsigned int blue_x  = (static_cast<unsigned char>(edid[33]) << 2) | ((bx_by_low >> 6) & 0x03);
    unsigned int blue_y  = (static_cast<unsigned char>(edid[34]) << 2) | ((bx_by_low >> 4) & 0x03);
    unsigned int white_x = (static_cast<unsigned char>(edid[35]) << 2) | ((wx_wy_low >> 6) & 0x03);
    unsigned int white_y = (static_cast<unsigned char>(edid[36]) << 2) | ((wx_wy_low >> 4) & 0x03);

    // --- 3. Convert to CIE 1931 float points (Value / 1024.0) ---
    QPointF redPoint(  red_x   / 1024.0, red_y   / 1024.0);
    QPointF greenPoint(green_x / 1024.0, green_y / 1024.0);
    QPointF bluePoint( blue_x  / 1024.0, blue_y  / 1024.0);
    QPointF whitePoint(white_x / 1024.0, white_y / 1024.0);

    // Sanity check coordinates to prevent generating an invalid layout
    if (redPoint.x() == 0 || greenPoint.y() == 0 || whitePoint.y() == 0) {
        qWarning() << "Parsed coordinates look corrupted. Aborting custom conversion.";
        return QColorSpace(QColorSpace::SRgb);
    }

    qDebug() << "Successfully parsed physical monitor coordinates from EDID:";
    qDebug() << "  Red Primaries:" << redPoint;
    qDebug() << "  Green Primaries:" << greenPoint;
    qDebug() << "  Blue Primaries:" << bluePoint;
    qDebug() << "  White Point:" << whitePoint;
    qDebug() << "  Hardware Gamma:" << gamma;

    // --- 4. Construct the custom matching QColorSpace ---
    // If you need exact gamma mapping, use the continuous gamma value construct:
    return QColorSpace(redPoint, greenPoint, bluePoint, whitePoint,
                      QColorSpace::TransferFunction::Gamma, static_cast<float>(gamma));
}
