/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
***************************************************************************/

#include <QtGui>
#include <QtDebug>
#include <QInputDialog>

#include "mainwindow.h"

#define AVOID_INPUT_DIALOG 1

MainWindow::MainWindow()
    : settings (tr ("TomAmp"), "TomAmp")
{
    audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
    mediaObject = new Phonon::MediaObject(this);
    metaInformationResolver = new Phonon::MediaObject(this);

    mediaObject->setTickInterval(500);
    connect(mediaObject, SIGNAL(tick(qint64)), this, SLOT(tick(qint64)));
    connect(mediaObject, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
    this, SLOT(stateChanged(Phonon::State,Phonon::State)));
    connect(metaInformationResolver, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
    this, SLOT(metaStateChanged(Phonon::State,Phonon::State)));
    connect(mediaObject, SIGNAL(currentSourceChanged(Phonon::MediaSource)),
    this, SLOT(sourceChanged(Phonon::MediaSource)));
    connect(mediaObject, SIGNAL(aboutToFinish()), this, SLOT(aboutToFinish()));

    Phonon::createPath(mediaObject, audioOutput);

    repeat = settings.value("repeat", false).toBool();
    shuffle = settings.value("shuffle", false).toBool();
    qsrand (time (NULL));
    setupShuffleList();
    setupActions();
    setupMenus();
    setupUi();
    timeLcd->display("00:00");
    addStringList(settings.value("lastPlaylist").toStringList());
}

MainWindow::~MainWindow()
{
    settings.setValue("shuffle", shuffle);
    QStringList curList;
    foreach (Phonon::MediaSource src, sources)
    {
        curList.append(src.fileName());
    }
    settings.setValue("lastPlaylist", curList);
}

void MainWindow::addFiles()
{
    QString folder = settings.value("LastFolder").toString();
    if (folder.isEmpty())
        folder = QDesktopServices::storageLocation(QDesktopServices::MusicLocation);
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Select Files To Add"),
                    folder);

    if (files.isEmpty())
        return;

    QString dir = QFileInfo (files[0]).absoluteDir().absolutePath();
    settings.setValue("LastFolder", dir);
    int index = sources.size();
    foreach (QString string, files)
    {
        Phonon::MediaSource source (string);
        sources.append(source);
    }
    if (!sources.isEmpty())
        metaInformationResolver->setCurrentSource(sources.at(index));
    setupShuffleList();

}

void MainWindow::addFolders()
{
    QString folder = settings.value("LastFolder").toString();
    if (folder.isEmpty())
        folder = QDesktopServices::storageLocation(QDesktopServices::MusicLocation);
    QString dir = QFileDialog::getExistingDirectory(this,
            tr("Select Directory To Add"),
            folder);

    QStringList filters;
    filters << "*.mp3";

    QStringList files = QDir (dir).entryList(filters);

    if (files.isEmpty())
        return;

    settings.setValue("LastFolder", dir);
    int index = sources.size();
    foreach (QString string, files)
    {
        QString fname = dir + "/" + string;
        qDebug () << fname;
        Phonon::MediaSource source(fname);
        sources.append(source);
    }
    if (!sources.isEmpty())
        metaInformationResolver->setCurrentSource(sources.at(index));
    setupShuffleList();

}

void MainWindow::addUrl()
{
#ifdef AVOID_INPUT_DIALOG
    QString url = "http://war.str3am.com:7970";
#else
    QString url = QInputDialog::getText(this, "Get URL", "Please type in the stream URL");
#endif
    int index = sources.size();
    if (!url.isEmpty())
    {
        Phonon::MediaSource source(url);
        sources.append(source);
    }
    if (!sources.isEmpty())
        metaInformationResolver->setCurrentSource(sources.at(index));
    setupShuffleList();
}

void MainWindow::addStringList(const QStringList& list)
{
    int index = sources.size();
    foreach (QString string, list)
    {
        Phonon::MediaSource source(string);
        sources.append(source);
    }
    if (!sources.isEmpty())
        metaInformationResolver->setCurrentSource(sources.at(index));
    setupShuffleList();
}

void MainWindow::about()
{
    QMessageBox::information(this, tr("About Music Player"),
        tr("The Music Player example shows how to use Phonon - the multimedia"
        " framework that comes with Qt - to create a simple music player."));
}

void MainWindow::stateChanged(Phonon::State newState, Phonon::State /* oldState */)
{
    qDebug () << "State: " << newState;
    switch (newState)
    {
        case Phonon::ErrorState:
            if (mediaObject->errorType() == Phonon::FatalError)
            {
                QMessageBox::warning(this, tr("Fatal Error"),
                mediaObject->errorString());
            }
            else
            {
                QMessageBox::warning(this, tr("Error"),
                mediaObject->errorString());
            }
            break;
        case Phonon::PlayingState:
            setWindowTitle(metaInformationResolver->metaData().value("TITLE") + " - TomAmp");
            playAction->setEnabled(false);
            pauseAction->setEnabled(true);
            stopAction->setEnabled(true);
            break;
        case Phonon::StoppedState:
            stopAction->setEnabled(false);
            playAction->setEnabled(true);
            pauseAction->setEnabled(false);
            timeLcd->display("00:00");
            break;
        case Phonon::PausedState:
            pauseAction->setEnabled(false);
            stopAction->setEnabled(true);
            playAction->setEnabled(true);
            qDebug () << "Queue size: " << mediaObject->queue().size ();
            if (mediaObject->queue().size ())
            {
                mediaObject->setCurrentSource(mediaObject->queue()[0]);
                musicTable->selectRow(sources.indexOf(mediaObject->currentSource()));
                mediaObject->play();
            }
            mediaObject->clearQueue();
            break;
        case Phonon::BufferingState:
            qDebug () << "Buffering";
            break;
        default:
        ;
    }
}

void MainWindow::tick(qint64 time)
{
    QTime displayTime(0, (time / 60000) % 60, (time / 1000) % 60);

    timeLcd->display(displayTime.toString("mm:ss"));
}

void MainWindow::tableClicked(int row, int /* column */)
{
//    bool wasPlaying = mediaObject->state() == Phonon::PlayingState;

    mediaObject->stop();
    mediaObject->clearQueue();

    if (row >= sources.size())
        return;

    mediaObject->setCurrentSource(sources[row]);

    mediaObject->play();
    int ind = shuffleList.indexOf(row);
    shuffleList.removeAt(ind);
    shuffleList.insert(0, row);
    qDebug () << "Modified shuffle list: " << shuffleList;
}

void MainWindow::sourceChanged(const Phonon::MediaSource &source)
{
    musicTable->selectRow(sources.indexOf(source));
    timeLcd->display("00:00");
}

void MainWindow::metaStateChanged(Phonon::State newState, Phonon::State /* oldState */)
{
    if (newState == Phonon::ErrorState)
    {
        QMessageBox::warning(this, tr("Error opening files"),
        metaInformationResolver->errorString());
        while (!sources.isEmpty() &&
            !(sources.takeLast() == metaInformationResolver->currentSource())) {}  /* loop */;
        return;
    }

    if (newState != Phonon::StoppedState && newState != Phonon::PausedState)
    {
        return;
    }

    if (metaInformationResolver->currentSource().type() == Phonon::MediaSource::Invalid)
        return;

    QMap<QString, QString> metaData = metaInformationResolver->metaData();

    QString title = metaData.value("TITLE");
    if (title == "")
        title = metaInformationResolver->currentSource().fileName();

    if (title == "")
        title = metaInformationResolver->currentSource().url().toString();

    QTableWidgetItem *titleItem = new QTableWidgetItem(title);
    titleItem->setFlags(titleItem->flags() ^ Qt::ItemIsEditable);
    QTableWidgetItem *artistItem = new QTableWidgetItem(metaData.value("ARTIST"));
    artistItem->setFlags(artistItem->flags() ^ Qt::ItemIsEditable);
    QTableWidgetItem *albumItem = new QTableWidgetItem(metaData.value("ALBUM"));
    albumItem->setFlags(albumItem->flags() ^ Qt::ItemIsEditable);
    QTableWidgetItem *yearItem = new QTableWidgetItem(metaData.value("DATE"));
    yearItem->setFlags(yearItem->flags() ^ Qt::ItemIsEditable);

    int currentRow = musicTable->rowCount();
    musicTable->insertRow(currentRow);
    musicTable->setItem(currentRow, 0, artistItem);
    musicTable->setItem(currentRow, 1, titleItem);
    musicTable->setItem(currentRow, 2, albumItem);
    musicTable->setItem(currentRow, 3, yearItem);


    if (musicTable->selectedItems().isEmpty())
    {
        musicTable->selectRow(0);
        mediaObject->setCurrentSource(metaInformationResolver->currentSource());
    }

    Phonon::MediaSource source = metaInformationResolver->currentSource();
    int index = sources.indexOf(metaInformationResolver->currentSource()) + 1;
    if (sources.size() > index)
    {
        metaInformationResolver->setCurrentSource(sources.at(index));
    }
    else
    {
        musicTable->resizeColumnsToContents();
        if (musicTable->columnWidth(0) > 300)
            musicTable->setColumnWidth(0, 300);
    }
}

void MainWindow::aboutToFinish()
{
    qDebug () << "Abouttotfinish";
    int index = sources.indexOf(mediaObject->currentSource()) + 1;
    if (shuffle)
    {
        index = shuffleList.indexOf(sources.indexOf(mediaObject->currentSource())) + 1;
        if (index < shuffleList.size ())
        {
            mediaObject->enqueue(sources.at (shuffleList[index]));
        }
        else if (repeat)
        {
            mediaObject->enqueue(sources.at (shuffleList[0]));
        }

    }
    else
    {
        if (sources.size() > index)
        {
            mediaObject->enqueue(sources.at(index));
            qDebug () << "Enqueue " << index << " pfm " << mediaObject->prefinishMark();
        }
        else if (repeat)
        {
            mediaObject->enqueue(sources.at(0));
            qDebug () << "Enqueue " << 0 << " pfm " << mediaObject->prefinishMark();
        }
    }
}

void MainWindow::finished()
{
    qDebug () << "Finished";
}

void MainWindow::setupActions()
{
    playAction = new QAction(style()->standardIcon(QStyle::SP_MediaPlay), tr("Play"), this);
    playAction->setShortcut(tr("Crl+P"));
    playAction->setDisabled(true);
    pauseAction = new QAction(style()->standardIcon(QStyle::SP_MediaPause), tr("Pause"), this);
    pauseAction->setShortcut(tr("Ctrl+A"));
    pauseAction->setDisabled(true);
    stopAction = new QAction(style()->standardIcon(QStyle::SP_MediaStop), tr("Stop"), this);
    stopAction->setShortcut(tr("Ctrl+S"));
    stopAction->setDisabled(true);
    nextAction = new QAction(style()->standardIcon(QStyle::SP_MediaSkipForward), tr("Next"), this);
    nextAction->setShortcut(tr("Ctrl+N"));
    previousAction = new QAction(style()->standardIcon(QStyle::SP_MediaSkipBackward), tr("Previous"), this);
    previousAction->setShortcut(tr("Ctrl+R"));
    repeatAction = new QAction(style()->standardIcon(QStyle::SP_BrowserReload), tr("Repeat"), this);
    repeatAction->setCheckable(true);
    repeatAction->setChecked(repeat);
    repeatAction->setShortcut(tr("Ctrl+I"));
    shuffleAction = new QAction(style()->standardIcon(QStyle::SP_TrashIcon), tr("S&huffle"), this);
    shuffleAction->setCheckable(true);
    shuffleAction->setChecked(shuffle);
    shuffleAction->setShortcut(tr("Ctrl+H"));
    addFilesAction = new QAction(tr("Add &File"), this);
    addFilesAction->setShortcut(tr("Ctrl+F"));
    addFoldersAction = new QAction(tr("Add F&older"), this);
    addFoldersAction->setShortcut(tr("Ctrl+O"));
    addUrlAction = new QAction(tr("Add &Url"), this);
    addUrlAction->setShortcut(tr("Ctrl+U"));
    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcuts(QKeySequence::Quit);
    aboutAction = new QAction(tr("A&bout"), this);
    aboutAction->setShortcut(tr("Ctrl+B"));
    aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutQtAction->setShortcut(tr("Ctrl+Q"));

    connect(playAction, SIGNAL(triggered()), mediaObject, SLOT(play()));
    connect(pauseAction, SIGNAL(triggered()), mediaObject, SLOT(pause()) );
    connect(stopAction, SIGNAL(triggered()), mediaObject, SLOT(stop()));
    connect(repeatAction, SIGNAL(triggered()), this, SLOT(repeatToggle()));
    connect(shuffleAction, SIGNAL(triggered()), this, SLOT(shuffleToggle()));
    connect(addFilesAction, SIGNAL(triggered()), this, SLOT(addFiles()));
    connect(addFoldersAction, SIGNAL(triggered()), this, SLOT(addFolders()));
    connect(addUrlAction, SIGNAL(triggered()), this, SLOT(addUrl()));
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}

void MainWindow::repeatToggle ()
{
    repeat = !repeat;
    qDebug() << "Repeat toggled to " << repeat;
    settings.setValue("repeat", QVariant (repeat));
}

void MainWindow::shuffleToggle ()
{
    shuffle = !shuffle;
    settings.setValue("shuffle", QVariant (shuffle));
}


void MainWindow::setupMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(addFilesAction);
    fileMenu->addAction(addFoldersAction);
    fileMenu->addAction(addUrlAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    QMenu *aboutMenu = menuBar()->addMenu(tr("&Help"));
    aboutMenu->addAction(aboutAction);
    aboutMenu->addAction(aboutQtAction);
}

void MainWindow::setupUi()
{
    QToolBar *bar = new QToolBar;

    bar->setOrientation(Qt::Vertical);
    bar->addAction(playAction);
    bar->addAction(pauseAction);
    bar->addAction(stopAction);
    bar->addAction(repeatAction);
    bar->addAction(shuffleAction);

    seekSlider = new Phonon::SeekSlider(this);
    seekSlider->setMediaObject(mediaObject);

/*    volumeSlider = new Phonon::VolumeSlider(this);
    volumeSlider->setAudioOutput(audioOutput);
    volumeSlider->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    volumeSlider->setOrientation(Qt::Vertical);*/

    QLabel *volumeLabel = new QLabel;
    volumeLabel->setPixmap(QPixmap("images/volume.png"));

/*    QPalette palette;
    palette.setBrush(QPalette::Light, Qt::darkGray);*/

    timeLcd = new QLCDNumber;
//    timeLcd->setPalette(palette);

    QStringList headers;
    headers << tr("Artist") << tr("Title") << tr("Album") << tr("Year");

    musicTable = new QTableWidget(0, 4);
    musicTable->setHorizontalHeaderLabels(headers);
    musicTable->setSelectionMode(QAbstractItemView::SingleSelection);
    musicTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(musicTable, SIGNAL(cellDoubleClicked(int,int)),
        this, SLOT(tableClicked(int,int)));

    QHBoxLayout *seekerLayout = new QHBoxLayout;
    seekerLayout->addWidget(seekSlider);
    seekerLayout->addWidget(timeLcd);

    QVBoxLayout *playbackLayout = new QVBoxLayout;
    playbackLayout->addWidget(bar);
//    playbackLayout->addStretch();
//    playbackLayout->addWidget(volumeSlider);
//    playbackLayout->addWidget(volumeLabel);

    QVBoxLayout *seekAndTableLayout = new QVBoxLayout;

    seekAndTableLayout->addWidget(musicTable);
    seekAndTableLayout->addLayout(seekerLayout);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addLayout(seekAndTableLayout);
    mainLayout->addLayout(playbackLayout);

    QWidget *widget = new QWidget;
    widget->setLayout(mainLayout);

    setCentralWidget(widget);
    setWindowTitle("TomAmp");
//    ui.setupUi(this);
}


void MainWindow::setupShuffleList()
{
    QList<int> tmp;
    for (int i = 0; i < sources.size(); ++i)
        tmp.append(i);
    shuffleList.clear();
    while (tmp.size ())
    {
        int ind = qrand () % tmp.size();
        shuffleList.append(tmp[ind]);
        tmp.removeAt(ind);
    }
    qDebug () << shuffleList;
}
