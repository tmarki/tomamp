#ifndef OPTIONDIALOG_H
#define OPTIONDIALOG_H

#include <QDialog>
#include <QSettings>

class OptionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OptionDialog(QWidget *parent, QSettings& set);

signals:

public slots:
private:
    void    setupUi ();
    QSettings& settings;
};

#endif // OPTIONDIALOG_H
