#include "colorspacetransferfunctioneditor.h"

#include <QSlider>
#include <QLabel>
#include <QBoxLayout>
#include "extract_colorspace_transfer_tables.h"

/** Invert the transfer function @a tab
 *
 * This samples the transfer function @a tab for as many times it is sized,
 * doing linear interpolation between the origin points.
 * This assumes the @a tab is invertible.
 *
 * @param tab The original transfer function
 * @return The inverted transfer function
 */
QList<uint16_t> invertTransferFunction(QList<uint16_t> const &tab) {
    QList<uint16_t> result;
    result.resize(tab.size());
    int tab_idx = 0;
    result.front() = 0;
    for(int i = 1; i < result.size()-1; i++) {
        double x = i * 1.0 / (result.size() - 1);
        //look for the value in tab that is closest to x. we can just start
        //our search at tab_idx and advance from that.
        while(tab_idx < tab.size()-2) {
            double x1 = tab[tab_idx] / 65535.0;
            double x2 = tab[tab_idx+1] / 65535.0;
            if(x2 < x) {
                tab_idx++;
                continue;
            }
            if(x1 > x) {
                break;
            }
            break;
        }
        double x1 = tab[tab_idx] / 65535.0;
        double x2 = tab[tab_idx+1] / 65535.0;
        double y1 = tab_idx * 1.0 / (tab.size() - 1);
        double y2 = (tab_idx+1) * 1.0 / (tab.size() - 1);
        result[i] = ((x2 - x) * y1 + (x - x1) * y2)/(x2-x1) * 65535.0;
    }
    result.back() = 65535;
    return result;
}

/** Fixes the transfer function @a table so it is monotonous rising
 *
 * This should not really be needed, but these are integer values at their
 * core, so they might get too close to resolve, and adjacent elements cannot
 * have the same number, else the color space will become invalid.
 *
 * @param table Reference to the transfer function to be fixed
 */
static void fixupTransferFunctionTable(QList<uint16_t> &table) {
    while(true) {
        bool good = true;
        for(int pos = 1; pos < table.size(); pos++) {
            if(table[pos] <= table[pos-1]) {
                good = false;
                uint16_t val = (table[pos]+table[pos-1]) / 2;
                if(val == 65535) {
                    val = 65534;
                }
                table[pos-1] = val;
                table[pos] = val+1;
            }
        }
        if(good) {
            break;
        }
    }
}

ColorSpaceTransferFunctionEditor::ColorSpaceTransferFunctionEditor(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f) {
    m_horzLayout = new QHBoxLayout(this);
}

ColorSpaceTransferFunctionEditor::~ColorSpaceTransferFunctionEditor() {
}

void ColorSpaceTransferFunctionEditor::setColorSpace(QColorSpace const &space) {
    m_colorSpace = space;
    extractColorSpaceTransferTables(space,
                                    m_redTransferFunctionTable,
                                    m_greenTransferFunctionTable,
                                    m_blueTransferFunctionTable);

    m_redTransferFunctionTable = invertTransferFunction(m_redTransferFunctionTable);
    m_greenTransferFunctionTable = invertTransferFunction(m_greenTransferFunctionTable);
    m_blueTransferFunctionTable = invertTransferFunction(m_blueTransferFunctionTable);

    m_originalRedTransferFunctionTable = m_redTransferFunctionTable;
    m_originalGreenTransferFunctionTable = m_greenTransferFunctionTable;
    m_originalBlueTransferFunctionTable = m_blueTransferFunctionTable;

    m_updatingColorSpace = true;

    for(auto &pi : m_positionInfos) {
        double val = getTransferFunctionValueAt(pi.first);
        pi.second.lblBottom->setText(tr("%1").arg(val, 0, 'g', 2));

        auto it = m_positionInfos.find(pi.first);
        if(it != m_positionInfos.begin()) {
            auto prev_it = it;
            prev_it--;
            unsigned distance = pi.first - prev_it->first;
            prev_it->second.sld->setMaximum(val*65535-distance-1);
            pi.second.sld->setMinimum(prev_it->second.sld->value()+distance+1);
        } else {
            pi.second.sld->setMinimum(pi.first);
        }
        auto next_it = it;
        next_it++;
        if(next_it != m_positionInfos.end()) {
            unsigned distance = next_it->first - pi.first;
            next_it->second.sld->setMinimum(val*65535+distance+1);
            pi.second.sld->setMaximum(next_it->second.sld->value()-distance-1);
        } else {
            pi.second.sld->setMaximum(65536 - pi.first + 256);
        }
        pi.second.sld->setValue(val * 65535);
    }
    m_updatingColorSpace = false;
}

void ColorSpaceTransferFunctionEditor::setActiveColorComponents(bool red, bool green, bool blue) {
    if(!red && !green && !blue) {
        red = true;
        green = true;
        blue = true;
    }
    m_redActive = red;
    m_greenActive = green;
    m_blueActive = blue;
}

void ColorSpaceTransferFunctionEditor::addPosition(uint8_t sourcePosition) {
    PositionInfo pi;
    pi.sourcePosition = sourcePosition;;
    pi.sld = new QSlider();
    pi.lblTop = new QLabel();
    pi.lblBottom = new QLabel();
    pi.vertLayout = new QVBoxLayout();


    pi.sld->setOrientation(Qt::Orientation::Vertical);

    pi.lblTop->setText(tr("%1").arg(sourcePosition));
    double val = getTransferFunctionValueAt(sourcePosition);
    pi.lblBottom->setText(tr("%1").arg(val, 0, 'g', 2));

    pi.vertLayout->addWidget(pi.lblTop);
    pi.vertLayout->addWidget(pi.sld);
    pi.vertLayout->addWidget(pi.lblBottom);
    m_positionInfos[sourcePosition] = pi;
    m_horzLayout->addLayout(pi.vertLayout);

    auto it = m_positionInfos.find(sourcePosition);
    if(it != m_positionInfos.begin()) {
        auto prev_it = it;
        prev_it--;
        unsigned distance = sourcePosition - prev_it->first;
        prev_it->second.sld->setMaximum(val*65535-distance-1);
        pi.sld->setMinimum(prev_it->second.sld->value()+distance+1);
    } else {
        pi.sld->setMinimum(sourcePosition);
    }
    auto next_it = it;
    next_it++;
    if(next_it != m_positionInfos.end()) {
        unsigned distance = next_it->first - sourcePosition;
        next_it->second.sld->setMinimum(val*65535+distance+1);
        pi.sld->setMaximum(next_it->second.sld->value()-distance-1);
    } else {
        pi.sld->setMaximum(65536 - sourcePosition + 256);
    }
    pi.sld->setValue(val * 65535);

    connect(pi.sld, &QSlider::valueChanged,
    this, [this, sourcePosition](int value) {
        if(m_updatingColorSpace) {
            return;
        }
        auto it = m_positionInfos.find(sourcePosition);
        if(it == m_positionInfos.end()) {
            return;
        }
        auto next_it = it;
        next_it++;
        auto prev_it = it;
        prev_it--;
        if(next_it != m_positionInfos.end()) {
            unsigned distance = next_it->first - sourcePosition;
            next_it->second.sld->setMinimum(value+distance+1);
        }
        if(prev_it != m_positionInfos.end()) {
            unsigned distance = sourcePosition - prev_it->first;
            prev_it->second.sld->setMaximum(value-distance-1);
        }
        it->second.lblBottom->setText(tr("%1").arg(value / 65535.0, 0, 'g', 2));
        updateTransferFunctions();
        emit colorSpaceChanged(m_colorSpace);
    });
}

double ColorSpaceTransferFunctionEditor::getTransferFunctionValueAt(uint8_t position) {
    unsigned count = 0;
    unsigned sum = 0;
    if(m_redActive) {
        sum += m_redTransferFunctionTable[position];
        count++;
    }
    if(m_greenActive) {
        sum += m_greenTransferFunctionTable[position];
        count++;
    }
    if(m_blueActive) {
        sum += m_blueTransferFunctionTable[position];
        count++;
    }
    return sum / 65535.0 / count;
}

void ColorSpaceTransferFunctionEditor::updateTransferFunctions() {
    m_redTransferFunctionTable = m_originalRedTransferFunctionTable;
    m_greenTransferFunctionTable = m_originalGreenTransferFunctionTable;
    m_blueTransferFunctionTable = m_originalBlueTransferFunctionTable;

    uint8_t before_pos = 0;
    for(auto it = m_positionInfos.begin(); it != m_positionInfos.end();) {
        uint8_t current_pos = it->first;
        double value = it->second.sld->value() / 65535.0;
         it++;
         uint8_t next_pos = (it == m_positionInfos.end())?255:it->first;


        unsigned count = 0;
        unsigned sum = 0;
        unsigned red_comp = m_redTransferFunctionTable[current_pos];
        unsigned green_comp = m_greenTransferFunctionTable[current_pos];
        unsigned blue_comp = m_blueTransferFunctionTable[current_pos];
        if(m_redActive) {
            sum += red_comp;
            count++;
        }
        if(m_greenActive) {
            sum += green_comp;
            count++;
        }
        if(m_blueActive) {
            sum += blue_comp;
            count++;
        }
        double old_value = sum / 65535.0 / count;
        double value_factor = value / old_value;
        double red_change = red_comp * (value_factor - 1.0);
        double green_change = green_comp * (value_factor - 1.0);
        double blue_change = blue_comp * (value_factor - 1.0);
        for(uint8_t pos = before_pos+1; pos <= current_pos; pos++) {
            if(m_redActive) {
                m_redTransferFunctionTable[pos] += round(red_change * (pos - before_pos) / (current_pos - before_pos));
            }
            if(m_greenActive) {
                m_greenTransferFunctionTable[pos] += round(green_change * (pos - before_pos) / (current_pos - before_pos));
            }
            if(m_blueActive) {
                m_blueTransferFunctionTable[pos] += round(blue_change * (pos - before_pos) / (current_pos - before_pos));
            }
        }
        for(uint8_t pos = current_pos+1; pos < next_pos; pos++) {
            if(m_redActive) {
                m_redTransferFunctionTable[pos] += round(red_change * (next_pos - pos) / (next_pos - current_pos));
            }
            if(m_greenActive) {
                m_greenTransferFunctionTable[pos] += round(green_change * (next_pos - pos) / (next_pos - current_pos));
            }
            if(m_blueActive) {
                m_blueTransferFunctionTable[pos] += round(blue_change * (next_pos - pos) / (next_pos - current_pos));
            }
        }
        if(m_redActive) {
            fixupTransferFunctionTable(m_redTransferFunctionTable);
        }
        if(m_greenActive) {
            fixupTransferFunctionTable(m_greenTransferFunctionTable);
        }
        if(m_blueActive) {
            fixupTransferFunctionTable(m_blueTransferFunctionTable);
        }
        m_colorSpace.setTransferFunctions(invertTransferFunction(m_redTransferFunctionTable),
                                          invertTransferFunction(m_greenTransferFunctionTable),
                                          invertTransferFunction(m_blueTransferFunctionTable));
    }
}

void ColorSpaceTransferFunctionEditor::clearPositions() {
    for(auto pi : m_positionInfos) {
        pi.second.vertLayout->deleteLater();
    }
    m_positionInfos.clear();
}

