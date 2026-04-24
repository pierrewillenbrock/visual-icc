#include "visual-icc.h"
#include "ui_visual-icc.h"

visual_icc::visual_icc(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::visual_icc)
{
    m_ui->setupUi(this);
}

visual_icc::~visual_icc() = default;

#include "moc_visual-icc.cpp"
