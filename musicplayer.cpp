#include "musicplayer.h"
#include <QDir>
#include <QDateTime>
#include <QApplication>

MediaPlayer::MediaPlayer(QWidget* parent) : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setFixedSize(500, 185);

    currentTrack = "Click EJECT to open an MP3...";
    volume = 0.7f;
    progress = 0.0f;
    draggingProgress = false;
    draggingVolume = false;
    isBrowserOpen = false;
    currentHeight = 185;
    targetHeight = 185;
    textOffset = 0;
    lastToggleTime = 0;
    playMode = NORMAL;

    currentDir = QDir::currentPath();

    mp3Player = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    mp3Player->setAudioOutput(audioOutput);
    audioOutput->setVolume(volume);

    connect(mp3Player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            if (playMode == LOOP_TRACK) {
                playAudio(currentFilePath);
            }
            else {
                playNext(true);
            }
        }
    });

    fileList = new QListWidget(this);
    fileList->setGeometry(30, 175, 455, 0);
    fileList->setStyleSheet("QListWidget { background-color: rgb(15, 20, 25); color: rgb(0, 255, 200); font-family: Consolas; font-size: 14px; border: 2px solid rgba(0, 200, 255, 100); } "
                            "QListWidget::item:selected { background-color: rgb(0, 150, 255); color: white; }");
    fileList->hide();

    loadFiles(currentDir);

    connect(fileList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        QString selected = item->text();
        if (selected == "[ .. ] (Up)") {
            QDir dir(currentDir);
            dir.cdUp();
            currentDir = dir.absolutePath();
            loadFiles(currentDir);
        }
        else if (selected.startsWith("[ ") && selected.endsWith(" ]")) {
            QString folderName = selected.mid(2, selected.length() - 4);
            currentDir = QDir(currentDir).absoluteFilePath(folderName);
            loadFiles(currentDir);
        }
        else {
            stopAudio();
            currentFilePath = QDir(currentDir).absoluteFilePath(selected);
            currentTrack = selected;
            textOffset = 0;
            playAudio(currentFilePath);
        }
    });

    textTimer = new QTimer(this);
    connect(textTimer, &QTimer::timeout, this, [this]() {
        textOffset -= 1;
        update();
    });
    textTimer->start(30);

    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, [this]() {
        int step = 20;
        if (currentHeight < targetHeight) {
            currentHeight += step;
            if (currentHeight >= targetHeight) {
                currentHeight = targetHeight;
                animationTimer->stop();
            }
        }
        else if (currentHeight > targetHeight) {
            currentHeight -= step;
            if (currentHeight <= targetHeight) {
                currentHeight = targetHeight;
                animationTimer->stop();
                fileList->hide();
            }
        }

        setFixedSize(500, currentHeight);

        int listHeight = currentHeight - 185;
        if (listHeight > 0) {
            fileList->setGeometry(30, 175, 455, listHeight - 5);
            if (fileList->isHidden()) fileList->show();
        }
        update();
    });
}

MediaPlayer::~MediaPlayer() {}

void MediaPlayer::loadFiles(const QString& dirPath) {
    fileList->clear();
    QDir dir(dirPath);
    if (!dir.isRoot()) {
        fileList->addItem("[ .. ] (Up)");
    }

    QFileInfoList list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden, QDir::Name);
    for (const QFileInfo& fileInfo : list) {
        if (!fileInfo.isHidden()) fileList->addItem("[ " + fileInfo.fileName() + " ]");
    }

    QFileInfoList fileInfoList = dir.entryInfoList(QStringList() << "*.mp3", QDir::Files, QDir::Name);
    for (const QFileInfo& fileInfo : fileInfoList) {
        fileList->addItem(fileInfo.fileName());
    }
}

void MediaPlayer::togglePlay() {
    if (mp3Player->playbackState() == QMediaPlayer::PlayingState) {
        mp3Player->pause();
    }
    else if (mp3Player->playbackState() == QMediaPlayer::PausedState) {
        mp3Player->play();
    }
    else {
        if (!currentFilePath.isEmpty()) playAudio(currentFilePath);
    }
    update();
}

void MediaPlayer::playAudio(const QString& fileName) {
    mp3Player->setSource(QUrl::fromLocalFile(fileName));
    mp3Player->play();
    progress = 0.0f;
    update();
}

void MediaPlayer::stopAudio() {
    mp3Player->stop();
    update();
}

void MediaPlayer::playNext(bool autoAdvance) {
    QStringList mp3Files;
    for (int i = 0; i < fileList->count(); ++i) {
        QString txt = fileList->item(i)->text();
        if (!txt.startsWith("[")) mp3Files.append(txt);
    }
    if (mp3Files.isEmpty()) return;

    int currentIndex = mp3Files.indexOf(currentTrack);
    int nextIndex = currentIndex;

    if (playMode == SHUFFLE) {
        nextIndex = QRandomGenerator::global()->bounded(mp3Files.size());
    }
    else {
        nextIndex = currentIndex + 1;
        if (nextIndex >= mp3Files.size()) {
            if (playMode == LOOP_PLAYLIST || !autoAdvance) {
                nextIndex = 0;
            }
            else {
                stopAudio();
                return;
            }
        }
    }
    currentTrack = mp3Files[nextIndex];
    currentFilePath = QDir(currentDir).absoluteFilePath(currentTrack);
    textOffset = 0;
    playAudio(currentFilePath);
}

void MediaPlayer::playPrev() {
    QStringList mp3Files;
    for (int i = 0; i < fileList->count(); ++i) {
        QString txt = fileList->item(i)->text();
        if (!txt.startsWith("[")) mp3Files.append(txt);
    }
    if (mp3Files.isEmpty()) return;
    int currentIndex = mp3Files.indexOf(currentTrack);
    int prevIndex = currentIndex - 1;
    if (playMode == SHUFFLE) {
        prevIndex = QRandomGenerator::global()->bounded(mp3Files.size());
    }
    else {
        if (prevIndex < 0) prevIndex = mp3Files.size() - 1;
    }
    currentTrack = mp3Files[prevIndex];
    currentFilePath = QDir(currentDir).absoluteFilePath(currentTrack);
    textOffset = 0;
    playAudio(currentFilePath);
}

bool MediaPlayer::isInsideCircle(int mx, int my, int x, int y, int size) {
    int cx = x + size / 2;
    int cy = y + size / 2;
    int radius = size / 2;
    return std::pow(mx - cx, 2) + std::pow(my - cy, 2) <= std::pow(radius, 2);
}

void MediaPlayer::mousePressEvent(QMouseEvent* event) {
    int mx = event->position().x();
    int my = event->position().y();

    if (mx >= 463 && mx <= 488 && my >= 8 && my <= 33) {
        stopAudio();
        QApplication::quit();
        return;
    }

    if (mx >= 140 && mx <= 460 && my >= 95 && my <= 125) {
        draggingProgress = true;
        progress = std::max(0.0f, std::min(1.0f, (float)(mx - 140) / 320.0f));
        if (mp3Player->duration() > 0) {
            mp3Player->setPosition(progress * mp3Player->duration());
        }
        update();
    }
    else if (mx >= 350 && mx <= 450 && my >= 140 && my <= 165) {
        draggingVolume = true;
        volume = std::max(0.0f, std::min(1.0f, (float)(mx - 350) / 110.0f));
        audioOutput->setVolume(volume);
        update();
    }
    else if (!hoveredBtn.isEmpty()) {
        pressedBtn = hoveredBtn;
        update();
    }
    else {
        dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
}

void MediaPlayer::mouseMoveEvent(QMouseEvent* event) {
    int mx = event->position().x();
    int my = event->position().y();

    if (event->buttons() & Qt::LeftButton) {
        if (draggingProgress) {
            progress = std::max(0.0f, std::min(1.0f, (float)(mx - 140) / 320.0f));
            if (mp3Player->duration() > 0) mp3Player->setPosition(progress * mp3Player->duration());
            update();
        }
        else if (draggingVolume) {
            volume = std::max(0.0f, std::min(1.0f, (float)(mx - 350) / 110.0f));
            audioOutput->setVolume(volume);
            update();
        }
        else if (hoveredBtn.isEmpty()) {
            move(event->globalPosition().toPoint() - dragPosition);
        }
    }
    else {
        if (isInsideCircle(mx, my, 45, 25, 75)) hoveredBtn = "PLAY";
        else if (isInsideCircle(mx, my, 140, 130, 40)) hoveredBtn = "PREV";
        else if (isInsideCircle(mx, my, 200, 130, 40)) hoveredBtn = "NEXT";
        else if (isInsideCircle(mx, my, 260, 130, 40)) hoveredBtn = "EJECT";
        else if (mx >= 130 && mx <= 460 && my >= 25 && my <= 90) hoveredBtn = "DISPLAY";
        else hoveredBtn = "";
        update();
    }
}

void MediaPlayer::mouseReleaseEvent(QMouseEvent* event) {
    draggingProgress = false;
    draggingVolume = false;

    QString action = pressedBtn;
    pressedBtn = "";
    update();

    if (action == "PLAY") {
        togglePlay();
    }
    else if (action == "PREV") {
        playPrev();
    }
    else if (action == "NEXT") {
        playNext();
    }
    else if (action == "DISPLAY") {
        int mode = (int)playMode + 1;
        if (mode > 3) mode = 0;
        playMode = (PlaybackMode)mode;
    }
    else if (action == "EJECT") {
        if (QDateTime::currentMSecsSinceEpoch() - lastToggleTime < 500) return;
        lastToggleTime = QDateTime::currentMSecsSinceEpoch();

        isBrowserOpen = !isBrowserOpen;
        targetHeight = isBrowserOpen ? 400 : 185;
        loadFiles(currentDir);
        animationTimer->start(15);
    }
}

void MediaPlayer::drawSpeakerMesh(QPainter& painter, const QPainterPath& shape, QColor c1, QColor c2) {
    QRectF bounds = shape.boundingRect();
    QLinearGradient metal(bounds.topLeft(), bounds.topRight());
    metal.setColorAt(0, c1);
    metal.setColorAt(1, c2);

    painter.fillPath(shape, metal);
    painter.setClipPath(shape);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(10, 20, 30, 180));

    for (int i = bounds.x(); i < bounds.x() + bounds.width(); i += 4) {
        for (int j = bounds.y(); j < bounds.y() + bounds.height(); j += 4) {
            int offset = (j % 8 == 0) ? 2 : 0;
            painter.drawRect(i + offset, j, 2, 2);
        }
    }
    painter.setClipping(false);
    painter.setPen(QPen(QColor(0, 0, 0, 180), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(shape);
}

void MediaPlayer::drawGlossyButton(QPainter& painter, int x, int y, int size, const QString& iconType, QColor glowColor) {
    bool isHover = (hoveredBtn == iconType);
    bool isPressed = (pressedBtn == iconType);

    if (isPressed) { x += 1; y += 1; }
    if (isHover && !isPressed) {
        glowColor = QColor(glowColor.red(), std::min(255, glowColor.green() + 50), std::min(255, glowColor.blue() + 50), 255);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 150));
    painter.drawEllipse(x + 2, y + 4, size, size);

    QLinearGradient btnGrad(x, y, x, y + size);
    btnGrad.setColorAt(0, QColor(60, 70, 80));
    btnGrad.setColorAt(1, QColor(10, 15, 20));
    painter.setBrush(btnGrad);
    painter.drawEllipse(x, y, size, size);

    QLinearGradient btnNeon(x, y + size, x, y + size / 2.0);
    btnNeon.setColorAt(0, glowColor);
    btnNeon.setColorAt(1, QColor(glowColor.red(), glowColor.green(), glowColor.blue(), 0));
    painter.setBrush(btnNeon);
    painter.drawEllipse(x + 5, y + 5, size - 10, size - 10);

    QLinearGradient btnGloss(x, y, x, y + size / 2.0);
    btnGloss.setColorAt(0, QColor(255, 255, 255, 180));
    btnGloss.setColorAt(1, QColor(255, 255, 255, 0));
    painter.setBrush(btnGloss);
    painter.drawEllipse(x + size / 10, y + size / 15, size - size / 5, size / 2);

    painter.setBrush(Qt::white);
    bool isPlayingState = mp3Player->playbackState() == QMediaPlayer::PlayingState;

    if (iconType == "PLAY" && isPlayingState) {
        painter.drawRect(x + size * 0.35, y + size * 0.3, size * 0.1, size * 0.4);
        painter.drawRect(x + size * 0.55, y + size * 0.3, size * 0.1, size * 0.4);
    }
    else if (iconType == "PLAY") {
        QPolygonF poly;
        poly << QPointF(x + size * 0.35, y + size * 0.25)
             << QPointF(x + size * 0.7, y + size * 0.5)
             << QPointF(x + size * 0.35, y + size * 0.75);
        painter.drawPolygon(poly);
    }
    else if (iconType == "PREV") {
        QPolygonF p1; p1 << QPointF(x + size * 0.53, y + size * 0.35) << QPointF(x + size * 0.37, y + size * 0.5) << QPointF(x + size * 0.53, y + size * 0.65);
        QPolygonF p2; p2 << QPointF(x + size * 0.71, y + size * 0.35) << QPointF(x + size * 0.55, y + size * 0.5) << QPointF(x + size * 0.71, y + size * 0.65);
        painter.drawPolygon(p1); painter.drawPolygon(p2);
        painter.drawRect(x + size * 0.29, y + size * 0.35, size * 0.06, size * 0.3);
    }
    else if (iconType == "NEXT") {
        QPolygonF p1; p1 << QPointF(x + size * 0.29, y + size * 0.35) << QPointF(x + size * 0.45, y + size * 0.5) << QPointF(x + size * 0.29, y + size * 0.65);
        QPolygonF p2; p2 << QPointF(x + size * 0.47, y + size * 0.35) << QPointF(x + size * 0.63, y + size * 0.5) << QPointF(x + size * 0.47, y + size * 0.65);
        painter.drawPolygon(p1); painter.drawPolygon(p2);
        painter.drawRect(x + size * 0.65, y + size * 0.35, size * 0.06, size * 0.3);
    }
    else if (iconType == "EJECT") {
        QPolygonF p; p << QPointF(x + size * 0.3, y + size * 0.55) << QPointF(x + size * 0.5, y + size * 0.35) << QPointF(x + size * 0.7, y + size * 0.55);
        painter.drawPolygon(p);
        painter.drawRect(x + size * 0.3, y + size * 0.65, size * 0.4, size * 0.08);
    }
}

void MediaPlayer::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    if (currentHeight > 185) {
        painter.setPen(QPen(QColor(0, 200, 255, 100), 2));
        painter.setBrush(QColor(20, 25, 30));
        painter.drawRoundedRect(25, 160, 465, currentHeight - 160 - 5, 20, 20);
    }

    QPainterPath bodyPath;
    bodyPath.moveTo(40, 0);
    bodyPath.lineTo(460, 0);
    bodyPath.quadTo(490, 0, 490, 30);
    bodyPath.lineTo(490, 150);
    bodyPath.quadTo(490, 180, 460, 180);
    bodyPath.lineTo(40, 180);
    bodyPath.quadTo(25, 180, 25, 165);
    bodyPath.quadTo(55, 90, 25, 15);
    bodyPath.quadTo(25, 0, 40, 0);
    bodyPath.closeSubpath();

    QLinearGradient chromeMetal(0, 0, 500, 200);
    chromeMetal.setColorAt(0.0f, QColor(240, 245, 250));
    chromeMetal.setColorAt(0.2f, QColor(180, 185, 195));
    chromeMetal.setColorAt(0.5f, QColor(255, 255, 255));
    chromeMetal.setColorAt(0.8f, QColor(140, 145, 155));
    chromeMetal.setColorAt(1.0f, QColor(100, 105, 115));

    painter.setPen(Qt::NoPen);
    painter.setBrush(chromeMetal);
    painter.drawPath(bodyPath);

    painter.setClipPath(bodyPath);
    QLinearGradient bodyGloss(25, 0, 25, 90);
    bodyGloss.setColorAt(0, QColor(255, 255, 255, 150));
    bodyGloss.setColorAt(1, QColor(255, 255, 255, 0));
    painter.setBrush(bodyGloss);
    painter.drawPath(bodyPath);
    painter.setClipping(false);

    painter.setPen(QPen(QColor(50, 55, 60, 200), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(bodyPath);

    int bottomY = currentHeight - 5;
    QPainterPath leftGrip;
    leftGrip.moveTo(25, 15);
    leftGrip.quadTo(55, 90, 25, 165);
    leftGrip.lineTo(25, bottomY - 15);
    leftGrip.quadTo(25, bottomY, 15, bottomY);
    leftGrip.quadTo(5, bottomY, 5, bottomY - 15);
    leftGrip.lineTo(5, 165);
    leftGrip.quadTo(35, 90, 5, 15);
    leftGrip.quadTo(5, 0, 15, 0);
    leftGrip.quadTo(25, 0, 25, 15);
    leftGrip.closeSubpath();

    drawSpeakerMesh(painter, leftGrip, QColor(0, 220, 255), QColor(0, 80, 140));

    int screenX = 130;
    int screenY = 25;
    int screenW = 320;
    int screenH = 65;

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(10, 15, 20, 200));
    painter.drawRoundedRect(screenX - 4, screenY - 4, screenW + 8, screenH + 6, 20, 20);

    QLinearGradient bezelGrad(screenX, screenY, screenX, screenY + screenH);
    bezelGrad.setColorAt(0, QColor(200, 210, 220));
    bezelGrad.setColorAt(1, QColor(100, 110, 120));
    painter.setBrush(bezelGrad);
    painter.drawRoundedRect(screenX - 2, screenY - 2, screenW + 4, screenH + 4, 18, 18);

    QLinearGradient screenGrad(screenX, screenY, screenX, screenY + screenH);
    screenGrad.setColorAt(0, QColor(0, 25, 45));
    screenGrad.setColorAt(1, QColor(0, 70, 110));
    painter.setBrush(screenGrad);
    painter.drawRoundedRect(screenX, screenY, screenW, screenH, 15, 15);

    QPainterPath innerShadow;
    innerShadow.addRoundedRect(screenX, screenY, screenW, screenH, 15, 15);
    painter.setClipPath(innerShadow);
    painter.setPen(QPen(QColor(0, 0, 0, 180), 5));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(screenX, screenY, screenW, screenH, 15, 15);

    painter.setPen(QColor(0, 0, 0, 40));
    for (int i = screenY; i < screenY + screenH; i += 3) {
        painter.drawLine(screenX, i, screenX + screenW, i);
    }

    QFont font("Consolas", 14, QFont::Bold);
    painter.setFont(font);
    QFontMetrics fm(font);

    int textWidth = fm.horizontalAdvance(currentTrack);
    int gap = 50;
    if (textOffset < -(textWidth + gap)) textOffset = 0;

    painter.setPen(QColor(0, 0, 0, 150));
    painter.drawText(screenX + 15 + textOffset + 1, screenY + 28 + 1, currentTrack);
    painter.drawText(screenX + 15 + textOffset + textWidth + gap + 1, screenY + 28 + 1, currentTrack);

    painter.setPen(QColor(0, 255, 200));
    painter.drawText(screenX + 15 + textOffset, screenY + 28, currentTrack);
    painter.drawText(screenX + 15 + textOffset + textWidth + gap, screenY + 28, currentTrack);

    QString modeStr = "";
    if (playMode == LOOP_PLAYLIST) modeStr = " [LOOP ALL]";
    else if (playMode == LOOP_TRACK) modeStr = " [LOOP ONE]";
    else if (playMode == SHUFFLE) modeStr = " [SHUFFLE]";

    QString statusText = "Stopped";
    if (mp3Player->playbackState() != QMediaPlayer::StoppedState) {
        qint64 pos = mp3Player->position();
        if (!draggingProgress && mp3Player->duration() > 0) {
            progress = (float)pos / mp3Player->duration();
        }
        int sec = (pos / 1000) % 60;
        int min = (pos / 1000) / 60;
        QString state = mp3Player->playbackState() == QMediaPlayer::PausedState ? "Paused" : "Playing";
        statusText = QString("%1 [%2:%3]").arg(state).arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')).arg(modeStr);
    }
    else {
        statusText += modeStr;
    }

    QFont statusFont("Consolas", 10);
    painter.setFont(statusFont);
    painter.setPen(QColor(0, 0, 0, 150));
    painter.drawText(screenX + 15 + 1, screenY + 50 + 1, statusText);
    painter.setPen(QColor(0, 255, 200));
    painter.drawText(screenX + 15, screenY + 50, statusText);

    QLinearGradient topGloss(screenX, screenY, screenX, screenY + screenH / 2.0);
    topGloss.setColorAt(0, QColor(255, 255, 255, 90));
    topGloss.setColorAt(1, QColor(255, 255, 255, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(topGloss);
    painter.drawEllipse(screenX - 20, screenY - 30, screenW + 40, screenH + 40);

    QLinearGradient bottomGloss(screenX, screenY + screenH, screenX, screenY + screenH - 12);
    bottomGloss.setColorAt(0, QColor(255, 255, 255, 40));
    bottomGloss.setColorAt(1, QColor(255, 255, 255, 0));
    painter.setBrush(bottomGloss);
    painter.drawRoundedRect(screenX, screenY + screenH - 12, screenW, 12, 15, 15);

    painter.setClipping(false);

    QFont closeFont("Arial", 12, QFont::Bold);
    painter.setFont(closeFont);
    QFontMetrics cfm(closeFont);
    int cx = 463 + (25 - cfm.horizontalAdvance("X")) / 2;
    int cy = 8 + (25 - cfm.height()) / 2 + cfm.ascent();

    painter.setPen(QColor(255, 255, 255, 180));
    painter.drawText(cx, cy + 1, "X");
    painter.setPen(QColor(40, 45, 50));
    painter.drawText(cx, cy, "X");

    drawGlossyButton(painter, 45, 25, 75, "PLAY", QColor(0, 150, 255, 200));
    drawGlossyButton(painter, 140, 130, 40, "PREV", QColor(0, 180, 255, 180));
    drawGlossyButton(painter, 200, 130, 40, "NEXT", QColor(0, 180, 255, 180));
    drawGlossyButton(painter, 260, 130, 40, "EJECT", QColor(0, 180, 255, 180));

    int progX = 140, progY = 105, progW = 320, progH = 8;
    painter.setBrush(QColor(0, 0, 0, 180));
    painter.drawRoundedRect(progX, progY, progW, progH, 4, 4);

    int currentProgress = progW * progress;
    QLinearGradient progFill(progX, progY, progX, progY + progH);
    progFill.setColorAt(0, QColor(0, 255, 200));
    progFill.setColorAt(1, QColor(0, 100, 150));
    painter.setBrush(progFill);
    painter.drawRoundedRect(progX, progY, currentProgress, progH, 4, 4);

    QLinearGradient thumbGrad(progX + currentProgress - 6, progY - 2, progX + currentProgress - 6, progY + progH + 2);
    thumbGrad.setColorAt(0, Qt::white);
    thumbGrad.setColorAt(1, Qt::gray);
    painter.setBrush(thumbGrad);
    painter.drawEllipse(progX + currentProgress - 6, progY - 2, 12, 12);

    int volX = 350, volY = 148, volW = 110, volH = 6;
    QPolygonF spk;
    spk << QPointF(325, 148) << QPointF(330, 148) << QPointF(336, 143)
        << QPointF(336, 159) << QPointF(330, 154) << QPointF(325, 154);

    painter.translate(0, 1);
    painter.setBrush(QColor(255, 255, 255, 180));
    painter.drawPolygon(spk);
    painter.setPen(QPen(QColor(255, 255, 255, 180), 1.5));
    painter.drawArc(335, 146, 6, 10, -60 * 16, 120 * 16);
    painter.drawArc(333, 142, 12, 18, -60 * 16, 120 * 16);
    painter.translate(0, -1);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(40, 45, 50));
    painter.drawPolygon(spk);
    painter.setPen(QPen(QColor(40, 45, 50), 1.5));
    painter.drawArc(335, 146, 6, 10, -60 * 16, 120 * 16);
    painter.drawArc(333, 142, 12, 18, -60 * 16, 120 * 16);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 180));
    painter.drawRoundedRect(volX, volY, volW, volH, 3, 3);

    int currentVol = volW * volume;
    QLinearGradient volFill(volX, volY, volX, volY + volH);
    volFill.setColorAt(0, QColor(0, 150, 255));
    volFill.setColorAt(1, QColor(0, 50, 150));
    painter.setBrush(volFill);
    painter.drawRoundedRect(volX, volY, currentVol, volH, 3, 3);

    QLinearGradient vThumb(volX + currentVol - 5, volY - 2, volX + currentVol - 5, volY + volH + 2);
    vThumb.setColorAt(0, Qt::white);
    vThumb.setColorAt(1, Qt::gray);
    painter.setBrush(vThumb);
    painter.drawEllipse(volX + currentVol - 5, volY - 2, 10, 10);
}