#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QWidget>
#include <phonon/mediaobject.h>
#include <phonon/backendcapabilities.h>

struct PlaylistItem
{
    Phonon::MediaSource source;
    QString uri;
    bool playable;
    bool localFile;
    QString title;
    QString artist;
    QString album;
    PlaylistItem (const QString& uri) : source (Phonon::MediaSource (uri)), uri (uri), playable (false), localFile (false) {  }
    bool operator ==(const Phonon::MediaSource& s) const { return source == s; }
};

class PlaylistManager
{
public:
    PlaylistManager(QWidget* parent);
    void addStringList (const QStringList&);
    void parseAndAddFolder (const QString& dir, bool recursive);

    QStringList playlist () const { return QStringList (); }
    int size () const { return items.size (); }
    int indexOf (const Phonon::MediaSource& s) const  { return items.indexOf (s); }
    const Phonon::MediaSource& at (int i) { return items[i].source; }
    const PlaylistItem& getItem (int i) const { return items[i]; }
public slots:
    void savePlaylist(const QString& filename);
    void loadPlaylist(const QString& filename);
    void clearPlaylist();
signals:
    void playlistChanged (QStringList newItems);
private slots:
    void metaStateChanged(Phonon::State newState, Phonon::State oldState);
private:

    Phonon::MediaObject *metaInformationResolver;
    QList<PlaylistItem> items;
    QWidget* parentWidget;
};

#endif // PLAYLISTMANAGER_H
