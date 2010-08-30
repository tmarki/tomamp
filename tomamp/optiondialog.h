#ifndef OPTIONDIALOG_H
#define OPTIONDIALOG_H

#include <QDialog>
#include <QSettings>

class QComboBox;
class QVBoxLayout;

class OptionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OptionDialog(QWidget *parent, QSettings& set);
    ~OptionDialog ();

signals:

public slots:
private slots:
    void orderControl (QString);
private:
    void    setupUi ();
    QSettings& settings;
    QComboBox* orient;
    QStringList availableHeaders;
    QVBoxLayout *headerLayout;
};

#endif // OPTIONDIALOG_H
