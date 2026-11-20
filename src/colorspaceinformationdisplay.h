#pragma once

#include <qwidget.h>
#include <QScopedPointer>

namespace Ui
{
class ColorSpaceInformationDisplay;
}

/** Display some information about the given color space
 *
 * This displays the numeric primary point coordinates and approximate gamma value
 * besides their position on the CIE xy plane and the transfer function.
 */
class ColorSpaceInformationDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit ColorSpaceInformationDisplay(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~ColorSpaceInformationDisplay() override;

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

private:
    const std::unique_ptr<Ui::ColorSpaceInformationDisplay> m_ui;
};

