
#include "visual-icc.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    visual_icc w;
    w.show();

    return app.exec();
}

