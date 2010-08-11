#include <QtGui>
#include <QtDebug>
#include <QInputDialog>

#include "mainwindow.h"
#include "time.h"

//#define AVOID_INPUT_DIALOG 0

MainWindow::MainWindow()
    : plman (this), settings (tr ("TomAmp"), "TomAmp"), isPlaying (false)
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
    timeLcd->display("00:00:00");
    plman.addStringList(settings.value("lastPlaylist").toStringList());
    setupShuffleList();
    int curind = settings.value("currentIndex", -1).toInt ();
    if (curind >= 0)
        setItem (curind, false);
    audioOutput->setVolume(settings.value("volume", .5).toReal());
    QApplication::setWindowIcon(QIcon (QPixmap (":images/tomamp")));
}

MainWindow::~MainWindow()
{
    settings.setValue("shuffle", shuffle);
    settings.setValue("repeat", repeat);
    settings.setValue("lastPlaylist", plman.playlistStrings());
    settings.setValue("volume", audioOutput->volume());
    settings.setValue("currentIndex", plman.indexOf(mediaObject->currentSource()));
    for (int i = 0; i < musicTable->columnCount(); ++i)
    {
        QString lab = QString ("colWidth_%1").arg (i);
        settings.setValue(lab, musicTable->columnWidth(i));
    }
}

void MainWindow::addFiles()
{
    QString folder = settings.value("LastFolder").toString();
    if (folder.isEmpty())
        folder = QDesktopServices::storageLocation(QDesktopServices::MusicLocation);
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Select Files To Add"),
                    folder, "Music files (*.mp3 *.ogg *.wav *.flac);;Playlists (*.m3u *.pls)");

    if (files.isEmpty())
        return;

    QString dir = QFileInfo (files[0]).absoluteDir().absolutePath();
    settings.setValue("LastFolder", dir);
    QStringList toadd;
    foreach (QString string, files)
    {
        if (string.toLower().endsWith(".pls") || string.toLower().endsWith(".m3u"))
            plman.addPlaylist(string);
        else
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

    if (dir.isEmpty())
        return;

    settings.setValue("LastFolder", dir);

    QStringList filters;
    QStringList files = QDir (dir).entryList(filters, QDir::AllDirs);
    files.removeAll(".");
    files.removeAll("..");
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
    if (url.isEmpty() || !url.toLower().startsWith("http"))
        return;
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
        "Icons by http://itweek.deviantart.com/"));
}

void MainWindow::stateChanged(Phonon::State newState, Phonon::State /* oldState */)
{
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
            next ();
            break;
        case Phonon::PlayingState:
            setWindowTitle(mediaObject->metaData().value("TITLE") + " - TomAmp");
            pauseAction->setVisible(true);
            playAction->setVisible (false);
            playAction->setEnabled(false);
            pauseAction->setEnabled(true);
            stopAction->setEnabled(true);
            //lastPlayed = plman.indexOf(mediaObject->currentSource());
            break;
        case Phonon::StoppedState:
            setWindowTitle("TomAmp");
            stopAction->setEnabled(false);
            playAction->setEnabled(true);
            pauseAction->setVisible(false);
            playAction->setVisible(true);
            pauseAction->setEnabled(false);
            timeLcd->display("00:00:00");
            unhighlightRow(plman.indexOf(mediaObject->currentSource()));
            break;
        case Phonon::PausedState:
            pauseAction->setEnabled(false);
            stopAction->setEnabled(true);
            pauseAction->setVisible(false);
            playAction->setVisible(true);
            playAction->setEnabled(true);
            break;
        case Phonon::BufferingState:
            break;
        default:
        ;
    }
}

void MainWindow::next()
{
    bool wasPlaying = isPlaying;
    if (mediaObject->state () == Phonon::ErrorState)
        wasPlaying = true;
    int index = plman.indexOf(mediaObject->currentSource());
    if (shuffle)
    {
        index = shuffleList.indexOf(plman.indexOf(mediaObject->currentSource())) + 1;
        while (index < shuffleList.size () && !plman.getItem(shuffleList[index]).playable)
        {
            index += 1;
        }
        if (index < shuffleList.size ())
        {
            setItem (index, wasPlaying);
        }
        else if (repeat)
        {
            index = 0;
            while ((index) < shuffleList.size () && !plman.getItem(shuffleList[index]).playable)
            {
                index += 1;
            }
            setItem (index, wasPlaying);
        }
    }
    else
    {
        index++;
        while ((index) < plman.size () && !plman.getItem(index).playable)
        {
            index += 1;
        }
        if (index < plman.size())
        {
            setItem (index, wasPlaying);
        }
        else if (repeat)
        {
            index = 0;
            while ((index) < plman.size () && !plman.getItem(index).playable)
            {
                index += 1;
            }
            setItem (index, wasPlaying);
        }
    }
}

void MainWindow::setItem(int i, bool doplay)
{
    if (i < plman.size() && i >= 0)
    {
        if (lastPlayed >= 0)
            unhighlightRow(lastPlayed);
        if (shuffle)
        {
            mediaObject->setCurrentSource(plman.at (shuffleList[i]));
        }
        else
        {
            mediaObject->setCurrentSource(plman.at(i));
        }
    }
    if (doplay && mediaObject->currentSource().type() != Phonon::MediaSource::Invalid)
    {
        play();
    }
    else
        stop ();
}

void MainWindow::previous()
{
    bool wasPlaying = isPlaying;//(mediaObject->state () == Phonon::PlayingState);
    if (mediaObject->state () == Phonon::ErrorState)
        wasPlaying = true;
    int index = plman.indexOf(mediaObject->currentSource());
    if (shuffle)
    {
        index = shuffleList.indexOf(plman.indexOf(mediaObject->currentSource())) - 1;
        while (index >= 0 && !plman.getItem(shuffleList[index]).playable)
        {
            index--;
        }
        if (index >= 0)
        {
            setItem (index, wasPlaying);
        }
        else if (repeat)
        {
            index = plman.size () - 1;
            while (index >= 0 && !plman.getItem(shuffleList[index]).playable)
            {
                index--;
            }
            setItem (index, wasPlaying);
        }
/*        if (index < 0)
            wasPlaying = false;*/

    }
    else
    {
        index--;
        while ((index) >= 0 && !plman.getItem(index).playable)
        {
            index--;
        }
        if (index >= 0)
        {
            setItem (index, wasPlaying);
        }
        else if (repeat)
        {
            index = plman.size() - 1;
            while ((index) >= 0 && !plman.getItem(index).playable)
            {
                index--;
            }
            setItem (index, wasPlaying);
        }
    }
}

void MainWindow::highlightRow (int i)
{
    for (int j = 0; j < 3; ++j)
    {
        QTableWidgetItem* item = musicTable->item(i, j);
        if (item)
        {
            QFont font = item->font();
            font.setBold(true);
            font.setItalic(true);
            item->setFont(font);
        }
    }
}

void MainWindow::unhighlightRow (int i)
{
    for (int j = 0; j < 3; ++j)
    {
        QTableWidgetItem* item = musicTable->item(i, j);
        if (item)
        {
            QFont font = item->font();
            font.setBold(false);
            font.setItalic(false);
            item->setFont(font);
        }
    }
}


void MainWindow::tick(qint64 time)
{
    QTime displayTime((time / 3600000), (time / 60000) % 60, (time / 1000) % 60);

    timeLcd->display(displayTime.toString("HH:mm:ss"));
}

void MainWindow::tableClicked(int row, int /* column */)
{
//    bool wasPlaying = mediaObject->state() == Phonon::PlayingState;

/*    mediaObject->stop();
    mediaObject->clearQueue();*/

    if (row >= plman.size())
        return;

    int index = row;
    while (index < shuffleList.size () && !plman.getItem(index).playable)
    {
        index += 1;
    }
    if (plman.size() > index)
    {
        if (shuffle)
            index = shuffleList.indexOf(index);
        setItem (index, true);
//        mediaObject->play();
    }
    else
    {
        index = 0;
        while (index < plman.size () && !plman.getItem(index).playable)
        {
            index += 1;
        }
        if (plman.size() > index)
        {
            if (shuffle)
                index = shuffleList.indexOf(index);
            setItem (index, true);
//            mediaObject->play();
        }
    }

}

void MainWindow::sourceChanged(const Phonon::MediaSource &source)
{
    int ind = plman.indexOf(source);
    highlightRow(ind);
    unhighlightRow(lastPlayed);
    lastPlayed = ind;
    musicTable->selectRow(ind);
    timeLcd->display("00:00:00");
}


void MainWindow::aboutToFinish()
{
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
        }
        else if (repeat)
        {
            mediaObject->enqueue(plman.at(0));
        }
    }
}

void MainWindow::finished()
{
}

void MainWindow::setupActions()
{
    playAction = new QAction(QIcon (QPixmap (":images/play")), "", this);
    playAction->setShortcut(tr("Crl+P"));
    playAction->setDisabled(true);
    pauseAction = new QAction(QIcon (QPixmap (":images/pause")), "", this);
    pauseAction->setShortcut(tr("Ctrl+A"));
    pauseAction->setDisabled(true);
    pauseAction->setVisible(false);
    stopAction = new QAction(QIcon (QPixmap (":images/stop")), "", this);
    stopAction->setShortcut(tr("Ctrl+S"));
    stopAction->setDisabled(true);
    nextAction = new QAction(QIcon (QPixmap (":images/next")), "", this);
    nextAction->setShortcut(tr("Ctrl+N"));
    previousAction = new QAction(QIcon (QPixmap (":images/previous")), "", this);
    previousAction->setShortcut(tr("Ctrl+R"));
    if (repeat)
        repeatAction = new QAction(QIcon (QPixmap (":images/repeatActive")), "", this);
    else
        repeatAction = new QAction(QIcon (QPixmap (":images/repeat")), "", this);
    repeatAction->setCheckable(true);
    repeatAction->setChecked(repeat);
    repeatAction->setShortcut(tr("Ctrl+I"));
    if (shuffle)
        shuffleAction = new QAction(QIcon (QPixmap (":images/shuffleActive")), "", this);
    else
        shuffleAction = new QAction(QIcon (QPixmap (":images/shuffle")), "", this);
    shuffleAction->setCheckable(true);
    shuffleAction->setChecked(shuffle);
    shuffleAction->setShortcut(tr("Ctrl+H"));
    volumeAction = new QAction(QIcon (QPixmap (":images/volume")), "", this);
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
/*    removeSelected = new QAction (tr("&Delete from playlist"));
    removeSelected->setShortcut(tr ("Ctrl+D"));*/

    connect(playAction, SIGNAL(triggered()), this, SLOT(play()));
    connect(pauseAction, SIGNAL(triggered()), mediaObject, SLOT(pause()) );
    connect(stopAction, SIGNAL(triggered()), this, SLOT(stop()));
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
//    connect (removeSelected, SIGNAL (triggered()), this, SLOT (removeSelectedItem()));
}

void MainWindow::removeSelectedItem()
{
    if (QMessageBox::question(this, "Confirm remove", "Are you sure you want to remove this item?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;
    int row = musicTable->currentRow();
    if (row >= 0)
        plman.removeItem(row);
}

void MainWindow::removeAllButSelectedItem()
{
    if (QMessageBox::question(this, "Confirm remove", "Are you sure you want to remove all other items?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;
    int row = musicTable->currentRow();
    if (row >= 0)
    {
        QString uri = plman.getItem(row).uri;
        QStringList lst;
        lst << uri;
        plman.clearPlaylist();
        plman.addStringList(lst);
    }
}

void MainWindow::repeatToggle ()
{
    repeat = !repeat;
    settings.setValue("repeat", QVariant (repeat));
    if (repeat)
        repeatAction->setIcon(QIcon (QPixmap (":images/repeatActive")));
    else
        repeatAction->setIcon(QIcon (QPixmap (":images/repeat")));
}

void MainWindow::shuffleToggle ()
{
    shuffle = !shuffle;
    settings.setValue("shuffle", QVariant (shuffle));
    if (shuffle)
        shuffleAction->setIcon(QIcon (QPixmap (":images/shuffleActive")));
    else
        shuffleAction->setIcon(QIcon (QPixmap (":images/shuffle")));
}

void MainWindow::volumeToggle ()
{
    volumeSlider->setVisible(volumeAction->isChecked());
}

void MainWindow::play()
{
    mediaObject->play();
    lastPlayed = plman.indexOf(mediaObject->currentSource());
    highlightRow(lastPlayed);
    isPlaying = true;
}

void MainWindow::stop()
{
    mediaObject->stop();
    isPlaying = false;
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

    contextMenu = new QMenu (this);
    removeSelected = contextMenu->addAction(tr ("Remove selected"));
    removeAllButSelected = contextMenu->addAction(tr("Remove all but selected"));
    connect (removeSelected, SIGNAL (triggered()), this, SLOT (removeSelectedItem()));
    connect (removeAllButSelected, SIGNAL (triggered()), this, SLOT (removeAllButSelectedItem()));


    timeLcd = new QLCDNumber;

    QStringList headers;
    headers << tr("Artist") << tr("Title") << tr("Album") << "Controls";

    musicTable = new QTableWidget(0, 4);
    musicTable->setHorizontalHeaderLabels(headers);
    musicTable->setSelectionMode(QAbstractItemView::SingleSelection);
    musicTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(musicTable, SIGNAL(cellDoubleClicked(int,int)),
        this, SLOT(tableClicked(int,int)));
    connect(musicTable, SIGNAL(cellClicked(int,int)),
        this, SLOT(cellClicked(int,int)));
/*    for (int i = 0; i < 3; ++i)
    {
        if (!musicTable->horizontalHeaderItem(i))
            continue;
        musicTable->horizontalHeaderItem(i)->setBackgroundColor(QColor (128, 128, 255));;
        musicTable->horizontalHeaderItem(i)->setForeground(QColor (255, 255, 255));
    }*/
    for (int i = 0; i < musicTable->columnCount(); ++i)
    {
        QString lab = QString ("colWidth_%1").arg (i);
        int val = settings.value(lab, 0).toInt();
        if (val)
            musicTable->setColumnWidth(i, val);
//        settings.setValue(lab, musicTable->columnWidth(i));
    }


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
}

void MainWindow::cellClicked(int /*row*/, int)
{
}

void MainWindow::contextMenuEvent (QContextMenuEvent*e)
{
    if (!childAt (e->pos()))
        return;
    if (childAt (e->pos())->parentWidget() != musicTable)
        return;
    contextMenu->popup(e->globalPos());
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
}

void MainWindow::savePlaylist ()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Please select file name"), "", "Playlist Files (*.m3u *.pls)");
    if (filename.isEmpty())
        return;
    plman.savePlaylist(filename);
}

void MainWindow::loadPlaylist ()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select playlist file to load"), "", "*.m3u *.pls");
    if (filename.isEmpty())
        return;
    plman.loadPlaylist (filename);
}

void MainWindow::playlistChanged(int from)
{
    while (musicTable->rowCount() > from)
    {
        musicTable->removeRow(musicTable->rowCount () - 1);
    }
    int firstGood = -1;
    for (int i = from; i < plman.size (); ++i)
    {
        if (firstGood < 0 && plman.getItem (i).playable)
            firstGood = i;
        int currentRow = musicTable->rowCount();
        musicTable->insertRow(currentRow);
        setRowFromItem (currentRow, plman.getItem(i));
    }
/*    if (plman.indexOf(mediaObject->currentSource()) < 0)
    {
        setItem (firstGood, false);
    }*/
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
    qDebug () << "Widget: " << musicTable->cellWidget(row, 3);

    if (!musicTable->cellWidget(row, 3))
    {
        QToolBar* bar = new QToolBar;
        QPushButton* up = new QPushButton;
        up->setText("up");
        up->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        bar->setProperty("row", row);
        bar->addWidget(up);
        musicTable->setCellWidget(row, 3, bar);
        connect (up, SIGNAL (clicked ()),  this, SLOT (buttonUp ()));
    }
}

void MainWindow::buttonUp()
{
    qDebug () << "Presses up on " << sender()->parent()->property("row");
    plman.moveItemUp(sender()->parent()->property("row").toInt());
}

void MainWindow::itemUpdated(int index)
{
    if (plman.indexOf(mediaObject->currentSource()) < 0 && plman.getItem (index).playable)
    {
        setItem (index, false);
    }
    setRowFromItem (index, plman.getItem(index));
    if (plman.indexOf(mediaObject->currentSource()) == index)
    {
        if (shuffle) index = shuffleList.indexOf(index);
        setItem (index, false);
    }
}
