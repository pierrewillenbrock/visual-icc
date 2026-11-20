
#pragma once

#include <QWidget>
#include <QColorSpace>
#include <QList>

class QTimer;

/** Display the primary points and transfer curves of a color space
 *
 * This displays the CIE color space x/y plane with the four primary colors
 * from the two color spaces and the transfer curves.
 */
class ColorSpaceInfoGraphicsWidget : public QWidget
{
    Q_OBJECT

public:
    ColorSpaceInfoGraphicsWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    /** Set the primary color space
     *
     * The primary color space is shown with bright dots and lines.
     *
     * @param space The color space
     */
    void setColorSpace(QColorSpace const &space);

    /** Set the secondary color space
     *
     * The secondary color space, if set, is shown with diminished dots and lines.
     *
     * @param space The color space
     */
    void setSecondaryColorSpace(QColorSpace const &space);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    /// The primary color space
    QColorSpace m_primaryColorSpace;
    /// The secondary color space
    QColorSpace m_secondaryColorSpace;

    /// The CIE x/y gamut image for the current widget size
    QImage m_gamut_image;

    /**
     * The transform that can be used to transform CIE x/y coordinates
     * to m_gamut_image coordinates */
    QTransform m_gamut_image_transform;
};


