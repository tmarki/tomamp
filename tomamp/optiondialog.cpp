#include <QtGui>
#include "optiondialog.h"

OptionDialog::OptionDialog(QWidget *parent, QSettings& set) :
    QDialog(parent), settings (set)
{
    availableHeaders = settings.value("headerOrder", QStringList()).toStringList();
    if (!availableHeaders.size())
        availableHeaders << "Artist" << "Title" << "Album" << "Controls";
    setupUi();
}

OptionDialog::~OptionDialog ()
{
#ifdef Q_WS_MAEMO_5
    settings.setValue("orientation", orient->currentText());
#endif
    settings.setValue("headerOrder", availableHeaders);
    QStringList h;
    foreach (QObject* child, children())
    {
        QCheckBox* cb = qobject_cast<QCheckBox*>(child);
        if (cb && cb->isChecked () && availableHeaders.indexOf(cb->text()) >= 0)
        {
            h << cb->text ();
        }
    }
    settings.setValue("headers", h);
}


void OptionDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
#ifdef Q_WS_MAEMO_5
    QString ori = settings.value("orientation", "Automatic").toString();
    orient = new QComboBox();
    orient->addItem(tr ("Automatic"));
    orient->addItem(tr ("Landscape"));
    orient->addItem(tr ("Portrait"));
    if (ori == "Landscape")
        orient->setCurrentIndex(1);
    if (ori == "Portrait")
        orient->setCurrentIndex(2);
    QLabel *label = new QLabel (tr ("Orientation"));
    QHBoxLayout *subLayout = new QHBoxLayout;
    subLayout->addWidget(label);
    subLayout->addWidget(orient);
    mainLayout->addLayout(subLayout);
#endif
    QLabel* lab = new QLabel (tr ("Enabled columns: "));
    QHBoxLayout *sub = new QHBoxLayout;
    sub->addWidget(lab);
    headerLayout = new QVBoxLayout;
    QStringList headers = settings.value ("headers", QStringList ()).toStringList();
    if (!headers.size())
        headers = availableHeaders.mid(0, 3);
    foreach (QString str, availableHeaders)
    {
        QHBoxLayout *cont = new QHBoxLayout;
        QCheckBox* box = new QCheckBox (str);
        if (headers.indexOf(str) >= 0)
            box->setChecked(true);
        QLabel* label = new QLabel;
        label->setText(QString::fromUtf8("<b><a style='text-decoration:none' href='up'>▲</a> <a style='text-decoration:none' href='down'>▼</a></b>"));
        label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        label->setProperty("row", str);
        cont->addWidget(box);
        cont->addWidget(label);
        headerLayout->addLayout(cont);
        connect (label, SIGNAL(linkActivated(QString)), this, SLOT (orderControl(QString)));
    }
    sub->addLayout(headerLayout);
    mainLayout->addLayout(sub);
    QCheckBox *cb = new QCheckBox (tr ("Flip UI controls"));
    cb->setChecked(settings.value("uiflipped", false).toBool());
    connect (cb, SIGNAL(toggled(bool)), this, SLOT(toggleFlip(bool)));
    mainLayout->addWidget(cb);
    setLayout(mainLayout);
    setWindowTitle("Settings");
}

void OptionDialog::toggleFlip (bool val)
{
    settings.setValue("uiflipped", val);
}


void OptionDialog::orderControl (QString link)
{
    QString str = sender ()->property("row").toString();
    qDebug () << "Col action " << link << " on " << str;
    int i = availableHeaders.indexOf(str);
    if (link == "up" && i > 0)
    {
        QString tmp = availableHeaders[i];
        availableHeaders[i] = availableHeaders[i - 1];
        availableHeaders[i - 1] = tmp;
    }
    else if (link == "down" && i < availableHeaders.size() - 1)
    {
        QString tmp = availableHeaders[i];
        availableHeaders[i] = availableHeaders[i + 1];
        availableHeaders[i + 1] = tmp;
    }
    QStringList h;
    foreach (QObject* child, children())
    {
        QCheckBox* cb = qobject_cast<QCheckBox*>(child);
        if (cb && cb->isChecked ())
        {
            h << cb->text ();
        }
    }
    settings.setValue("headers", h);
    delete layout ();
    foreach (QObject* child, children ())
        delete child;
//    update ();
    setupUi();
}


