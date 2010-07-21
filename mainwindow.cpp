#include <QtGui>
#include <QtDebug>
#include <QInputDialog>

#include "mainwindow.h"
#include "time.h"

//#define AVOID_INPUT_DIALOG 0

MainWindow::MainWindow()
    : plman (this), settings (tr ("TomAmp"), "TomAmp")
{
    audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
    mediaObject = new Phonon::MediaObject(this);

    mediaObject->setTickInterval(1000);
    connect(mediaObject, SIGNAL(tick(qint64)), this, SLOT(tick(qint64)));
    connect(mediaObject, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
        this, SLOT(stateChanged(Phonon::State,Phonon::State)));
    connect(mediaObject, SIGNAL(currentSourceChanged(Phonon::MediaSource)),
        this, SLOT(sourceChanged(Phonon::MediaSource)));
    connect(mediaObject, SIGNAL(aboutToFinish()), this, SLOT(aboutToFinish()));
    connect (&plman, SIGNAL (playlistChanged (int)), this, SLOT (playlistChanged(int)));
    connect (&plman, SIGNAL (itemUpdated(int)), this, SLOT (itemUpdated (int)));

    Phonon::createPath(mediaObject, audioOutput);

    qsrand (time (NULL));
    repeat = settings.value("repeat", false).toBool();
    shuffle = settings.value("shuffle", false).toBool();
    setupShuffleList();
    setupActions();
    setupMenus();
    setupUi();
    timeLcd->display("00:00");
    plman.addStringList(settings.value("lastPlaylist").toStringList());
    setupShuffleList();
    audioOutput->setVolume(settings.value("volume", .5).toReal());
}

MainWindow::~MainWindow()
{
    settings.setValue("shuffle", shuffle);
    settings.setValue("repeat", repeat);
    settings.setValue("lastPlaylist", plman.playlistStrings());
    settings.setValue("volume", audioOutput->volume());
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
    QStringList toadd;
    foreach (QString string, files)
    {
        toadd.append (string);
    }
    plman.addStringList(toadd);
}

void MainWindow::addFolder()
{
    QString folder = settings.value("LastFolder").toString();
    if (folder.isEmpty())
        folder = QDesktopServices::storageLocation(QDesktopServices::MusicLocation);
    QString dir = QFileDialog::getExistingDirectory(this,
            tr("Select Directory To Add"),
            folder);

    QStringList filters;
    QStringList files = QDir (dir).entryList(filters, QDir::AllDirs);
    files.removeAll(".");
    files.removeAll("..");
    qDebug () << files;
    bool recursive = false;
    if (files.size())
        recursive = QMessageBox::question(this, "Add all folders", "Subfolders have been detected, add everything?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
    plman.parseAndAddFolder(dir, recursive);
}


void MainWindow::addUrl()
{
#ifdef AVOID_INPUT_DIALOG
    QString url = "http://war.str3am.com:7970";
#else
    QString url = QInputDialog::getText(this, "Get URL", "Please type in the stream URL");
#endif
    QStringList toadd;
    toadd << url;
    plman.addStringList(toadd);
}


void MainWindow::about()
{
    QMessageBox::information(this, tr("About TomAmp v0.1"),
        tr("TomAmp is a simple playlist-based music player.\n\n"
        "(c) 2010 Tamas Marki <tmarki@gmail.com>\n\n"
        "Please send comments and bug reports to the above e-mail address.\n\n"
        "Icons by deleket (http://www.deleket.com)"));
}

void MainWindow::stateChanged(Phonon::State newState, Phonon::State /* oldState */)
{
    qDebug () << "State: " << newState;
    switch (newState)
    {
        case Phonon::ErrorState:
            if (mediaObject->errorType() == Phonon::FatalError)
            {
//                QMessageBox::warning(this, tr("Fatal Error"),
//                mediaObject->errorString() + mediaObject->currentSource().fileName() + ", " + mediaObject->currentSource().url().toString());
            }
            else
            {
//                QMessageBox::warning(this, tr("Error"),
//                mediaObject->errorString());
            }
//            next ();
            break;
        case Phonon::PlayingState:
            setWindowTitle(mediaObject->metaData().value("TITLE") + " - TomAmp");
            pauseAction->setVisible(true);
            playAction->setVisible (false);
            playAction->setEnabled(false);
            pauseAction->setEnabled(true);
            stopAction->setEnabled(true);
            break;
        case Phonon::StoppedState:
            stopAction->setEnabled(false);
            playAction->setEnabled(true);
            pauseAction->setVisible(false);
            playAction->setVisible(true);
            pauseAction->setEnabled(false);
            timeLcd->display("00:00");
            break;
        case Phonon::PausedState:
            pauseAction->setEnabled(false);
            stopAction->setEnabled(true);
            pauseAction->setVisible(false);
            playAction->setVisible(true);
            playAction->setEnabled(true);
            qDebug () << "Queue size: " << mediaObject->queue().size ();
            if (mediaObject->queue().size ())
            {
                mediaObject->setCurrentSource(mediaObject->queue()[0]);
                musicTable->selectRow(plman.indexOf(mediaObject->currentSource()));
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

void MainWindow::next()
{
    bool wasPlaying = (mediaObject->state () == Phonon::PlayingState);
    if (mediaObject->state () == Phonon::ErrorState)
        wasPlaying = true;
    qDebug () << "Getting index of current playing";
    int index = plman.indexOf(mediaObject->currentSource());
    qDebug () << "Next index is " << index;
    if (shuffle)
    {
        qDebug () << "Shuffle next";
        index = shuffleList.indexOf(plman.indexOf(mediaObject->currentSource()));
        do
        {
            index += 1;
            qDebug () << "Index increase a " << index;
        }
        while (index < shuffleList.size () && !plman.getItem(index).playable);
        qDebug () << "Shuffle next 2 " << index;
        if (index < shuffleList.size ())
        {
            mediaObject->setCurrentSource(plman.at (shuffleList[index]));
        }
        else if (repeat)
        {
            index = 0;
            do
            {
                qDebug () << "Index increase 2a " << index;
                index += 1;
            }
            while (index < shuffleList.size () && !plman.getItem(index).playable);
            if (index < shuffleList.size ())
                mediaObject->setCurrentSource(plman.at (shuffleList[index]));
        }

    }
    else
    {
        qDebug () << "Normal next";
        while ((index + 1) < plman.size ())
        {
            index += 1;
            qDebug () << "Index increase " << index;
            if (plman.getItem(index).playable)
                break;
        }
        qDebug () << "Normal next 2 " << index;
        if (plman.size() > index)
        {
            mediaObject->setCurrentSource(plman.at(index));
        }
        else if (repeat)
        {
            index = 0;
            do
            {
                qDebug () << "Index increase 2 " << index;
                index += 1;
                if (plman.getItem(index).playable)
                    break;
            }
            while ((index + 1) < shuffleList.size ());
            mediaObject->setCurrentSource(plman.at(index));
        }
    }
    musicTable->selectRow (plman.indexOf(mediaObject->currentSource()));
    if (wasPlaying)
        mediaObject->play();
}

void MainWindow::previous()
{
    bool wasPlaying = (mediaObject->state () == Phonon::PlayingState);
    int index = plman.indexOf(mediaObject->currentSource()) - 1;
    if (shuffle)
    {
        index = shuffleList.indexOf(plman.indexOf(mediaObject->currentSource())) - 1;
        if (index >= 0)
        {
            mediaObject->setCurrentSource(plman.at (shuffleList[index]));
        }
        else if (repeat)
        {
            mediaObject->setCurrentSource(plman.at (shuffleList[shuffleList.size() - 1]));
        }

    }
    else
    {
        if (index >= 0)
        {
            mediaObject->setCurrentSource(plman.at(index));
        }
        else if (repeat)
        {
            mediaObject->setCurrentSource(plman.at(plman.size() - 1));
        }
    }
    if (wasPlaying)
        mediaObject->play();

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

    if (row >= plman.size())
        return;

    int index = row;
    while (index < plman.size () && !plman.getItem(index).playable)
    {
        index += 1;
    }
    if (plman.size() > index)
    {
        mediaObject->setCurrentSource(plman.at(index));
        int ind = shuffleList.indexOf(index);
        shuffleList.removeAt(ind);
        shuffleList.insert(0, index);
        qDebug () << "Modified shuffle list: " << shuffleList;
        mediaObject->play();
    }
    else
    {
        next ();
    }

}

void MainWindow::sourceChanged(const Phonon::MediaSource &source)
{
    musicTable->selectRow(plman.indexOf(source));
    timeLcd->display("00:00");
}


void MainWindow::aboutToFinish()
{
    qDebug () << "Abouttotfinish";
    int index = plman.indexOf(mediaObject->currentSource()) + 1;
    if (shuffle)
    {
        index = shuffleList.indexOf(plman.indexOf(mediaObject->currentSource())) + 1;
        if (index < shuffleList.size ())
        {
            mediaObject->enqueue(plman.at (shuffleList[index]));
        }
        else if (repeat)
        {
            mediaObject->enqueue(plman.at (shuffleList[0]));
        }

    }
    else
    {
        if (plman.size() > index)
        {
            mediaObject->enqueue(plman.at(index));
            qDebug () << "Enqueue " << index << " pfm " << mediaObject->prefinishMark();
        }
        else if (repeat)
        {
            mediaObject->enqueue(plman.at(0));
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
    playAction = new QAction(QIcon (QPixmap (":images/play")), tr("Play"), this);
    playAction->setShortcut(tr("Crl+P"));
    playAction->setDisabled(true);
    pauseAction = new QAction(QIcon (QPixmap (":images/pause")), tr("Pause"), this);
    pauseAction->setShortcut(tr("Ctrl+A"));
    pauseAction->setDisabled(true);
    pauseAction->setVisible(false);
    stopAction = new QAction(QIcon (QPixmap (":images/stop")), tr("Stop"), this);
    stopAction->setShortcut(tr("Ctrl+S"));
    stopAction->setDisabled(true);
    nextAction = new QAction(QIcon (QPixmap (":images/next")), tr("Next"), this);
    nextAction->setShortcut(tr("Ctrl+N"));
    previousAction = new QAction(QIcon (QPixmap (":images/previous")), tr("Previous"), this);
    previousAction->setShortcut(tr("Ctrl+R"));
    repeatAction = new QAction(QIcon (QPixmap (":images/repeat")), tr("Repeat"), this);
    repeatAction->setCheckable(true);
    repeatAction->setChecked(repeat);
    repeatAction->setShortcut(tr("Ctrl+I"));
    shuffleAction = new QAction(QIcon (QPixmap (":images/shuffle")), tr("Shuffle"), this);
    shuffleAction->setCheckable(true);
    shuffleAction->setChecked(shuffle);
    shuffleAction->setShortcut(tr("Ctrl+H"));
    volumeAction = new QAction(QIcon (QPixmap (":images/volume")), tr("Volume"), this);
    volumeAction->setCheckable(true);
    volumeAction->setShortcut(tr("Ctrl+V"));
    addFilesAction = new QAction(tr("Add &File"), this);
    addFilesAction->setShortcut(tr("Ctrl+F"));
    addFoldersAction = new QAction(tr("Add F&older"), this);
    addFoldersAction->setShortcut(tr("Ctrl+O"));
    addUrlAction = new QAction(tr("Add &Url"), this);
    addUrlAction->setShortcut(tr("Ctrl+U"));
    savePlaylistAction = new QAction (tr("Sa&ve Playlist"), this);
    savePlaylistAction->setShortcut(tr ("Ctrl+V"));
    loadPlaylistAction = new QAction (tr("&Load Playlist"), this);
    loadPlaylistAction->setShortcut(tr("Ctrl+L"));
    clearPlaylistAction = new QAction (tr("&Clear Playlist"), this);
    clearPlaylistAction->setShortcut(tr("Ctrl+C"));
    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(tr("Ctrl+X"));
    aboutAction = new QAction(tr("A&bout"), this);
    aboutAction->setShortcut(tr("Ctrl+B"));
    aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutQtAction->setShortcut(tr("Ctrl+Q"));

    connect(playAction, SIGNAL(triggered()), mediaObject, SLOT(play()));
    connect(pauseAction, SIGNAL(triggered()), mediaObject, SLOT(pause()) );
    connect(stopAction, SIGNAL(triggered()), mediaObject, SLOT(stop()));
    connect(repeatAction, SIGNAL(triggered()), this, SLOT(repeatToggle()));
    connect(shuffleAction, SIGNAL(triggered()), this, SLOT(shuffleToggle()));
    connect(volumeAction, SIGNAL(triggered()), this, SLOT(volumeToggle()));
    connect(addFilesAction, SIGNAL(triggered()), this, SLOT(addFiles()));
    connect(addFoldersAction, SIGNAL(triggered()), this, SLOT(addFolder()));
    connect(addUrlAction, SIGNAL(triggered()), this, SLOT(addUrl()));
    connect (savePlaylistAction, SIGNAL (triggered()), this, SLOT (savePlaylist()));
    connect (loadPlaylistAction, SIGNAL (triggered()), this, SLOT (loadPlaylist()));
    connect (clearPlaylistAction, SIGNAL (triggered()), &plman, SLOT (clearPlaylist()));
    connect (nextAction, SIGNAL(triggered()), this, SLOT(next()));
    connect (previousAction, SIGNAL(triggered()), this, SLOT(previous()));
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

void MainWindow::volumeToggle ()
{
    qDebug () << "Volumetoggle: " << volumeAction->isChecked();
    volumeSlider->setVisible(volumeAction->isChecked());
}

void MainWindow::play()
{

}


void MainWindow::setupMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(addFilesAction);
    fileMenu->addAction(addFoldersAction);
    fileMenu->addAction(addUrlAction);
    fileMenu->addSeparator();
    fileMenu->addAction(savePlaylistAction);
    fileMenu->addAction(loadPlaylistAction);
    fileMenu->addAction(clearPlaylistAction);
//    fileMenu->addAction(exitAction);

    QMenu *aboutMenu = menuBar()->addMenu(tr("&Help"));
    aboutMenu->addAction(aboutAction);
    aboutMenu->addAction(aboutQtAction);
}

void MainWindow::setupUi()
{
    QToolBar *bar = new QToolBar;

    bar->setOrientation(Qt::Vertical);
    bar->setStyleSheet("padding:7px");
    //bar->addAction(volumeAction);

    seekSlider = new Phonon::SeekSlider(this);
    seekSlider->setMediaObject(mediaObject);

    volumeSlider = new Phonon::VolumeSlider(this);
    volumeSlider->setAudioOutput(audioOutput);
    volumeSlider->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    volumeSlider->setOrientation(Qt::Horizontal);
    volumeSlider->setMuteVisible(false);
//    volumeAddedAction = bar->addWidget(volumeSlider);
//    volumeAddedAction->setVisible(false);
    bar->addAction(playAction);
    bar->addAction(pauseAction);
    bar->addAction(stopAction);
    bar->addAction(repeatAction);
    bar->addAction(shuffleAction);
    bar->addAction(nextAction);
    bar->addAction(previousAction);

/*    QLabel *volumeLabel = new QLabel;
    volumeLabel->setPixmap(QPixmap("images/volume.png"));*/

/*    QPalette palette;
    palette.setBrush(QPalette::Light, Qt::darkGray);*/

    timeLcd = new QLCDNumber;
//    timeLcd->setPalette(palette);

    QStringList headers;
    headers << tr("Artist") << tr("Title") << tr("Album");

    musicTable = new QTableWidget(0, 3);
    musicTable->setHorizontalHeaderLabels(headers);
    musicTable->setSelectionMode(QAbstractItemView::SingleSelection);
    musicTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(musicTable, SIGNAL(cellDoubleClicked(int,int)),
        this, SLOT(tableClicked(int,int)));
    connect(musicTable, SIGNAL(cellClicked(int,int)),
        this, SLOT(cellClicked(int,int)));
    musicTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    QHBoxLayout *seekerLayout = new QHBoxLayout;
    QToolBar* bar2 = new QToolBar;
    bar2->addAction(volumeAction);
    seekerLayout->addWidget(bar2);
    seekerLayout->addWidget(volumeSlider);
    seekerLayout->addWidget(seekSlider);
    seekerLayout->addWidget(timeLcd);

    QVBoxLayout *playbackLayout = new QVBoxLayout;
    volumeSlider->hide ();
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
    qDebug () << "cucc: " << musicTable->columnWidth(1);
}

void MainWindow::cellClicked(int /*row*/, int)
{
    /*if (mediaObject->state() == Phonon::PlayingState)
    {
        int index = plman.indexOf(mediaObject->currentSource());
        if (index >= 0)
        {
            musicTable->selectRow(index);
        }
    }
    else if (row < plman.size())
    {
        mediaObject->setCurrentSource(plman.at(row));
        shuffleList.removeAll(row);
        shuffleList.insert(0, row);
        qDebug () << shuffleList;
    }*/
}

void MainWindow::setupShuffleList()
{
    QList<int> tmp;
    int index = plman.indexOf(mediaObject->currentSource());
    if (index < 0)
        index = 0;
    for (int i = 0; i < plman.size(); ++i)
    {
        if ((i != index))
            tmp.append(i);
    }
    shuffleList.clear();
    shuffleList.append (index);
    while (tmp.size ())
    {
        int ind = qrand () % tmp.size();
        shuffleList.append(tmp[ind]);
        tmp.removeAt(ind);
    }
    qDebug () << shuffleList;
    qDebug () << shuffleList;
}

void MainWindow::savePlaylist ()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Please select file name"), "", "Playlist Files (*.m3u)");
    plman.loadPlaylist(filename);
}

void MainWindow::loadPlaylist ()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select playlist file to load"), "", "*.m3u");
    plman.loadPlaylist (filename);
}

void MainWindow::playlistChanged(int from)
{
    while (musicTable->rowCount() > from)
    {
        musicTable->removeRow(musicTable->rowCount () - 1);
    }
    for (int i = from; i < plman.size (); ++i)
    {
        int currentRow = musicTable->rowCount();
        musicTable->insertRow(currentRow);
        setRowFromItem (currentRow, plman.getItem(i));
    }
    setupShuffleList();
}

void MainWindow::setRowFromItem (int row, const PlaylistItem& item)
{
    if (row >= musicTable->rowCount())
        return;
    if (item.artist.isEmpty() && item.title.isEmpty())
    {
        QTableWidgetItem *item1 = new QTableWidgetItem(item.uri);
        item1->setFlags(item1->flags() ^ Qt::ItemIsEditable);
        musicTable->setItem(row, 1, item1);
    }
    else
    {
        QTableWidgetItem *item1 = new QTableWidgetItem(item.artist);
        item1->setFlags(item1->flags() ^ Qt::ItemIsEditable);
        musicTable->setItem(row, 0, item1);
        QTableWidgetItem *item2 = new QTableWidgetItem(item.title);
        item2->setFlags(item2->flags() ^ Qt::ItemIsEditable);
        musicTable->setItem(row, 1, item2);
        QTableWidgetItem *item3 = new QTableWidgetItem(item.album);
        item3->setFlags(item3->flags() ^ Qt::ItemIsEditable);
        musicTable->setItem(row, 2, item3);
    }
}

void MainWindow::itemUpdated(int index)
{
    setRowFromItem (index, plman.getItem(index));
}
