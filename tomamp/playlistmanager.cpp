#include "playlistmanager.h"
#include <QDir>
#include <QUrl>
#include <QMap>
#include <QTemporaryFile>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

QStringList PlaylistManager::allowedExtensions;


PlaylistManager::PlaylistManager(QWidget* parent)
    : parentWidget (parent), lastMetaRead (-1)
{
    if (!allowedExtensions.size ())
    {
        allowedExtensions << "mp3" << "ogg" << "wav" << "mlp" << "wma" << "flac" << "amr";
        allowedExtensions << "midi" << "mid" << "mpeg" << "qcp" << "ac3" << "aiff" << "act";
        allowedExtensions << "alac" << "alaw" << "au" << "eac3" << "aac" << "ape" << "gsm";
        allowedExtensions << "mod" << "mp2" << "sbc" << "spx" << "tta" << "ra" << "riff";
        allowedExtensions << "m4a" << "mp4" << "m4p" << "pcm" << "amr";
    }
    metaInformationResolver = new Phonon::MediaObject(parent);
    connect(metaInformationResolver, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
        this, SLOT(metaStateChanged(Phonon::State,Phonon::State)));
    downloadmanager = new QNetworkAccessManager(this);
    connect(downloadmanager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(playlistDownloadFinished(QNetworkReply*)));
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

    QStringList files = QDir (dir).entryList(filters);

    if (files.isEmpty())
        return;


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
        if (fileSupported(fname))
        {
            qDebug () << "Adding: " << fname;
            items.append(PlaylistItem (PlaylistItem (fname)));
        }
    }
    if (items.size () > index)
    {
        metaInformationResolver->setCurrentSource(items.at(index).source);
        lastMetaRead = index;
    }
    emit playlistChanged (index);
}

void PlaylistManager::addStringList(const QStringList& list)
{
    int index = items.size();
    foreach (QString string, list)
    {
        if (fileSupported(string) || string.toLower().startsWith("http"))
        {
            if (string.toLower().startsWith("http") && ((string.toLower().endsWith("pls")
                                        || (string.toLower().endsWith("m3u")))))
            {
                downloadmanager->get(QNetworkRequest(QUrl(string)));
            }
            else
            {
                items.append(PlaylistItem (string));
            }
        }
    }
    if (items.size () > index)
    {
        metaInformationResolver->setCurrentSource(items.at(index).source);
        lastMetaRead = index;
    }
    emit playlistChanged(index);
}

void PlaylistManager::playlistDownloadFinished(QNetworkReply* rep)
{
    QTemporaryFile tmp;
    tmp.open ();
    tmp.write (rep->readAll());
    tmp.close ();
    QString filename = tmp.fileName();
    int index = items.size();
    if (rep->url().toString().right(4).toLower() == ".m3u")
        appendPlaylist(filename);
    else if (rep->url().toString().right(4).toLower() == ".pls")
        appendPlaylistPLS(filename);
    if (items.size () > index)
    {
        metaInformationResolver->setCurrentSource(items.at(index).source);
        lastMetaRead = index;
        emit playlistChanged (index);
    }
}


void PlaylistManager::metaStateChanged(Phonon::State newState, Phonon::State /*oldState*/)
{
    // NOTE: This is an ugly hack, since the metaInformationResolver doesn't properly load the assigned source when it's in the error state
    // In order to properly read the next file we have to set it as current source again when the resolver entered the stopped state after the error
    static bool wasError = false;
    if (wasError && newState == Phonon::StoppedState)
    {
        metaInformationResolver->setCurrentSource(items[lastMetaRead].source);
        wasError = false;
        return;
    }
    if (newState == Phonon::ErrorState)
    {
        wasError = true;
    }

    if (newState != Phonon::StoppedState && newState != Phonon::ErrorState)
    {
        return;
    }

    int index = lastMetaRead;

    QMap<QString, QString> metaData = metaInformationResolver->metaData();


    if (index >= 0 && newState != Phonon::ErrorState && index < items.size ())
    {
        items[index].artist = metaData.value("ARTIST");
        items[index].title = metaData.value("TITLE");
        items[index].album = metaData.value("ALBUM");
/*        items[index].year = metaData.value("DATE");
        items[index].genre = metaData.value("GENRE");
        qDebug () << "Meta " << metaData;
        qDebug () << "Info is: " << items[index].year << " - " << items[index].genre;*/
        if (!metaData.isEmpty())
            items[index].playable = true;
        emit itemUpdated (index);
    }
    if (index >= 0 && items.size () > index + 1)
    {
        metaInformationResolver->setCurrentSource(items[index + 1].source);
        lastMetaRead = index + 1;
    }
}

bool PlaylistManager::savePlaylist(const QString& filenam)
{
    QString filename = filenam;
    if (filename.isEmpty())
        return false;
    bool writepls = false;
    if (filename.length() < 4 || (filename.right(4).toLower() != ".m3u" && filename.right(4).toLower() != ".pls"))
    {
        filename += ".pls";
        writepls = true;
    }
    else if (filename.right(4).toLower() == ".pls")
        writepls = true;
    QFile f (filename);
    try
    {
        f.open(QFile::WriteOnly);
        if (writepls)
        {
            f.write ("[playlist]\n");
            f.write (QString ("NumberOfEntries=%1\n").arg (items.size ()).toAscii());
        }
        for (int i = 0; i < items.size(); ++i)
        {
            if (writepls)
            {
                f.write (QString ("File%1=%2\n").arg (i + 1).arg (items[i].uri).toAscii());
                f.write (QString ("Title%1=%2 - %3 - %4\n").arg (i + 1).arg (items[i].artist).arg (items[i].title).arg (items[i].album).toAscii());
            }
            else
            {
                f.write (items[i].uri.toAscii());
                f.write ("\n");
            }
        }
        if (writepls)
            f.write ("Version=2\n");
        f.close ();
        return true;
    }
    catch (...)
    {
    }
    return false;
}

void PlaylistManager::loadPlaylist(const QString& filename)
{
    clearPlaylist();
    if (filename.right(4).toLower() == ".m3u")
        appendPlaylist(filename);
    else if (filename.right(4).toLower() == ".pls")
        appendPlaylistPLS(filename);
    if (!items.isEmpty())
    {
        metaInformationResolver->setCurrentSource(items.at(0).source);
        lastMetaRead = 0;
    }
    emit playlistChanged (0);
}

void PlaylistManager::addPlaylist(const QString& filename)
{
    int index = items.size();
    if (filename.right(4).toLower() == ".m3u")
        appendPlaylist(filename);
    else if (filename.right(4).toLower() == ".pls")
        appendPlaylistPLS(filename);
    if (items.size () > index)
    {
        metaInformationResolver->setCurrentSource(items.at(index).source);
        lastMetaRead = index;
        emit playlistChanged (index);
    }
}

void PlaylistManager::appendPlaylist(const QString& filename)
{
    QFile f(filename);
    if (!f.open (QFile::ReadOnly))
        return;
    QString tmp = f.readAll();
    f.close ();
    QStringList lines = tmp.split("\n");
    foreach (QString l, lines)
    {
        if (l.isEmpty() || (!QFileInfo (l).exists() && (l.indexOf("http") != 0))
            || l.trimmed().startsWith("#"))
        {
            continue;
        }
        l = l.replace("\r", "");
        items.append(PlaylistItem (l));
    }
}

void PlaylistManager::appendPlaylistPLS(const QString& filename)
{
    QFile f(filename);
    if (!f.open (QFile::ReadOnly))
        return;
    QString tmp = f.readAll();
    f.close ();
    QStringList lines = tmp.split("\n");
    QMap<int, int> filemap;

    foreach (QString l, lines)
    {
        if (l.isEmpty() || l.trimmed().toLower() == "[playlist]" || l.trimmed().toLower() == "version=2"
            || l.trimmed().startsWith(";"))
        {
            continue;
        }
        l = l.replace("\r", "");
        if (l.trimmed().toLower().left(4) == "file")
        {
            QStringList tokens = l.split('=');
            if (tokens.size () < 2)
                continue;
            tokens[0] = tokens[0].mid (4);
            filemap.insert(tokens[0].toInt (), items.size ());
            items.append(PlaylistItem (tokens[1]));
        }
        else if (l.trimmed().toLower().left(5) == "title")
        {
            QStringList tokens = l.split('=');
            if (tokens.size () < 2)
                continue;
            tokens[0] = tokens[0].mid (5);
            int toupdate = filemap[tokens[0].toInt()];
            QStringList metatok = tokens[1].split (" - ");
            if (metatok.size() > 2 && toupdate >= 0 && toupdate < items.size ())
            {
                items[toupdate].artist = metatok[0];
                items[toupdate].title = metatok[1];
                metatok = metatok.mid (2);
                items[toupdate].album = metatok.join (" - ");
            }
            else
            {
                items[toupdate].title = metatok.join (" - ");
            }
        }
    }
}


void PlaylistManager::clearPlaylist()
{
    items.clear();
    emit playlistChanged(0);
}

QStringList PlaylistManager::playlistStrings() const
{
    QStringList ret;
    for (int i = 0; i < items.size (); ++i)
        ret << items[i].uri;
    return ret;
}

void PlaylistManager::removeItem(int i)
{
    items.removeAt (i);
    emit itemRemoved(i);
}


bool PlaylistManager::fileSupported (const QString& fname) const
{
    if (fname.lastIndexOf('.') < 0)
        return false;
    QString ext = fname.right(fname.size() - fname.lastIndexOf('.') - 1).toLower();
    foreach (QString e, allowedExtensions)
    {
        if (ext == e)
            return true;
    }

    return false;
}

bool PlaylistManager::moveItemUp (int i)
{
    if (i)
    {
        PlaylistItem tmp = items[i - 1];
        items[i - 1] = items[i];
        items[i] = tmp;
        return true;
    }
    return false;
}

bool PlaylistManager::moveItemDown (int i)
{
    if (i < items.size () - 1)
    {
        PlaylistItem tmp = items[i + 1];
        items[i + 1] = items[i];
        items[i] = tmp;
        return true;
    }
    return false;
}
