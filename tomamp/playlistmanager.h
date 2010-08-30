#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QWidget>
#include <phonon/mediaobject.h>
#include <phonon/backendcapabilities.h>


class QNetworkAccessManager;
class QNetworkReply;

struct PlaylistItem
{
    Phonon::MediaSource source;
    QString uri;
    bool playable;
    bool localFile;
    QString title;
    QString artist;
    QString album;
/*    QString year;
    QString genre;
    QString length;*/
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
    QStringList allowedExt () const { return allowedExtensions; }
    int size () const { return items.size (); }
    int indexOf (const Phonon::MediaSource& s) const;
    const Phonon::MediaSource& at (int i) { return items[i].source; }
    const PlaylistItem& getItem (int i) const { return items[i]; }
    bool moveItemUp (int i);
    bool moveItemDown (int i);
public slots:
    bool savePlaylist(const QString& filename);
    void loadPlaylist(const QString& filename);
    void clearPlaylist();
    void addPlaylist (const QString& filename);
signals:
    void playlistChanged (int from);
    void itemRemoved (int i);
    void itemUpdated (int index);
private slots:
    void metaStateChanged(Phonon::State newState, Phonon::State oldState);
    void appendPlaylist (const QString& filename);
    void appendPlaylistPLS (const QString& filename);
    void playlistDownloadFinished(QNetworkReply*);
private:
    bool fileSupported (const QString& fname) const;

    Phonon::MediaObject *metaInformationResolver;
    QList<PlaylistItem> items;
    QWidget* parentWidget;
    static QStringList allowedExtensions;
    int lastMetaRead;
    QNetworkAccessManager *downloadmanager;
};

#endif // PLAYLISTMANAGER_H
