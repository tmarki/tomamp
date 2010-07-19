#include "playlistmanager.h"
#include <QDir>

PlaylistManager::PlaylistManager(QWidget* parent)
    : parentWidget (parent)
{
    metaInformationResolver = new Phonon::MediaObject(parent);
    connect(metaInformationResolver, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
        this, SLOT(metaStateChanged(Phonon::State,Phonon::State)));
}

void PlaylistManager::parseAndAddFolder(const QString &dir, bool recursive)
{
    QStringList filters;
//    filters << "*.mp3";

    QStringList files = QDir (dir).entryList(filters);

    if (files.isEmpty())
        return;

    qDebug () << "Parsing folder " << dir;

    settings.setValue("LastFolder", dir);
    int index = items.size();
    foreach (QString string, files)
    {
        if (string == "."  || string == "..")
            continue;
        QString fname = dir + "/" + string;
        QFileInfo fi (fname);
        if (fi.isDir())
        {
            if (recursive)
                parseAndAddFolder(fname, true);
            continue;
        }
        qDebug () << fname;
        items.append(PlaylistItem (PlaylistItem (fname)));
    }
    if (!items.isEmpty())
        metaInformationResolver->setCurrentSource(items.at(index).source);
}

void PlaylistManager::addStringList(const QStringList& list)
{
    int index = items.size();
    foreach (QString string, list)
    {
        items.append(PlaylistItem (string));
    }
    if (!items.isEmpty())
        metaInformationResolver->setCurrentSource(items.at(index).source);
}

void PlaylistManager::metaStateChanged(Phonon::State newState, Phonon::State /* oldState */)
{
    if (newState == Phonon::ErrorState)
    {
//        QMessageBox::warning(this, tr("Error opening files"),
//        metaInformationResolver->errorString());
        while (!items.isEmpty() &&
            !(items.takeLast().source == metaInformationResolver->currentSource())) {}  /* loop */;
        qDebug () << items.size();
/*        int index = sources.indexOf(metaInformationResolver->currentSource());
        if (index >= 0)
        {
            sources.removeAt(index);
            qDebug () << "Removing invalid file in " << index << ": " << metaInformationResolver->currentSource().fileName();
            if (sources.size() > index)
            {
                metaInformationResolver->setCurrentSource(sources.at(index));
                qDebug () << "Setting new info source " << sources.at(index).fileName();
            }
        }*/
//        int index = sources.indexOf(metaInformationResolver->currentSource()) + 1;
//        sources.removeAt(index - 1);
//        if (items.size())
//        {
//            metaInformationResolver->setCurrentSource(sources.at(0));
//        }
        return;
    }

    if (newState != Phonon::StoppedState && newState != Phonon::PausedState)
    {
        return;
    }

    if (metaInformationResolver->currentSource().type() == Phonon::MediaSource::Invalid)
        return;
    qDebug () << "Reading meta info of " << metaInformationResolver->currentSource().fileName() << " " << metaInformationResolver->currentSource().type();

    qDebug () << "Index of this source is " << items.indexOf(metaInformationResolver->currentSource());

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

/*    int currentRow = musicTable->rowCount();
    musicTable->insertRow(currentRow);
    musicTable->setItem(currentRow, 0, artistItem);
    musicTable->setItem(currentRow, 1, titleItem);
    musicTable->setItem(currentRow, 2, albumItem);*/


/*    if (musicTable->selectedItems().isEmpty())
    {
        musicTable->selectRow(0);
        qDebug () << "Setting current media " + metaInformationResolver->currentSource().fileName();
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
    }*/
}

void PlaylistManager::savePlaylist(const QString& filename)
{
//    QString filename = QFileDialog::getSaveFileName(parentWidget, tr("Please select file name"), "", "Playlist Files (*.m3u)");
    if (filename.isEmpty())
        return;
    if (filename.length() < 4 || filename.right(4).toLower() != ".m3u")
        filename += ".m3u";
    QFile f (filename);
    try
    {
        f.open(QFile::WriteOnly);
        for (int i = 0; i < items.size(); ++i)
        {
            f.write (items[i].uri.toAscii());
            f.write ("\n");
        }
        f.close ();
    }
    catch (...)
    {
//        QMessageBox::critical(this, "Write error", "Could not write playlist file", QMessageBox::Ok);
    }
}

void PlaylistManager::loadPlaylist(const QString& filename)
{
//    QString filename = QFileDialog::getOpenFileName(parentWidget, tr("Select playlist file to load"), "", "*.m3u");
    QFile f(filename);
    f.open (QFile::ReadOnly);
    QString tmp = f.readAll();
    f.close ();
    QStringList lines = tmp.split("\n");
    clearPlaylist();
    foreach (QString l, lines)
    {
        if (l.isEmpty() || (!QFileInfo (l).exists() && (l.indexOf("http") != 0)))
        {
            qDebug () << "not loadable: " << l;\
            continue;
        }
        qDebug () << "Load " << l;
        items.append(PlaylistItem (l));
    }
    if (!items.isEmpty())
        metaInformationResolver->setCurrentSource(items.at(0).source);
}

void MainWindow::clearPlaylist()
{
    items.clear();
/*    while (musicTable->rowCount())
        musicTable->removeRow(0);
    mediaObject->clear();*/
}
