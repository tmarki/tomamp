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

class PlaylistManager : public QObject
{
    Q_OBJECT
public:
    PlaylistManager(QWidget* parent);
    void addStringList (const QStringList&);
    void parseAndAddFolder (const QString& dir, bool recursive);
    void removeItem (int i);

    QStringList playlistStrings () const;
    int size () const { return items.size (); }
    int indexOf (const Phonon::MediaSource& s) const;
    const Phonon::MediaSource& at (int i) { return items[i].source; }
    const PlaylistItem& getItem (int i) const { return items[i]; }
public slots:
    void savePlaylist(const QString& filename);
    void loadPlaylist(const QString& filename);
    void clearPlaylist();
    void addPlaylist (const QString& filename);
signals:
    void playlistChanged (int from);
    void itemUpdated (int index);
private slots:
    void metaStateChanged(Phonon::State newState, Phonon::State oldState);
    void appendPlaylist (const QString& filename);
    void appendPlaylistPLS (const QString& filename);
private:

    Phonon::MediaObject *metaInformationResolver;
    QList<PlaylistItem> items;
    QWidget* parentWidget;
};

#endif // PLAYLISTMANAGER_H
