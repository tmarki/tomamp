#include "playlistmanager.h"
#include <QDir>
#include <QUrl>

PlaylistManager::PlaylistManager(QWidget* parent)
    : parentWidget (parent)
{
    metaInformationResolver = new Phonon::MediaObject(parent);
    connect(metaInformationResolver, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
        this, SLOT(metaStateChanged(Phonon::State,Phonon::State)));
}

int PlaylistManager::indexOf(const Phonon::MediaSource &s) const
{
    for (int i = 0; i < items.size(); ++i)
    {
        if (items[i].source == s)
            return i;
    }
    return -1;
}

void PlaylistManager::parseAndAddFolder(const QString &dir, bool recursive)
{
    QStringList filters;
//    filters << "*.mp3";

    QStringList files = QDir (dir).entryList(filters);

    if (files.isEmpty())
        return;

    qDebug () << "Parsing folder " << dir;

    //settings.setValue("LastFolder", dir);
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
        qDebug () << "Adding: " << fname;
        items.append(PlaylistItem (PlaylistItem (fname)));
    }
    if (!items.isEmpty())
        metaInformationResolver->setCurrentSource(items.at(index).source);
    qDebug () << " SIZE: " << items.size ();
    emit playlistChanged (index);
}

void PlaylistManager::addStringList(const QStringList& list)
{
    int index = items.size();
    foreach (QString string, list)
    {
        qDebug () << "Adding " << string;
        items.append(PlaylistItem (string));
    }
    if (!items.isEmpty())
        metaInformationResolver->setCurrentSource(items.at(index).source);
    emit playlistChanged(index);
}

void PlaylistManager::metaStateChanged(Phonon::State newState, Phonon::State /* oldState */)
{
    if (newState == Phonon::ErrorState)
    {
//        QMessageBox::warning(this, tr("Error opening files"),
//        metaInformationResolver->errorString());
//        while (!items.isEmpty() &&
//            !(items.takeLast().source == metaInformationResolver->currentSource())) {}  /* loop */;
        int index = indexOf (metaInformationResolver->currentSource());
        if (index >= 0 && items.size () > index - 1)
            metaInformationResolver->setCurrentSource(items[index + 1].source);
        qDebug () << "Error for item " << index;
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
    int index = indexOf (metaInformationResolver->currentSource());
    qDebug () << "Reading meta info of " << metaInformationResolver->currentSource().fileName() << " " << index;

    qDebug () << "Index of this source is " << indexOf(metaInformationResolver->currentSource());

    QMap<QString, QString> metaData = metaInformationResolver->metaData();

/*    QString title = metaData.value("TITLE");
    if (title == "")
        title = metaInformationResolver->currentSource().fileName();

    if (title == "")
        title = metaInformationResolver->currentSource().url().toString();*/

    if (index >= 0)
    {
        items[index].artist = metaData.value("ARTIST");
        items[index].title = metaData.value("TITLE");
        items[index].album = metaData.value("ALBUM");
        if (metaData.isEmpty())
            qDebug () << "Detected to be empty: " << items[index].uri;
        else
            items[index].playable = true;
        emit itemUpdated (index);
        if (index >= 0 && items.size () > index + 1)
            metaInformationResolver->setCurrentSource(items[index + 1].source);
    }

    /*QTableWidgetItem *titleItem = new QTableWidgetItem(title);
    titleItem->setFlags(titleItem->flags() ^ Qt::ItemIsEditable);
    QTableWidgetItem *artistItem = new QTableWidgetItem(metaData.value("ARTIST"));
    artistItem->setFlags(artistItem->flags() ^ Qt::ItemIsEditable);
    QTableWidgetItem *albumItem = new QTableWidgetItem(metaData.value("ALBUM"));
    albumItem->setFlags(albumItem->flags() ^ Qt::ItemIsEditable);*/

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

void PlaylistManager::savePlaylist(const QString& filenam)
{
    QString filename = filenam;
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
    qDebug () << "Attempting to load playlist: " << filename;
    QFile f(filename);
    if (!f.open (QFile::ReadOnly))
        return;
    QString tmp = f.readAll();
    f.close ();
    QStringList lines = tmp.split("\n");
    clearPlaylist();
    foreach (QString l, lines)
    {
        if (l.isEmpty() || (!QFileInfo (l).exists() && (l.indexOf("http") != 0)))
        {
            continue;
        }
        qDebug () << "Load " << l;
        items.append(PlaylistItem (l));
    }
    if (!items.isEmpty())
        metaInformationResolver->setCurrentSource(items.at(0).source);
    emit playlistChanged (0);
}

void PlaylistManager::clearPlaylist()
{
    items.clear();
    emit playlistChanged(0);
/*    while (musicTable->rowCount())
        musicTable->removeRow(0);
    mediaObject->clear();*/
}

QStringList PlaylistManager::playlistStrings() const
{
    QStringList ret;
    for (int i = 0; i < items.size (); ++i)
        ret << items[i].uri;
    qDebug () << "Returning playlist " << ret << " SIZE: " << items.size ();
    return ret;
}

void PlaylistManager::removeItem(int i)
{
    items.removeAt (i);
    emit playlistChanged(i);
}
