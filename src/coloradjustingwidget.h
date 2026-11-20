
#pragma once

#include <QWidget>
#include <QColorSpace>
#include <QList>

class QTimer;

/** Color space aware display color space preview widget
 *
 * This renders a set of two-color blocks with a given density, with
 * rapidly changing random patterns such that the whole area
 * has the same temporal density.
 *
 * Colors are rendered in SRGBLinear color space.
 */
class ColorAdjustingWidget : public QWidget
{
    Q_OBJECT

public:
    ColorAdjustingWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    /** Clear all color blocks
     */
    void clearBlocks();

    /** Add a color block
     *
     * @param rect The rectangle of the block
     * @param fraction The density fraction, 0 to 255. 0 meaning all bg, 255 meaning all fg.
     * @param fg Foreground color, in SRGBLinear color space
     * @param bg Background color, in SRGBLinear color space
     */
    void addBlock(QRectF const &rect, uint8_t fraction, QColor const &fg, QColor const &bg = Qt::black);

public slots:
    /** Set the currently active display color space
     *
     * @param space the color space
     */
    void setActiveDisplayColorSpace(QColorSpace const &space) {
        m_activeDisplayColorSpace = space;
        update();
    }

    /** Set the simulated display color space
     *
     * @param space the color space
     */
    void setTargetDisplayColorSpace(QColorSpace const &space) {
        m_targetDisplayColorSpace = space;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct Block {
        QColor fg;
        QColor bg;
        uint8_t fraction;
        QRectF rect;
    };

    /// The currently active display color space
    QColorSpace m_activeDisplayColorSpace;

    /// The current target, simulated color space
    QColorSpace m_targetDisplayColorSpace;

    /// Timer for redrawing
    QTimer *m_updateTimer;

    /// Storage for a fast random number generator
    unsigned m_displayState;

    /// The list of color blocks
    QList<Block> m_blocks;

    /** Image containing the temporal sum of the display
     *
     * Meaning a low pass filtered average about each pixel.
     * Used to provide per pixel error value against the target density/fraction.
     */
    QImage m_temporal_sum;

    /** Image containing an error sum
     *
     * Used in the I-controller controlling the color request, steering it
     * towards the correct density/fraction
     */
    QImage m_error_sum;

    /** Image containing the target gray scale data
     *
     * This has the result of the I-controller and also is changed during
     * rendering according to the floyd-steinberg kernel.
     */
    QImage m_residue;

    /** The temporary image of the widget
     *
     * This contains only the fg/bg colors for each block,
     * filled according to \c m_residue.
     */
    QImage m_buffer;
};

