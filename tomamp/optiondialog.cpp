#include "optiondialog.h"

OptionDialog::OptionDialog(QWidget *parent, QSettings& set) :
    QDialog(parent), settings (set)
{
}
