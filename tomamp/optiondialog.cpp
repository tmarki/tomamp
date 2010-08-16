#include <QtGui>
#include "optiondialog.h"

OptionDialog::OptionDialog(QWidget *parent, QSettings& set) :
    QDialog(parent), settings (set)
{
    setupUi();
}


void OptionDialog::setupUi()
{
    QHBoxLayout *mainLayout = new QHBoxLayout;
//    QWidget *widget = new QWidget;
//    widget->setLayout(mainLayout);

//    setCentralWidget(widget);
    setLayout(mainLayout);
    setWindowTitle("TomAmp Settings");
}

