#ifndef PLAYLIST_H
#define PLAYLIST_H

#include<QStringList>
#include<QList>

class PlayListEntry
{
public:
    PlayListEntry (const QString& thepath, const QString& thename) : path (thepath), name (thename) { }
    QString getName () { return name; }
    QString getPath () { return path; }
private:
    QString path;
    QString name;
};

class PlayList
{
public:
    PlayList();
    void addFiles (const QString& dir, const QStringList& thelist);
    QList<PlayListEntry> getList ();
private:
    QList<PlayListEntry> entrylist;
};


#endif // PLAYLIST_H
