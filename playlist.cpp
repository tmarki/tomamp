#include "playlist.h"

PlayList::PlayList()
{
}

void PlayList::addFiles (const QString& path, const QStringList &thelist)
{
    foreach (QString p, thelist)
    {
        entrylist.append (PlayListEntry (path, p));
    }
}

QList<PlayListEntry> PlayList::getList()
{
    return entrylist;
}
