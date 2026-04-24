#ifndef VISUAL_ICC_H
#define VISUAL_ICC_H

#include <QMainWindow>

#include <memory>

namespace Ui {
class visual_icc;
}

class visual_icc : public QMainWindow
{
    Q_OBJECT

public:
    explicit visual_icc(QWidget *parent = nullptr);
    ~visual_icc() override;

private:
    const std::unique_ptr<Ui::visual_icc> m_ui;
};

#endif // VISUAL_ICC_H
