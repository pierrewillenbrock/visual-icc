#ifndef VISUAL_ICC_H
#define VISUAL_ICC_H

#include <QMainWindow>
#include <QColorSpace>

#include <memory>

namespace Ui {
class visual_icc;
}

class visual_icc : public QMainWindow
{
    Q_OBJECT

public:
    explicit visual_icc(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~visual_icc() override;

private:
    const std::unique_ptr<Ui::visual_icc> m_ui;

    /// The result color space currently being worked on
    QColorSpace m_resultColorSpace;

    /** Update all the places the color space is used
     *
     * @param space the new color space
     */
    void updateColorSpace(QColorSpace const &space);
};

#endif // VISUAL_ICC_H
