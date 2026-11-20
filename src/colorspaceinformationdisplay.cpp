#include "colorspaceinformationdisplay.h"
#include "ui_colorspaceinformationdisplay.h"
#include "fix-colorspace.h"
#include "extract_colorspace_transfer_tables.h"

/** Calculates the sum of the square errors between the given transfer table and gamma value
 *
 * @param transferTable the transfer table to compare against
 * @param gamma The gamma value to use
 * @return the sum of square errors between the two functions, sampled at the points of the transferTable
 */
static float gammaSquareError(QList<uint16_t> const &transferTable, float gamma) {
    int sz = transferTable.size();
    float sum = 0;
    for(int i = 0; i < sz; i++) {
        float y1 = transferTable[i] / 65535.0;
        float x = i * 1.0f / (sz-1);
        float y2 = pow(x,gamma);
        sum += pow(y1-y2, 2);
    }
    return sum;
}

/** Estimates a gamma value from a transfer function table
 *
 * @param transferTable the transfer function table
 * @return the estimated gamma value
 */
static float estimateGamma(QList<uint16_t> const &transferTable) {
    /*the gamma function is:
     y = x^gamma, mapping 0..1 to 0..1
    we want to minimize:
    \sum (x^gamma - table(x))^2
    = \sum x^(2*gamma) - 2*x^gamma*table(x) + table(x)*table(x)

    first derivative:
    \sum 2*x^gamma*x^gamma*log(x) - 2*x^gamma*table(x)*log(x)


    looking for gamma with
    \sum 2*x^gamma*x^gamma*log(x) - 2*x^gamma*table(x)*log(x) = 0

    \sum x^gamma*x^gamma*log(x) = \sum x^gamma*table(x)*log(x)

     */
    float gamma = 1.0;
    float eps = 0.001;
    while(true) {
        float sum1 = gammaSquareError(transferTable, gamma);
        float sum2 = gammaSquareError(transferTable, gamma+eps);
        float gradient = (sum2 - sum1) / eps;
        if(fabs(gradient) < 0.001) {
            break;
        }
        gamma -= gradient * 0.02;
    }

    return gamma;
}

ColorSpaceInformationDisplay::ColorSpaceInformationDisplay(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f),
      m_ui(new Ui::ColorSpaceInformationDisplay)
{
    m_ui->setupUi(this);
}

ColorSpaceInformationDisplay::~ColorSpaceInformationDisplay() =default;

void ColorSpaceInformationDisplay::setColorSpace(QColorSpace const &space) {
    m_ui->wdgRender->setColorSpace(space);
    auto pp = QColorSpace_primaryPoints(space);

    m_ui->lneRedX->setText(QStringLiteral("%1").arg(pp.redPoint.x()));
    m_ui->lneRedY->setText(QStringLiteral("%1").arg(pp.redPoint.y()));
    m_ui->lneGreenX->setText(QStringLiteral("%1").arg(pp.greenPoint.x()));
    m_ui->lneGreenY->setText(QStringLiteral("%1").arg(pp.greenPoint.y()));
    m_ui->lneBlueX->setText(QStringLiteral("%1").arg(pp.bluePoint.x()));
    m_ui->lneBlueY->setText(QStringLiteral("%1").arg(pp.bluePoint.y()));
    m_ui->lneWhiteX->setText(QStringLiteral("%1").arg(pp.whitePoint.x()));
    m_ui->lneWhiteY->setText(QStringLiteral("%1").arg(pp.whitePoint.y()));
    if(space.gamma() == 0.0) {
        QList<uint16_t> redTransferFunctionTable;
        QList<uint16_t> greenTransferFunctionTable;
        QList<uint16_t> blueTransferFunctionTable;

        extractColorSpaceTransferTables(space,
                                        redTransferFunctionTable,
                                        greenTransferFunctionTable,
                                        blueTransferFunctionTable);

        float gamma_r = estimateGamma(redTransferFunctionTable);
        float gamma_g = estimateGamma(greenTransferFunctionTable);
        float gamma_b = estimateGamma(blueTransferFunctionTable);

        m_ui->lneGamma->setText(QStringLiteral("%1 %2 %3").arg(gamma_r).arg(gamma_g).arg(gamma_b));
    } else {
        m_ui->lneGamma->setText(QStringLiteral("%1").arg(space.gamma()));
    }
}

void ColorSpaceInformationDisplay::setSecondaryColorSpace(QColorSpace const &space) {
    m_ui->wdgRender->setSecondaryColorSpace(space);
}
