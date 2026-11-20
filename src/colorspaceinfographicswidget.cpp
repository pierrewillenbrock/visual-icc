
#include "colorspaceinfographicswidget.h"

#include <QPainter>
#include <QFile>
#include <QWindow>
#include <QTimer>
#include <string.h>
#include <iostream>
#include "fix-colorspace.h"
#include "extract_colorspace_transfer_tables.h"

ColorSpaceInfoGraphicsWidget::ColorSpaceInfoGraphicsWidget(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f) {
    m_primaryColorSpace = QColorSpace(QColorSpace::SRgb);
    setAutoFillBackground(true);
}

void ColorSpaceInfoGraphicsWidget::setColorSpace(QColorSpace const &space) {
    m_primaryColorSpace = space;
    //no need to update the gamut image, it does not change with the color space
    //only the stuff drawn in front of it.
    update();
}

void ColorSpaceInfoGraphicsWidget::setSecondaryColorSpace(QColorSpace const &space) {
    m_secondaryColorSpace = space;
    update();
}


QSize ColorSpaceInfoGraphicsWidget::sizeHint() const
{
    QSize res = QWidget::sizeHint();
    if(res.width() > res.height() * 2) {
        res.setWidth(res.height() * 2);
    } else {
        res.setHeight(res.width() / 2);
    }
    return res;
}

QSize ColorSpaceInfoGraphicsWidget::minimumSizeHint() const
{
    QSize res = QWidget::minimumSizeHint();
    if(res.width() < res.height() * 2) {
        res.setWidth(res.height() * 2);
    } else {
        res.setHeight(res.width() / 2);
    }
    return res;
}

/** CIE XYZ colorspace coordinate
 *
 * Note the difference between XYZ and xy.
 */
struct XYZ_coord {
    float X;
    float Y;
    float Z;
};

/** Calculate a dual gaussian distribution function
 *
 * @param x x coordinate to calculate for
 * @param mu maximum value x coordinate
 * @param tau1 tau for x < mu
 * @param tau2 tau for x >= mu
 * @return The distribution value at x
 */
static float wavelength_g(float x, float mu, float tau1, float tau2) {
    if(x < mu) {
        return exp(-tau1*tau1*(x-mu)*(x-mu)/2);
    } else {
        return exp(-tau2*tau2*(x-mu)*(x-mu)/2);
    }
}

/** Calculate the XYZ coordinates for a wavelength
 *
 * Note the difference between XYZ and xy.
 *
 * @param lambda_nm  Wavelength in nanometers
 * @return The XYZ coordinate
 */
static XYZ_coord wavelength_XYZ(float lambda_nm) {
    XYZ_coord result;
    result.X = 1.056f * wavelength_g(lambda_nm, 599.8f, 0.0264f, 0.0323f)
               + 0.362f * wavelength_g(lambda_nm, 442.0f, 0.0624f, 0.0374f)
               - 0.065f * wavelength_g(lambda_nm, 501.1f, 0.0490f, 0.0382f);
    result.Y = 0.821f * wavelength_g(lambda_nm, 568.8f, 0.0213f, 0.0247f)
               + 0.286 * wavelength_g(lambda_nm, 530.9f, 0.0613f, 0.0322f);
    result.Z = 1.217f * wavelength_g(lambda_nm, 437.0f, 0.0845f, 0.0278f)
               + 0.681 * wavelength_g(lambda_nm, 459.0f, 0.0385f, 0.0725f);
    return result;
}

/** Calculate the xy coordinates for a wavelength
 *
 * Note the difference between XYZ and xy.
 *
 * @param lambda_nm  Wavelength in nanometers
 * @return The xy coordinate
 */
static QPointF wavelength_xy(float lambda_nm) {
    XYZ_coord pos = wavelength_XYZ(lambda_nm);

    return QPointF(pos.X / (pos.X + pos.Y + pos.Z),
                   pos.Y / (pos.X + pos.Y + pos.Z));
}

/** Find wavelength of the pure color with the highest y value
 *
 * This is a green color
 *
 * @param threshold Amount of y space between candidates to stop at
 * @return The found wavelength
 */
static float find_green(float threshold) {
    //find the green point
    float wl_low = 450;
    float wl_high = 650;
    while(true) {
        float wl_mid1 = wl_low * 2 / 3 + wl_high * 1 / 3;
        float wl_mid2 = wl_low * 1 / 3 + wl_high * 2 / 3;
        QPointF pos_low = wavelength_xy(wl_low);
        QPointF pos_high = wavelength_xy(wl_high);
        QPointF pos_mid1 = wavelength_xy(wl_mid1);
        QPointF pos_mid2 = wavelength_xy(wl_mid2);
        float miny = pos_low.y();
        float maxy = pos_low.y();
        if(miny > pos_mid1.y()) {
            miny = pos_mid1.y();
        }
        if(maxy < pos_mid1.y()) {
            maxy = pos_mid1.y();
        }
        if(miny > pos_mid2.y()) {
            miny = pos_mid2.y();
        }
        if(maxy < pos_mid2.y()) {
            maxy = pos_mid2.y();
        }
        if(miny > pos_high.y()) {
            miny = pos_high.y();
        }
        if(maxy < pos_high.y()) {
            maxy = pos_high.y();
        }
        if(int(maxy / threshold) == int(miny / threshold)) {
            break;
        }

        //the mid points must have higher y than the lower of low,high.
        //they can _both_ be below any of low,high.
        if(pos_mid1.y() < pos_high.y() && pos_mid2.y() < pos_high.y()) {
            //if both mids are lower than high, then they both must be on the left part of the graph.
            wl_low = wl_mid2;
        } else if(pos_mid1.y() < pos_low.y() && pos_mid2.y() < pos_low.y()) {
            //similar for low.
            wl_high = wl_mid1;
        } else if(pos_low.y() < pos_mid1.y() && pos_mid2.y() < pos_mid1.y()) {
            //mid1 is above both low and mid2, real max must be between the latter.
            wl_high = wl_mid2;
        } else if(pos_mid1.y() < pos_mid2.y() && pos_high.y() < pos_mid2.y()) {
            //mid2 is above both mid1 and high, real max must be between the latter.
            wl_low = wl_mid1;
        }
        /*
         * l m1 m2 h    m1,m2 < h
         * m1 l m2 h    m1,m2 < h; not possible
         * l m2 m1 h    m1,m2 < h
         * m1 m2 l h    m1,m2 < h; not possible
         * m2 l m1 h    m1,m2 < h; not possible
         * m2 m1 l h    m1,m2 < h; not possible
         * m1 m2 h l    m1,m2 < l; not possible
         * m2 m1 h l    m1,m2 < l; not possible
         * m1 h m2 l    m1,m2 < l; not possible
         * m2 h m1 l    m1,m2 < l; not possible
         * h m1 m2 l    m1,m2 < l
         * h m2 m1 l    m1,m2 < l
         * l m2 h m1    l,m2 < m1
         * m2 l h m1    l,m2 < m1; not possible
         * l h m2 m1    l,m2 < m1
         * m2 h l m1    l,m2 < m1; not possible
         * h l m2 m1    l,m2 < m1
         * h m2 l m1    l,m2 < m1
         * l m1 h m2    m1,h < m2
         * m1 l h m2    m1,h < m2; not possible
         * l h m1 m2    m1,h < m2
         * m1 h l m2    m1,h < m2; not possible
         * h l m1 m2    m1,h < m2
         * h m1 l m2    m1,h < m2
         */
    }
    return (wl_low + wl_high) / 2;
}

/** Get a pure color wavelength for an y coordinate
 *
 * @param y The y coordinate
 * @param threshold Amount of y space between candidates to stop at
 * @param wl_low The lower side of the wavelength range to scan for y coordinates
 * @param wl_high The higher side of the wavelength range to scan for y coordinates
 * @return The wavelength found
 */
static float find_wl_for_y(float y, float threshold, float wl_low, float wl_high) {
    QPointF pos_low = wavelength_xy(wl_low);
    QPointF pos_high = wavelength_xy(wl_high);
    float wl_low_y;
    float wl_high_y;
    if(pos_low.y() < pos_high.y()) {
        wl_low_y = wl_low;
        wl_high_y = wl_high;
    } else {
        wl_low_y = wl_high;
        wl_high_y = wl_low;
    }
    while(true) {
        float wl_mid = wl_low_y / 2 + wl_high_y / 2;
        QPointF pos_low = wavelength_xy(wl_low_y);
        QPointF pos_high = wavelength_xy(wl_high_y);
        QPointF pos_mid = wavelength_xy(wl_mid);
        if(y > pos_mid.y()) {
            if(pos_mid.y() - pos_low.y() < threshold) {
                return wl_mid;
            }
            wl_low_y = wl_mid;
        } else {
            if(pos_high.y() - pos_mid.y() < threshold) {
                return wl_mid;
            }
            wl_high_y = wl_mid;
        }
    }
    return (wl_low_y + wl_high_y) / 2;
}

/** Get the wavelength corresponding the lower x of two y coordinates
 *
 * @param low_y The low y coordinate
 * @param high_y The high y coordinate
 * @param wl_low The lower side of the wavelength range to scan for y coordinates
 * @param wl_high The higher side of the wavelength range to scan for y coordinates
 * @return The wavelength found
 */
static float find_lowest_x_between_y(float low_y, float high_y, float wl_low, float wl_high) {
    float wl_high_y = find_wl_for_y(high_y, (high_y - low_y) / 4, wl_low, wl_high);
    float wl_low_y = find_wl_for_y(low_y, (high_y - low_y) / 4, wl_low, wl_high);
    QPointF pos_low_y = wavelength_xy(wl_low_y);
    QPointF pos_high_y = wavelength_xy(wl_high_y);
    if(pos_low_y.x() < pos_high_y.x()) {
        return wl_low_y;
    } else {
        return wl_high_y;
    }
}

/** Get the wavelength corresponding the higher x of two y coordinates
 *
 * @param low_y The low y coordinate
 * @param high_y The high y coordinate
 * @param wl_low The lower side of the wavelength range to scan for y coordinates
 * @param wl_high The higher side of the wavelength range to scan for y coordinates
 * @return The wavelength found
 */
static float find_highest_x_between_y(float low_y, float high_y, float wl_low, float wl_high) {
    float wl_high_y = find_wl_for_y(high_y, (high_y - low_y) / 4, wl_low, wl_high);
    float wl_low_y = find_wl_for_y(low_y, (high_y - low_y) / 4, wl_low, wl_high);
    QPointF pos_low_y = wavelength_xy(wl_low_y);
    QPointF pos_high_y = wavelength_xy(wl_high_y);
    if(pos_low_y.x() > pos_high_y.x()) {
        return wl_low_y;
    } else {
        return wl_high_y;
    }
}

/** Render the gamut image
 *
 * Fills @a image with a scaled version of the real color portion of
 * a CIE xy plane and fills @a image_transform to match the scale
 *
 * @param image  A reference to the target image, properly sized already
 * @param image_transform A reference to a transform that will be filled
 *                        to convert the coordinates to image space
 */
static void render_gamut(QImage &image, QTransform &image_transform) {
    // 3. Tag the buffer with your target ICC working color space
    image.fill(Qt::white);

    int iw = image.width();
    int ih = image.height();
    if(iw < ih) {
        ih = iw;
    } else {
        iw = ih;
    }

    uint8_t *b_bits = image.bits();
    unsigned b_bytesperline = image.bytesPerLine();

    //find first and last point of the spectal colors
    float wl_blue = 450;
    float wl_red = 650;
    float wl_green = find_green(1.0f / ih / 2);
    QPointF pos_blue = wavelength_xy(wl_blue);
    QPointF pos_red = wavelength_xy(wl_red);
    QPointF pos_green = wavelength_xy(wl_green);
    QPoint blue(pos_blue.x() * iw, (1.0f - pos_blue.y()) * ih);
    QPoint red(pos_red.x() * iw, (1.0f - pos_red.y()) * ih);
    QPoint green(pos_green.x() * iw, (1.0f - pos_green.y()) * ih);

    float max_y = pos_green.y();
    float min_y = pos_blue.y() < pos_red.y()?pos_blue.y():pos_red.y();

    float min_x = 0;
    float max_x = max_y - min_y;

    image_transform.setMatrix(
      iw / (max_x - min_x), 0, 0,
      0, -ih / (max_y - min_y), 0,
      -iw * min_x / (max_x - min_x), ih * (min_y / (max_y - min_y) + 1.0f), 1);

    QTransform inverted_transform = image_transform.inverted();

    float wl_min_x = wl_green;
    float wl_max_x = wl_green;
    int px_py_offset = (QPointF(1.0-max_y, 0) * image_transform).x();
    for(int py = 0; py < ih; py++) {
        float y =  1.0f - py * 1.0f / ih;
        y = (QPointF(0, py) * inverted_transform).y();
        float gamut_min_x;
        float gamut_max_x;
        if(y > pos_blue.y()) {
            wl_min_x = find_lowest_x_between_y(y - 0.5f / ih, y + 0.5f / ih,
                                               wl_blue, wl_min_x);
            gamut_min_x = wavelength_xy(wl_min_x).x();
        } else {
            wl_min_x = wl_blue;
            gamut_min_x = (pos_blue.x() * pos_red.y() - pos_red.x() * pos_blue.y() +
                     (pos_red.x() - pos_blue.x()) * y)/(pos_red.y() - pos_blue.y());
        }
        if(y > pos_red.y()) {
            wl_max_x = find_highest_x_between_y(y - 0.5f / ih, y + 0.5f / ih,
                                                wl_max_x, wl_red);
            gamut_max_x = wavelength_xy(wl_max_x).x();
        } else {
            wl_max_x = wl_red;
            gamut_max_x = (pos_blue.x() * pos_red.y() - pos_red.x() * pos_blue.y() +
                     (pos_red.x() - pos_blue.x()) * y)/(pos_red.y() - pos_blue.y());
        }

        int min_px = (QPointF(gamut_min_x, 0) * image_transform).x();
        int max_px = (QPointF(gamut_max_x, 0) * image_transform).x();

        for(int px = 0; px < min_px; px++) {
            ((QRgb*)(b_bits + py * b_bytesperline))[px] = qRgb(255,255,255);
        }
        for(int px = max_px; px < iw; px++) {
            ((QRgb*)(b_bits + py * b_bytesperline))[px] = qRgb(255,255,255);
        }
        for(int px = py + px_py_offset; px < iw; px++) {
            ((QRgb*)(b_bits + py * b_bytesperline))[px] = qRgb(128,128,128);
        }

        for(int px = min_px; px < max_px; px++) {
            float x = (QPointF(px, 0) * inverted_transform).x();

            float z = 1.0f - x - y;

            float Y = 0.65; //brightness
            float X = (Y / y) * x;
            float Z = (Y / y) * z;

            //from CIE XYZ to rgb
            float r =  X * 1.4628067f - Y * 0.1840623f - Z * 0.2743606f;
            float g = -X * 0.5217933f + Y * 1.4472381f + Z * 0.0677227f;
            float b = X * 0.0349342f - Y * 0.0968930f + Z * 1.2884099f;

            //adjust gamma according to srgb
            r = r <= 0.0031308f ? 12.92f * r : (1.0f + 0.055f) * pow(r, (1.0f / 2.4f)) - 0.055f;
            g = g <= 0.0031308f ? 12.92f * g : (1.0f + 0.055f) * pow(g, (1.0f / 2.4f)) - 0.055f;
            b = b <= 0.0031308f ? 12.92f * b : (1.0f + 0.055f) * pow(b, (1.0f / 2.4f)) - 0.055f;

            QRgb color;
            /*if(r < 0 || r > 1.0 || g < 0 || g > 1.0 || b < 0 || b > 1.0) {
                color = qRgb(255,255,255);
            } else {*/
            //}
            r = r > 1.0?1.0:r < 0.0?0.0:r;
            g = g > 1.0?1.0:g < 0.0?0.0:g;
            b = b > 1.0?1.0:b < 0.0?0.0:b;
            color = qRgb(r * 255, g * 255, b * 255);

            if(X < 0 || Y < 0 || Z < 0) {
                color = qRgb(255,255,255);
            }

            ((QRgb*)(b_bits + py * b_bytesperline))[px] = color;
        }
    }

    /*
    //technically, 360 to 830, but below 450, it does a squigly and above 830, it goes back up.
    for(float wl_nm = wl_blue; wl_nm < wl_red; wl_nm += .1f) {
        QPointF pos = wavelength_xy(wl_nm);

        float y = (pos.y() - min_y) / (max_y - min_y);

        int px = pos.x() * iw;
        int py = (1.0f - y) * ih;
        if(px < 0) {
            px = 0;
        }
        if(px >= iw) {
            px = iw-1;
        }
        if(py < 0) {
            py = 0;
        }
        if(py >= ih) {
            py = ih-1;
        }
        QRgb color = qRgb(0,0,0);
        ((QRgb *)(b_bits + py * b_bytesperline))[px] = color;
    }
    */
}

/** Convert a transfer function table to points in a graph
 *
 * @param transferFunctionTable  The transfer function table
 * @param tgt The target rectangle
 * @return The points transformed to the target rectangle
 */
static QList<QPointF> transferFunctionTableToPointList(QList<uint16_t> const &transferFunctionTable, QRectF const &tgt) {
    QList<QPointF> points;
    int sz = transferFunctionTable.size();
    points.resize(sz);
    for(int i = 0; i < sz; i++) {
        float y1 = transferFunctionTable[i] / 65535.0;
        float x1 = i * 1.0f / (sz-1);
        points[i] = QPointF(x1 * tgt.width() + tgt.x(), (1.0 - y1) * tgt.height() + tgt.y());
    }
    return points;
}

void ColorSpaceInfoGraphicsWidget::paintEvent(QPaintEvent* event)
{
    QPainter widgetPainter(this);

    QSize virtualsize = size();
    if(virtualsize.width() > virtualsize.height() * 2 + 16) {
        virtualsize.setWidth(virtualsize.height() * 2 + 16);
    } else {
        virtualsize.setHeight((virtualsize.width() - 16) / 2);
    }

    QSize imagesize((virtualsize.width() - 16) / 2, virtualsize.height());

    imagesize *= devicePixelRatio();

   if(m_gamut_image.size() != imagesize) {
        m_gamut_image = QImage(imagesize, QImage::Format_ARGB32_Premultiplied);
        m_gamut_image.setDevicePixelRatio(devicePixelRatio());

        render_gamut(m_gamut_image, m_gamut_image_transform);
    }

    widgetPainter.drawImage(0, 0, m_gamut_image);

    if(m_secondaryColorSpace.isValid()) {
        auto pp = QColorSpace_primaryPoints(m_secondaryColorSpace);

        widgetPainter.setPen(Qt::darkGray);
        if(pp.isValid()) {
            QPointF r_p(pp.redPoint * m_gamut_image_transform / devicePixelRatio());
            QPointF g_p(pp.greenPoint * m_gamut_image_transform / devicePixelRatio());
            QPointF b_p(pp.bluePoint * m_gamut_image_transform / devicePixelRatio());
            QPointF w_p(pp.whitePoint * m_gamut_image_transform / devicePixelRatio());

            widgetPainter.drawLine(r_p, g_p);
            widgetPainter.drawLine(g_p, b_p);
            widgetPainter.drawLine(b_p, r_p);

            widgetPainter.setBrush(Qt::darkRed);
            widgetPainter.drawEllipse(r_p, 2, 2);

            widgetPainter.setBrush(Qt::darkGreen);
            widgetPainter.drawEllipse(g_p, 2, 2);

            widgetPainter.setBrush(Qt::darkBlue);
            widgetPainter.drawEllipse(b_p, 2, 2);

            widgetPainter.setBrush(Qt::gray);
            widgetPainter.drawEllipse(w_p, 2, 2);
        }

        QList<uint16_t> redTransferFunctionTable;
        QList<uint16_t> greenTransferFunctionTable;
        QList<uint16_t> blueTransferFunctionTable;

        extractColorSpaceTransferTables(m_secondaryColorSpace,
                                        redTransferFunctionTable,
                                        greenTransferFunctionTable,
                                        blueTransferFunctionTable);

        QRectF tgt((virtualsize.width() - 16) / 2 + 16, 0,
                   (virtualsize.width() - 16) / 2, virtualsize.height());
        widgetPainter.setPen(Qt::darkRed);
        widgetPainter.drawLines(transferFunctionTableToPointList(redTransferFunctionTable, tgt));
        widgetPainter.setPen(Qt::darkGreen);
        widgetPainter.drawLines(transferFunctionTableToPointList(greenTransferFunctionTable, tgt));
        widgetPainter.setPen(Qt::darkBlue);
        widgetPainter.drawLines(transferFunctionTableToPointList(blueTransferFunctionTable, tgt));
    }

    if(m_primaryColorSpace.isValid()) {
        auto pp = QColorSpace_primaryPoints(m_primaryColorSpace);

        widgetPainter.setPen(Qt::black);
        if(pp.isValid()) {
            QPointF r_p(pp.redPoint * m_gamut_image_transform / devicePixelRatio());
            QPointF g_p(pp.greenPoint * m_gamut_image_transform / devicePixelRatio());
            QPointF b_p(pp.bluePoint * m_gamut_image_transform / devicePixelRatio());
            QPointF w_p(pp.whitePoint * m_gamut_image_transform / devicePixelRatio());

            widgetPainter.drawLine(r_p, g_p);
            widgetPainter.drawLine(g_p, b_p);
            widgetPainter.drawLine(b_p, r_p);

            widgetPainter.setBrush(Qt::red);
            widgetPainter.drawEllipse(r_p, 2, 2);

            widgetPainter.setBrush(Qt::green);
            widgetPainter.drawEllipse(g_p, 2, 2);

            widgetPainter.setBrush(Qt::blue);
            widgetPainter.drawEllipse(b_p, 2, 2);

            widgetPainter.setBrush(Qt::white);
            widgetPainter.drawEllipse(w_p, 2, 2);
        }

        QList<uint16_t> redTransferFunctionTable;
        QList<uint16_t> greenTransferFunctionTable;
        QList<uint16_t> blueTransferFunctionTable;

        extractColorSpaceTransferTables(m_primaryColorSpace,
                                        redTransferFunctionTable,
                                        greenTransferFunctionTable,
                                        blueTransferFunctionTable);

        QRectF tgt((virtualsize.width() - 16) / 2 + 16, 0,
                   (virtualsize.width() - 16) / 2, virtualsize.height());
        widgetPainter.setPen(Qt::red);
        widgetPainter.drawLines(transferFunctionTableToPointList(redTransferFunctionTable, tgt));
        widgetPainter.setPen(Qt::green);
        widgetPainter.drawLines(transferFunctionTableToPointList(greenTransferFunctionTable, tgt));
        widgetPainter.setPen(Qt::blue);
        widgetPainter.drawLines(transferFunctionTableToPointList(blueTransferFunctionTable, tgt));
    }
}

