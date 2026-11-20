#pragma once

#include <QWidget>
#include <QColorSpace>
#include <map>

class QSlider;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;

/** Color space transfer function editor
 *
 * This allows to move about sections of the transfer function(s) in
 * the inverted transfer function.
 */
class ColorSpaceTransferFunctionEditor : public QWidget
{
    Q_OBJECT

public:
    ColorSpaceTransferFunctionEditor(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~ColorSpaceTransferFunctionEditor();

    /** Set the component transfer functions to be edited
     *
     * @param red If true, the red component will be edited
     * @param green If true, the green component will be edited
     * @param blue If true, the blue component will be edited
     */
    void setActiveColorComponents(bool red, bool green, bool blue);

    /** Add an editing position to the editor
     *
     * @param sourcePosition Position to be added. These are 0 to 255,
     * 0 meaning position with absence of color and 255 meaning full color.
     * 0 and 255 themselves cannot be changed.
     */
    void addPosition(uint8_t sourcePosition);

    /** Clear all editing position
     */
    void clearPositions();

public slots:
    /** Set the color space that is to be edited
     *
     * @param space The color space to be edited(a copy will be taken)
     */
    void setColorSpace(QColorSpace const &space);

signals:
    /** The color space was edited/changed
     *
     * @param space The color space updated according to the edit(s)
     */
    void colorSpaceChanged(QColorSpace const &space);

private:
    /// Information about each editing position
    struct PositionInfo {
        uint8_t sourcePosition; ///< Position of the editing
        QSlider *sld;///< slider for this position
        QLabel *lblTop;///< the top label, containing the sourcePosition
        QLabel *lblBottom;///< the bottom label, containing the fraction it is being mapped to
        QVBoxLayout *vertLayout;///< containing layout
    };

    /// The color space being edited
    QColorSpace m_colorSpace;
    /// The horizontal box layout containing all the vertical layouts for each editing position
    QHBoxLayout *m_horzLayout;
    /// Map for all position infos, mapping the source position to the information
    std::map<uint8_t, PositionInfo> m_positionInfos;

    /// The original red transfer function table
    QList<uint16_t> m_originalRedTransferFunctionTable;
    /// The original green transfer function table
    QList<uint16_t> m_originalGreenTransferFunctionTable;
    /// The original blue transfer function table
    QList<uint16_t> m_originalBlueTransferFunctionTable;
    /// The changed red transfer function table
    QList<uint16_t> m_redTransferFunctionTable;
    /// The changed green transfer function table
    QList<uint16_t> m_greenTransferFunctionTable;
    /// The changed bluetransfer function table
    QList<uint16_t> m_blueTransferFunctionTable;

    /// Set if red is being edited
    bool m_redActive = true;
    /// Set if green is being edited
    bool m_greenActive = true;
    /// Set if blue is being edited
    bool m_blueActive = true;

    /// Set while the color space is being updated, suppressing the colorSpaceChanged signal
    bool m_updatingColorSpace = false;

    /** Extract the transfer function value(average) at position
     *
     * @param position the position to query
     * @return the value the @a position maps to
     */
    double getTransferFunctionValueAt(uint8_t position);

    /** Update the transfer functions from the originals and the settings of the sliders
     */
    void updateTransferFunctions();
};

