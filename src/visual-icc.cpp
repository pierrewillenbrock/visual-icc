#include "visual-icc.h"
#include "ui_visual-icc.h"
#include <QWindow>
#include <QScreen>
#include <QFile>
#include <QFileInfo>
#include "findcolorprofile.h"
#include "colorspaceinformationdisplay.h"
#include "extract_colorspace_transfer_tables.h"
#include <QFileDialog>

/** Fills @a wdg with a 2x2 checkerboard pattern
 *
 * @param wdg  The widget to be configured
 * @param fraction The fraction to be used with the two-tone blocks and
 *                 linear color fraction for the constant color blocks.
 *                 0..255, 0 meaning black.
 * @param red If true, the red component is setup, else left at 0
 * @param green If true, the green component is setup, else left at 0
 * @param blue If true, the blue component is setup, else left at 0
 */
static void setupCrossBlocks(ColorAdjustingWidget *wdg, uint8_t fraction, bool red, bool green, bool blue) {
    QColor fg = QColor::fromRgb(red?255:0,green?255:0,blue?255:0);
    QColor other = QColor::fromRgb(red?fraction:0,green?fraction:0,blue?fraction:0);
    wdg->clearBlocks();
    wdg->addBlock(QRectF(0.0, 0.0, 0.5, 0.5), fraction, fg);
    wdg->addBlock(QRectF(0.5, 0.5, 0.5, 0.5), fraction, fg);
    wdg->addBlock(QRectF(0.0, 0.5, 0.5, 0.5), 255, other);
    wdg->addBlock(QRectF(0.5, 0.0, 0.5, 0.5), 255, other);
}

/** Setup the @a caw with vertical stripes of blocks and the @a cstfw with matching fractions
 *
 * @param caw  The ColorAdjustingWidget to be configured
 * @param cstfe  The ColorSpaceTransferFunctionEditor to be configured
 * @param fractions A list of fractions to be used with the two-tone blocks and
 *                 linear color fraction for the constant color blocks.
 *                 0..255, 0 meaning black.
 * @param red If true, the red component is setup, else left at 0
 * @param green If true, the green component is setup, else left at 0
 * @param blue If true, the blue component is setup, else left at 0
 * @param distance empty distance between columns. Must be smaller than
 *                 1.0 / fractions.size()
 */
static void setupMultiColumnBlocks(ColorAdjustingWidget *caw, ColorSpaceTransferFunctionEditor *cstfe,
                                   QList<uint8_t> fractions,
                                   bool red, bool green, bool blue,
                                   float distance) {
    caw->clearBlocks();
    cstfe->clearPositions();
    float block_width = (1.0-distance * (fractions.size()-1))/fractions.size();
    float block_offset = block_width + distance;
    for(int i = 0; i < fractions.size(); i++) {
        uint8_t fraction = fractions[i];
        QColor fg = QColor::fromRgb(red?255:0,green?255:0,blue?255:0);
        QColor other = QColor::fromRgb(red?fraction:0,green?fraction:0,blue?fraction:0);
        caw->addBlock(QRectF(i*block_offset, 0.0, block_width, 1.0/3), fraction, fg);
        caw->addBlock(QRectF(i*block_offset, 1.0/3, block_width, 1.0/3), 255, other);
        caw->addBlock(QRectF(i*block_offset, 2.0/3, block_width, 1.0/3), fraction, fg);

        cstfe->addPosition(fraction);
    }
}

visual_icc::visual_icc(QWidget* parent, Qt::WindowFlags f)
    : QMainWindow(parent, f),
      m_ui(new Ui::visual_icc)
{
    m_ui->setupUi(this);

    auto window = this->window()->windowHandle();
    QString output;
    if(window) {
        auto screen = window->screen();
        if(screen) {
            output = screen->name();
        }
    }
    ColorProfileInformation info = findColorProfileInformation(output);
    if(info) {
        m_ui->lneICCProfilePath->setText(info.iccProfileFile);
        m_ui->lneDisplayName->setText(info.output);
        switch(info.type) {
        case ColorProfileInformation::CPIT_NONE:
            m_ui->lneProfileType->setText(tr("Unknown"));
            break;
        case ColorProfileInformation::CPIT_EDID:
            m_ui->lneProfileType->setText(tr("EDID"));
            break;
        case ColorProfileInformation::CPIT_ICC:
            m_ui->lneProfileType->setText(tr("ICC"));
            break;
        }
        m_ui->lneProfileSource->setText(info.providerPlugin);
    }

    updateColorSpace(info.colorSpace);

    setupCrossBlocks(m_ui->cawIntro1, 128, true, true, true);
    setupCrossBlocks(m_ui->cawIntro2, 128, true, false, false);
    setupCrossBlocks(m_ui->cawIntro3, 128, false, true, false);
    setupCrossBlocks(m_ui->cawIntro4, 128, false, false, true);

    m_ui->cstfeGray->setActiveColorComponents(true, true, true);

    setupMultiColumnBlocks(m_ui->cawTransferFuncGray, m_ui->cstfeGray,
                           {4, 8, 32, 64, 128, 176, 208, 224, 240},
                           true, true, true,
                           0.1/8);

    connect(m_ui->cstfeGray, &ColorSpaceTransferFunctionEditor::colorSpaceChanged,
            m_ui->cawTransferFuncGray, &ColorAdjustingWidget::setTargetDisplayColorSpace);

    m_ui->cstfeRed->setActiveColorComponents(true, false, false);

    setupMultiColumnBlocks(m_ui->cawTransferFuncRed, m_ui->cstfeRed,
                           {4, 8, 32, 64, 128, 176, 208, 224, 240},
                           true, false, false,
                           0.1/8);

    connect(m_ui->cstfeRed, &ColorSpaceTransferFunctionEditor::colorSpaceChanged,
            m_ui->cawTransferFuncRed, &ColorAdjustingWidget::setTargetDisplayColorSpace);

    m_ui->cstfeGreen->setActiveColorComponents(false, true, false);

    setupMultiColumnBlocks(m_ui->cawTransferFuncGreen, m_ui->cstfeGreen,
                           {4, 8, 32, 64, 128, 176, 208, 224, 240},
                           false, true, false,
                           0.1/8);

    connect(m_ui->cstfeGreen, &ColorSpaceTransferFunctionEditor::colorSpaceChanged,
            m_ui->cawTransferFuncGreen, &ColorAdjustingWidget::setTargetDisplayColorSpace);

    m_ui->cstfeBlue->setActiveColorComponents(false, false, true);

    setupMultiColumnBlocks(m_ui->cawTransferFuncBlue, m_ui->cstfeBlue,
                           {4, 8, 32, 64, 128, 176, 208, 224, 240},
                           false, false, true,
                           0.1/8);

    connect(m_ui->cstfeBlue, &ColorSpaceTransferFunctionEditor::colorSpaceChanged,
            m_ui->cawTransferFuncBlue, &ColorAdjustingWidget::setTargetDisplayColorSpace);



    connect(m_ui->cstfeGray, &ColorSpaceTransferFunctionEditor::colorSpaceChanged,
            m_ui->cawTransferFuncRed, &ColorAdjustingWidget::setTargetDisplayColorSpace);
    connect(m_ui->cstfeGray, &ColorSpaceTransferFunctionEditor::colorSpaceChanged,
            m_ui->cawTransferFuncGreen, &ColorAdjustingWidget::setTargetDisplayColorSpace);
    connect(m_ui->cstfeGray, &ColorSpaceTransferFunctionEditor::colorSpaceChanged,
            m_ui->cawTransferFuncBlue, &ColorAdjustingWidget::setTargetDisplayColorSpace);

    connect(m_ui->cstfeGray, &ColorSpaceTransferFunctionEditor::colorSpaceChanged,
            m_ui->cstfeRed, &ColorSpaceTransferFunctionEditor::setColorSpace);
    connect(m_ui->cstfeGray, &ColorSpaceTransferFunctionEditor::colorSpaceChanged,
            m_ui->cstfeGreen, &ColorSpaceTransferFunctionEditor::setColorSpace);
    connect(m_ui->cstfeGray, &ColorSpaceTransferFunctionEditor::colorSpaceChanged,
            m_ui->cstfeBlue, &ColorSpaceTransferFunctionEditor::setColorSpace);

    connect(m_ui->cstfeGray, &ColorSpaceTransferFunctionEditor::colorSpaceChanged,
    this, [this](QColorSpace const &spc) {
        m_resultColorSpace = spc;
    });

    connect(m_ui->cstfeGray, &ColorSpaceTransferFunctionEditor::colorSpaceChanged,
            m_ui->wdgColorSpaceInformationResult, &ColorSpaceInformationDisplay::setColorSpace);


    connect(m_ui->cstfeRed, &ColorSpaceTransferFunctionEditor::colorSpaceChanged,
    this,[this](QColorSpace const &spc) {
        QList<uint16_t> spc_red, spc_green, spc_blue;
        QList<uint16_t> res_red, res_green, res_blue;
        extractColorSpaceTransferTables(spc, spc_red, spc_green, spc_blue);
        extractColorSpaceTransferTables(m_resultColorSpace, res_red, res_green, res_blue);
        m_resultColorSpace.setTransferFunctions(spc_red, res_green, res_blue);
        m_ui->cstfeGray->setColorSpace(m_resultColorSpace);
        m_ui->cawTransferFuncGray->setTargetDisplayColorSpace(m_resultColorSpace);
    });
    connect(m_ui->cstfeGreen, &ColorSpaceTransferFunctionEditor::colorSpaceChanged,
    this,[this](QColorSpace const &spc) {
        QList<uint16_t> spc_red, spc_green, spc_blue;
        QList<uint16_t> res_red, res_green, res_blue;
        extractColorSpaceTransferTables(spc, spc_red, spc_green, spc_blue);
        extractColorSpaceTransferTables(m_resultColorSpace, res_red, res_green, res_blue);
        m_resultColorSpace.setTransferFunctions(res_red, spc_green, res_blue);
        m_ui->cstfeGray->setColorSpace(m_resultColorSpace);
        m_ui->cawTransferFuncGray->setTargetDisplayColorSpace(m_resultColorSpace);
    });
    connect(m_ui->cstfeBlue, &ColorSpaceTransferFunctionEditor::colorSpaceChanged,
    this,[this](QColorSpace const &spc) {
        QList<uint16_t> spc_red, spc_green, spc_blue;
        QList<uint16_t> res_red, res_green, res_blue;
        extractColorSpaceTransferTables(spc, spc_red, spc_green, spc_blue);
        extractColorSpaceTransferTables(m_resultColorSpace, res_red, res_green, res_blue);
        m_resultColorSpace.setTransferFunctions(res_red, res_green, spc_blue);
        m_ui->cstfeGray->setColorSpace(m_resultColorSpace);
        m_ui->cawTransferFuncGray->setTargetDisplayColorSpace(m_resultColorSpace);
        m_ui->wdgColorSpaceInformationResult->setColorSpace(m_resultColorSpace);
    });

    connect(m_ui->lneICCProfilePath, &QLineEdit::editingFinished,
    this, [this]() {
        QString path = m_ui->lneICCProfilePath->text();
        if(path.isEmpty()){
            return;
        }
        QFile f(path);
        if(!f.open(QIODevice::ReadOnly)) {
            return;
        }
        QColorSpace space = QColorSpace::fromIccProfile(f.readAll());

        m_ui->lneProfileType->setText(tr("ICC"));
        m_ui->lneProfileSource->setText(tr("User provided"));

        updateColorSpace(space);
    });

    connect(m_ui->tlbOpenICCProfile, &QAbstractButton::clicked,
    this, [this]() {
        QString basedir;
        if(!m_ui->lneICCProfilePath->text().isEmpty()) {
            QFileInfo fi(m_ui->lneICCProfilePath->text());
            basedir = fi.absolutePath();
        }
        QString path = QFileDialog::getOpenFileName(this, "Open ICC Profile", basedir, tr("ICC Profile (*.icc *.icm)"));

        if(path.isEmpty()){
            return;
        }
        QFile f(path);
        if(!f.open(QIODevice::ReadOnly)) {
            return;
        }
        QColorSpace space = QColorSpace::fromIccProfile(f.readAll());

        m_ui->lneICCProfilePath->setText(path);
        m_ui->lneProfileType->setText(tr("ICC"));
        m_ui->lneProfileSource->setText(tr("User provided"));

        updateColorSpace(space);
    });

    connect(m_ui->psbSaveProfile, &QAbstractButton::clicked,
    this, [this]() {
        QString basedir;
        if(!m_ui->lneICCProfilePath->text().isEmpty()) {
            QFileInfo fi(m_ui->lneICCProfilePath->text());
            basedir = fi.absolutePath();
        }
        QString path = QFileDialog::getSaveFileName(this, "Save ICC Profile", basedir, tr("ICC Profile (*.icc *.icm)"));

        if(path.isEmpty()){
            return;
        }
        QFile f(path);
        if(!f.open(QIODevice::WriteOnly)) {
            return;
        }
        f.write(m_resultColorSpace.iccProfile());
    });
}

visual_icc::~visual_icc() = default;

void visual_icc::updateColorSpace(QColorSpace const &space) {
    m_resultColorSpace = space;

    m_ui->wdgColorSpaceInformation->setColorSpace(space);
    m_ui->wdgColorSpaceInformationResult->setColorSpace(space);
    m_ui->wdgColorSpaceInformationResult->setSecondaryColorSpace(space);

    m_ui->cawIntro1->setActiveDisplayColorSpace(space);
    m_ui->cawIntro1->setTargetDisplayColorSpace(space);

    m_ui->cawIntro2->setActiveDisplayColorSpace(space);
    m_ui->cawIntro2->setTargetDisplayColorSpace(space);

    m_ui->cawIntro3->setActiveDisplayColorSpace(space);
    m_ui->cawIntro3->setTargetDisplayColorSpace(space);

    m_ui->cawIntro4->setActiveDisplayColorSpace(space);
    m_ui->cawIntro4->setTargetDisplayColorSpace(space);

    m_ui->cawTransferFuncGray->setActiveDisplayColorSpace(space);
    m_ui->cawTransferFuncGray->setTargetDisplayColorSpace(space);

    m_ui->cstfeGray->setColorSpace(space);

    m_ui->cawTransferFuncRed->setActiveDisplayColorSpace(space);
    m_ui->cawTransferFuncRed->setTargetDisplayColorSpace(space);

    m_ui->cstfeRed->setColorSpace(space);

    m_ui->cawTransferFuncGreen->setActiveDisplayColorSpace(space);
    m_ui->cawTransferFuncGreen->setTargetDisplayColorSpace(space);

    m_ui->cstfeGreen->setColorSpace(space);

    m_ui->cawTransferFuncBlue->setActiveDisplayColorSpace(space);
    m_ui->cawTransferFuncBlue->setTargetDisplayColorSpace(space);

    m_ui->cstfeBlue->setColorSpace(space);
}


#include "moc_visual-icc.cpp"

