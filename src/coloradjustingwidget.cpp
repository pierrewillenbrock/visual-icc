#include "coloradjustingwidget.h"

#include <QPainter>
#include <QFile>
#include <QWindow>
#include <QTimer>
#include <string.h>

/*
 * Qt encapsulates the final steps of color processing to where the
 * application only tells it, what color space is being used
 * with QWindow::format()::colorSpace and renders according to that.
 *
 * The full chain looks like this:
 * display_pixel = transform(display_profile,qt_profile) * transform(qt_profile,app_profile) * app_pixel
 *
 * where transform(display_profile,qt_profile) happens inside the compositor and
 * transform(qt_profile,app_profile) is done by qt. We are not really concerned
 * by qt_profile, we only care for transform(display_profile,app_profile),
 * which luckily is transform(display_profile,qt_profile) * transform(qt_profile,app_profile)
 *
 * So, how do we go about this?
 *
 * probably best to render directly in display color profile, that should give
 * us "raw" pixels. then, we can adjust that profile so that the visible
 * things are correct.
 *
 * when one would want to render raw pixels, one would just select the display
 * profile to render in.
 *
 * when one would want to preview a changed profile, one would
 * start at a known profile(ex. srgb) convert that to the
 * changed profile, and render that in the display profile.
 *
 * we need to figure out how to integrate the "normal" visual tests
 * (fine checker board vs plain color) that only provide a single reference
 * point.
 *
 * => we only adjust the transfer functions
 *
 */

ColorAdjustingWidget::ColorAdjustingWidget(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f),
      m_displayState(0) {
    m_activeDisplayColorSpace = QColorSpace(QColorSpace::SRgb);
    m_targetDisplayColorSpace = QColorSpace(QColorSpace::SRgb);

    m_updateTimer = new QTimer();
    // this redraws the widget ~50 times per second. if the display
    // is slow, this might change results.
    m_updateTimer->start(20);

    Block b;
    b.bg = Qt::black;
    b.fg = Qt::white;
    b.fraction = 0x80;
    b.rect = QRectF(0,0,0.5,0.5);
    m_blocks.push_back(b);

    b.rect = QRectF(0.5,0.5,0.5,0.5);
    m_blocks.push_back(b);

    b.bg = QColor::fromRgb(0x80,0x80,0x80);
    b.fg = b.bg;

    b.rect = QRectF(0.0,0.5,0.5,0.5);
    m_blocks.push_back(b);

    b.rect = QRectF(0.5,0.0,0.5,0.5);
    m_blocks.push_back(b);

    connect(m_updateTimer, &QTimer::timeout,
    this, [this]() {
        update();
    });
}

void ColorAdjustingWidget::clearBlocks() {
    m_blocks.clear();
}

void ColorAdjustingWidget::addBlock(QRectF const &rect, uint8_t fraction, QColor const &fg, QColor const &bg) {
    m_blocks.push_back(Block{fg, bg, fraction, rect});
}

QSize ColorAdjustingWidget::sizeHint() const
{
    return QWidget::sizeHint();
}

QSize ColorAdjustingWidget::minimumSizeHint() const
{
    return QWidget::minimumSizeHint();
}

/** Clamp to unsigned 8 bit
 */
static uint8_t clamp_uint8(int v) {
    return v<0?0:
           v>255?255:
           v;
}

/** Clamp to unsigned 16 bit
 */
static uint16_t clamp_uint16(int v) {
    return v<0?0:
           v>65535?65535:
           v;
}

#if 1
///floyd-steinberg kernel
/* this discards the lowest ~2 bits: 2 * 112 / 256 = 0.875 = 0 and they never
 * get distributed. we do a temporal density adjustment that fixes that later.
 */
static int kernel[] = {
     0,  0,112,
    48, 80, 16,
};
/// half width of kernel, counting the center pixel as 1
static constexpr int kernelhw = 2;
#endif

#if 0
///floyd-steinberg kernel
/* this discards the lowest ~2 bits: 2 * 112 / 256 = 0.875 = 0 and they never
 * get distributed. we do a temporal density adjustment that fixes that later.
 */
static int kernel[] = {
    0,  0,  0,112,  0,
    0, 48, 80, 16,  0,
    0,  0,  0,  0,  0
};

/// half width of kernel, counting the center pixel as 1
static int constexpr kernelhw = 3;
#endif

#if 0
///floyd-steinberg kernel somewhat expanded over the whole thing
static int kernel[] = {
    0,  0,  0, 33, 29,
   18, 25, 31, 23, 21,
   11, 16, 27, 13,  9
};

/// half width of kernel, counting the center pixel as 1
static int constexpr kernelhw = 3;
#endif

void ColorAdjustingWidget::paintEvent(QPaintEvent* event)
{
    QPainter widgetPainter(this);

    unsigned drawCount = 1;
    unsigned resettlingDrawCount = 40;

    // 2. Create an offscreen buffer matching the widget size
    // Use an alpha-supporting format like Format_ARGB32_Premultiplied
    QSize imagesize = size() * devicePixelRatio();
    if(m_buffer.size() != imagesize) {
        drawCount = resettlingDrawCount;
        m_buffer = QImage(imagesize, QImage::Format_ARGB32_Premultiplied);
    }
    if(m_residue.size() != imagesize) {
        drawCount = resettlingDrawCount;
        m_residue = QImage(imagesize, QImage::Format_Grayscale8);
        uint8_t *r_bits = m_residue.bits();
        unsigned r_bytesperline = m_residue.bytesPerLine();
        for(auto &b : m_blocks) {
            QRect r = QRectF(b.rect.x() * imagesize.width(), b.rect.y() * imagesize.height(),
                    b.rect.width() * imagesize.width(), b.rect.height() * imagesize.height()).toRect();
            r = r.intersected(m_residue.rect());

            int rx = r.x();
            int ry = r.y();
            int rw = r.width();
            int rh = r.height();

            int tgtval = b.fraction;
            for(int y = ry; y < ry + rh; y++) {
                memset(r_bits + y * r_bytesperline + rx, tgtval, rw);
            }
        }
    }
    if(m_temporal_sum.size() != imagesize) {
        drawCount = resettlingDrawCount;
        m_temporal_sum = QImage(imagesize, QImage::Format_Grayscale16);
        uint8_t *t_bits = m_temporal_sum.bits();
        unsigned t_bytesperline = m_temporal_sum.bytesPerLine();
        for(auto &b : m_blocks) {
            QRect r = QRectF(b.rect.x() * imagesize.width(), b.rect.y() * imagesize.height(),
                    b.rect.width() * imagesize.width(), b.rect.height() * imagesize.height()).toRect();
            r = r.intersected(m_residue.rect());

            int rx = r.x();
            int ry = r.y();
            int rw = r.width();
            int rh = r.height();

            int tgtval = b.fraction * 0x101;
            for(int y = ry; y < ry + rh; y++) {
                for(int x = rx; x < rx + rw; x++) {
                    m_displayState *= 59811;
                    m_displayState += 1;
                    m_displayState ^= m_displayState >> 8;
                    ((uint16_t *)(t_bits + y * t_bytesperline))[x] = tgtval ^ (m_displayState & 0x3fff);
                }
            }
        }
    }

    if(m_error_sum.size() != imagesize) {
        drawCount = resettlingDrawCount;
        m_error_sum = QImage(imagesize, QImage::Format_Grayscale16);
        uint8_t *e_bits = m_error_sum.bits();
        unsigned e_bytesperline = m_error_sum.bytesPerLine();
        for(int y = 0; y < imagesize.height(); y++) {
            for(int x = 0; x < imagesize.width(); x++) {
                ((uint16_t*)(e_bits + y * e_bytesperline))[x] = 0x8000;
            }
        }
    }

    m_buffer.setDevicePixelRatio(devicePixelRatio());

    // 3. Tag the buffer with your target ICC working color space
    m_buffer.setColorSpace(QColorSpace(QColorSpace::NamedColorSpace::SRgbLinear));
    m_buffer.fill(Qt::black);

    uint8_t *r_bits = m_residue.bits();
    unsigned r_bytesperline = m_residue.bytesPerLine();
    uint8_t *b_bits = m_buffer.bits();
    unsigned b_bytesperline = m_buffer.bytesPerLine();
    uint8_t *t_bits = m_temporal_sum.bits();
    unsigned t_bytesperline = m_temporal_sum.bytesPerLine();
    uint8_t *e_bits = m_error_sum.bits();
    unsigned e_bytesperline = m_error_sum.bytesPerLine();


    for(unsigned int redraws = 0; redraws < drawCount; redraws++) {
        for(auto &b : m_blocks) {
            QRgb bg = b.bg.rgb();
            QRgb fg = b.fg.rgb();
            QRect r = QRectF(b.rect.x() * imagesize.width(), b.rect.y() * imagesize.height(),
                    b.rect.width() * imagesize.width(), b.rect.height() * imagesize.height()).toRect();
            r = r.intersected(m_residue.rect());

            int rx = r.x();
            int ry = r.y();
            int rw = r.width();
            int rh = r.height();

            int tgtval = b.fraction;

            // setup a gray image in m_residue such that we get the densities
            // in m_temporal_sum to be tgtval this is a very simple
            // I-controller(using m_error_sum) for each pixel with some noise
            // added in to make it not oscillate the same pattern all the time.
            for(int y = ry; y < ry + rh; y++) {
                for(int x = rx; x < rx + rw; x++) {
                    uint16_t temporal_sum = ((uint16_t *)(t_bits + y * t_bytesperline))[x];
                    uint16_t &error_sum = ((uint16_t *)(e_bits + y * e_bytesperline))[x];

                    int error = tgtval * 0x100 - (int)temporal_sum;
                    error_sum = clamp_uint16((int)error_sum + error / 0x40);
                    int adjust = ((int)error_sum - 0x8000);

                    m_displayState *= 59811;
                    m_displayState += 1;
                    m_displayState ^= m_displayState >> 8;
                    if((m_displayState & 0xfe00) == 0xfe00) {
                        if(m_displayState & 0x100) {
                            adjust += m_displayState & 0x0f;
                        } else {
                            adjust -= m_displayState & 0x0f;
                        }
                    }

                    ((uint8_t *)(r_bits + y * r_bytesperline))[x] = clamp_uint8(tgtval + adjust);
                }
            }

            for(int y = ry; y < ry + rh; y++) {
                for(int x = rx; x < rx + rw; x++) {
                    uint8_t oldpixel = ((uint8_t *)(r_bits + y * r_bytesperline))[x];
                    uint8_t newpixel = oldpixel < 0x80?0x00:0xff;
                    int quant_error = (int)oldpixel - (int)newpixel;

                    int end_ky = kernelhw;
                    if(end_ky > ry+rh-y) {
                        end_ky = ry+rh-y;
                    }
                    int end_kx = kernelhw;
                    if(end_kx > rx+rw-x) {
                        end_kx = rx+rw-x;
                    }
                    int begin_kx = -kernelhw+1;
                    if(begin_kx < rx-x) {
                        begin_kx = rx-x;
                    }

                    uint8_t *r_bits_org = r_bits + y * r_bytesperline + x;
                    int *kernel_org = kernel;

                    for(int ky = 0; ky < end_ky; ky++) {
                        for(int kx = begin_kx; kx < end_kx; kx++) {
                            int kv = kernel_org[kx + kernelhw-1];
                            int adjust = (quant_error * kv) / 256;
                            uint8_t &rval = r_bits_org[kx];
                            rval = clamp_uint8((int)rval + adjust);
                        }
                        r_bits_org += r_bytesperline;
                        kernel_org += kernelhw*2-1;
                    }

                    if(newpixel < 0x80) {
                        ((QRgb *)(b_bits + y * b_bytesperline))[x] = bg;
                        ((uint16_t *)(t_bits + y * t_bytesperline))[x] =
                        clamp_uint16(((int)((uint16_t *)(t_bits + y * t_bytesperline))[x] * 0xc0 + 0 * 0x40) / 0x100);
                    } else {
                        ((QRgb *)(b_bits + y * b_bytesperline))[x] = fg;
                        ((uint16_t *)(t_bits + y * t_bytesperline))[x] =
                        clamp_uint16(((int)((uint16_t *)(t_bits + y * t_bytesperline))[x] * 0xc0 + 0xffff * 0x40) / 0x100);
                    }
                }
            }
        }
    }

    QColorSpace windowColorSpace = QColorSpace(QColorSpace::SRgb);
    if(QWindow *windowHandle = this->window()->windowHandle()) {
        windowColorSpace = windowHandle->format().colorSpace();
        if(!windowColorSpace.isValid()) {
            windowColorSpace = QColorSpace(QColorSpace::SRgb);
        }
    }

    // convert from the srgb linear space to the simulated display color space
    m_buffer.convertToColorSpace(m_targetDisplayColorSpace);

    // switch to the actual display color space. this preserves the values we
    // want to see output to the display(constrained by the window color space).
    m_buffer.setColorSpace(m_activeDisplayColorSpace);

    // convert back to the window color space, the painter does not handle that for us.
    m_buffer.convertToColorSpace(windowColorSpace);

    widgetPainter.drawImage(0, 0, m_buffer);
}


