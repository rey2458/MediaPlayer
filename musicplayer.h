#pragma once
#pragma warning(disable: 4996)

#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>

class MediaPlayer : public QWidget {
    Q_OBJECT
public:
    MediaPlayer(QWidget* parent = nullptr);
    ~MediaPlayer();

    enum PlaybackMode {NORMAL, LOOP_PLAYLIST, LOOP_TRACK, SHUFFLE};

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void loadFiles(const QString& dirPath);
    void togglePlay();
    void playAudio(const QString& fileName);
    void stopAudio();

    void playNext(bool autoAdvance = false);
    void playPrev();

    bool isInsideCircle(int mx, int my, int x, int y, int size);
    void drawSpeakerMesh(QPainter& painter, const QPainterPath& shape, QColor c1, QColor c2);
    void drawGlossyButton(QPainter& painter, int x, int y, int size, const QString& iconType, QColor glowColor);

    QPoint dragPosition;
    QString currentTrack;
    QString currentFilePath;
    QString hoveredBtn;
    QString pressedBtn;

    float volume;
    float progress;
    bool draggingProgress;
    bool draggingVolume;

    QMediaPlayer* mp3Player;
    QAudioOutput* audioOutput;
    PlaybackMode playMode;

    bool isBrowserOpen;
    int currentHeight;
    int targetHeight;
    QTimer* animationTimer;
    QTimer* textTimer;
    qint64 lastToggleTime;
    int textOffset;

    QString currentDir;
    QListWidget *fileList;
};