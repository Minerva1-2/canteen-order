#include "videowidget.h"
#include <QApplication>
#include <QDebug>
#include <QScroller>
#include <QFile>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QStringList>
#include <QPalette>
#include <QPainter>

class VideoSurface : public QWidget
{
public:
    explicit VideoSurface(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_OpaquePaintEvent);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_PaintOnScreen);
    }

protected:
    void paintEvent(QPaintEvent *) override {
    }

    QPaintEngine *paintEngine() const override {
        return nullptr;
    }
};

VideoWidget::VideoWidget(QWidget *parent) : QWidget(parent)
{
    // 自身不画背景
    this->setAutoFillBackground(false);
    mplayerProcess = new QProcess(this);
    mplayerProcess->setProcessChannelMode(QProcess::MergedChannels);
    connect(mplayerProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(onMPlayerReadOutput()));
    connect(mplayerProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            [=](int, QProcess::ExitStatus){
        if(m_isPlaying) stopVideo();
    });

    // --- 定时器 ---
    progressTimer = new QTimer(this);
    progressTimer->setInterval(1000);
    connect(progressTimer, SIGNAL(timeout()), this, SLOT(updateVideoProgress()));

    m_isPlaying = false;
    m_isPaused = false;
    m_isDragging = false;
    m_totalDuration = 0;

    initUI();

    // === 测试数据 ===
    addDishItem("招牌鳗鱼饭", "精选新鲜鳗鱼，配以秘制酱汁。", "/workdir/videos/manyu.mp4");
    addDishItem("三文鱼刺身", "挪威直供，新鲜厚切三文鱼。", "/workdir/videos/sashimi.mp4");
    addDishItem("火腿海苔饭团", "精选火腿海苔，配以日本大米。", "/workdir/videos/sushi.mp4");
    addDishItem("日本清酒", "精选清酒，三十年酿造而成。", "/workdir/videos/drink.mp4");
    addDishItem("经典日式豚骨拉面", "大火熬出骨汤，鲜美至极。", "/workdir/videos/romen.mp4");
    addDishItem("铁火卷", "大火慢烤，外焦里内。", "/workdir/videos/sushi2.mp4");
}

VideoWidget::~VideoWidget()
{
    stopVideo();
}

void VideoWidget::initUI()
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ============================================================
    // A. 视频播放容器 (默认隐藏)
    // ============================================================
    playerContainer = new QWidget(this);
    playerContainer->setFixedSize(800, 480);
    playerContainer->hide();

    // 给容器设一个纯黑背景
    // 作用：当隐藏列表、显示容器的瞬间，屏幕变黑，给 MPlayer 准备好画布
    playerContainer->setAutoFillBackground(true);
    QPalette blackPal;
    blackPal.setColor(QPalette::Window, Qt::black);
    playerContainer->setPalette(blackPal);

    // 垂直布局：上视频，下控制
    QVBoxLayout *playerLayout = new QVBoxLayout(playerContainer);
    playerLayout->setContentsMargins(0, 0, 0, 0);
    playerLayout->setSpacing(0);

    // 1. 上半部：使用自定义的 VideoSurface (800x400)
    // ------------------------------------------------------------
    videoSurface = new VideoSurface(playerContainer);
    videoSurface->setFixedSize(800, 400);
    // 注意：这里不需要设置颜色，因为 paintEvent 被屏蔽了，它就是个“透明洞”
    // 实际上显示的是父容器 playerContainer 的黑色背景，直到 MPlayer 覆盖它

    // 2. 下半部：控制栏 (800x80)
    // ------------------------------------------------------------
    controlBar = new QWidget(playerContainer);
    controlBar->setFixedSize(800, 80);
    controlBar->setAutoFillBackground(true);

    QPalette barPal;
    barPal.setColor(QPalette::Window, QColor(35, 35, 35)); // 深灰背景
    controlBar->setPalette(barPal);

    QHBoxLayout *barLayout = new QHBoxLayout(controlBar);
    barLayout->setContentsMargins(20, 10, 20, 10);
    barLayout->setSpacing(15);

    btnPlayPause = new QPushButton("||", controlBar);
    btnPlayPause->setFixedSize(50, 50);
    btnPlayPause->setStyleSheet(
        "QPushButton { background-color: #FFD161; border-radius: 25px; font-weight: bold; font-size: 20px; color: #333; border: none; }"
        "QPushButton:pressed { background-color: #FFC107; }"
    );
    connect(btnPlayPause, SIGNAL(clicked()), this, SLOT(onBtnPlayPauseClicked()));

    seekSlider = new QSlider(Qt::Horizontal, controlBar);
    seekSlider->setFixedHeight(40);
    // Qt5 样式表
    seekSlider->setStyleSheet(
        "QSlider::groove:horizontal { border: 1px solid #444; height: 8px; background: #333; border-radius: 4px; }"
        "QSlider::handle:horizontal { background: #FF8C00; width: 30px; margin: -11px 0; border-radius: 15px; }"
    );
    connect(seekSlider, SIGNAL(sliderPressed()), this, SLOT(onSliderPressed()));
    connect(seekSlider, SIGNAL(sliderReleased()), this, SLOT(onSliderReleased()));
    connect(seekSlider, SIGNAL(valueChanged(int)), this, SLOT(onSliderMoved(int)));

    lblTime = new QLabel("0%", controlBar);
    lblTime->setFixedWidth(50);
    lblTime->setAlignment(Qt::AlignCenter);
    lblTime->setStyleSheet("color: white; font-weight: bold; font-size: 16px;");

    QPushButton *btnStop = new QPushButton("退出", controlBar);
    btnStop->setFixedSize(80, 40);
    btnStop->setStyleSheet(
        "QPushButton { background-color: #D32F2F; color: white; border-radius: 5px; font-weight: bold; border: none; }"
        "QPushButton:pressed { background-color: #B71C1C; }"
    );
    connect(btnStop, SIGNAL(clicked()), this, SLOT(onBtnStopClicked()));

    barLayout->addWidget(btnPlayPause);
    barLayout->addWidget(seekSlider);
    barLayout->addWidget(lblTime);
    barLayout->addWidget(btnStop);

    playerLayout->addWidget(videoSurface);
    playerLayout->addWidget(controlBar);

    // ============================================================
    // B. 菜品列表专页 (默认显示)
    // ============================================================
    initList();

    mainLayout->addWidget(playerContainer);
    mainLayout->addWidget(listDishes);
}

void VideoWidget::initList()
{
    listDishes = new QListWidget(this);
    listDishes->setFrameShape(QFrame::NoFrame);
    listDishes->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    // 给列表一个背景色
    listDishes->setStyleSheet("background-color: #F5F5F5; border: none;");
    QScroller::grabGesture(listDishes, QScroller::LeftMouseButtonGesture);
}

void VideoWidget::playVideo(const QString &path)
{
    if (path.isEmpty()) return;

    emit videoStarted();
    QApplication::processEvents();

    // 1. 切换页面：隐藏列表，显示视频容器
    listDishes->hide();
    playerContainer->show();
    playerContainer->raise();

    m_isPaused = false;
    m_isDragging = false;
    m_totalDuration = 0;

    btnPlayPause->setText("||");
    seekSlider->setValue(0);
    lblTime->setText("0%");

    m_isPlaying = true;
    currentVideoPath = path;

    if (mplayerProcess->state() != QProcess::NotRunning) {
        mplayerProcess->kill();
        mplayerProcess->waitForFinished(300);
    }

    QString mplayerProgram = "mplayer";
    if (QFile::exists("/workdir/mplayer")) mplayerProgram = "/workdir/mplayer";
    else if (QFile::exists("/usr/bin/mplayer")) mplayerProgram = "/usr/bin/mplayer";

    QStringList args;
    // 使用 fbdev 输出
    args << "-slave" << "-quiet" << "-vo" << "fbdev";

    // [坐标参数]
    // MPlayer 负责往 (80,20) 宽高 640x360 的区域画图
    // 这个位置正好对应我们的 VideoSurface 的几何中心
    args << "-x" << "640" << "-y" << "360" << "-geometry" << "80:20";

    args << "-zoom" << "-noborder" << "-cache" << "8192";
    args << path;

    mplayerProcess->start(mplayerProgram, args);

    if(mplayerProcess->waitForStarted(2000)) {
        progressTimer->start(1000);
        sendMplayerCommand("get_time_length");
    } else {
        stopVideo();
    }
}

void VideoWidget::stopVideo()
{
    if(listDishes->isVisible()) return;

    progressTimer->stop();

    if (mplayerProcess->state() != QProcess::NotRunning) {
        mplayerProcess->write("quit\n");
        // 给它一点时间退出，否则可能锁死资源
        if(!mplayerProcess->waitForFinished(300)) mplayerProcess->kill();
    }

    m_isPlaying = false;
    m_isPaused = false;

    // 2. 切换回列表
    playerContainer->hide();
    listDishes->show();

    // 强制刷新列表，覆盖掉可能残留的视频数据
    listDishes->update();

    emit videoStopped();
}

void VideoWidget::onBtnStopClicked() { stopVideo(); }

void VideoWidget::onBtnPlayPauseClicked()
{
    if (mplayerProcess->state() != QProcess::Running) {
        if(!currentVideoPath.isEmpty()) playVideo(currentVideoPath);
        return;
    }
    progressTimer->stop();
    sendMplayerCommand("pause");
    m_isPaused = !m_isPaused;

    if (m_isPaused) {
        btnPlayPause->setText(" ▶ ");
    } else {
        btnPlayPause->setText(" || ");
        progressTimer->start(1000);
    }
}

void VideoWidget::updateVideoProgress()
{
    if (m_isPlaying && !m_isPaused && !m_isDragging) {
        sendMplayerCommand("get_percent_pos");
        if (m_totalDuration <= 0) {
            sendMplayerCommand("get_time_length");
        }
    }
}

void VideoWidget::onMPlayerReadOutput()
{
    while(mplayerProcess->canReadLine()) {
        QByteArray data = mplayerProcess->readLine();
        QString line = QString::fromLatin1(data).trimmed();
        if(line.isEmpty()) continue;

        if(line.startsWith("ANS_PERCENT_POSITION")) {
            QStringList parts = line.split("=");
            if(parts.length() >= 2) {
                bool ok;
                int percent = parts.last().toInt(&ok);
                if (ok && !m_isDragging) {
                    seekSlider->setValue(percent);
                    QString str = QString("%1%").arg(percent);
                    if(lblTime->text() != str) lblTime->setText(str);
                }
            }
        }
        else if (line.startsWith("ANS_LENGTH")) {
            QStringList parts = line.split("=");
            if(parts.length() >= 2) {
                QString valStr = parts.last().trimmed().remove('\'').remove('"');
                bool ok;
                double tempVal = valStr.toDouble(&ok);
                if (ok) {
                    int durationInt = static_cast<int>(tempVal);
                    if (durationInt > 1) {
                        m_totalDuration = durationInt;
                    }
                }
            }
        }
    }
}

void VideoWidget::onSliderPressed()
{
    m_isDragging = true;
    progressTimer->stop();
}

void VideoWidget::onSliderMoved(int value)
{
    QString str = QString("%1%").arg(value);
    if(lblTime->text() != str) lblTime->setText(str);
}

void VideoWidget::onSliderReleased()
{
    int sliderVal = seekSlider->value();
    sendMplayerCommand(QString("seek %1 1").arg(sliderVal));
    m_isDragging = false;
    QTimer::singleShot(1000, [=](){
        if(m_isPlaying && !m_isPaused) progressTimer->start(1000);
    });
}

void VideoWidget::sendMplayerCommand(const QString &cmd)
{
    if (mplayerProcess->state() == QProcess::Running) {
        mplayerProcess->write((cmd + "\n").toLatin1());
    }
}

void VideoWidget::onVideoBtnClicked() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn) playVideo(btn->property("videoPath").toString());
}

void VideoWidget::addDishItem(const QString &name, const QString &desc, const QString &path) {
    QWidget *w = new QWidget;
    w->setStyleSheet("background:white; border-bottom:1px solid #ddd;");
    QHBoxLayout *mainHBox = new QHBoxLayout(w);
    mainHBox->setContentsMargins(15, 15, 15, 15);
    QVBoxLayout *textVBox = new QVBoxLayout();
    QLabel *lblName = new QLabel(name);
    lblName->setStyleSheet("color: #333; font-weight: bold; font-size: 18px;");
    QLabel *lblDesc = new QLabel(desc);
    lblDesc->setStyleSheet("color: #666; font-size: 14px;");
    lblDesc->setWordWrap(true);
    textVBox->addWidget(lblName);
    textVBox->addWidget(lblDesc);
    mainHBox->addLayout(textVBox);
    mainHBox->addStretch();
    if(!path.isEmpty()){
        QPushButton *btn = new QPushButton("▶ 播放");
        btn->setProperty("videoPath", path);
        btn->setFixedSize(80, 40);
        btn->setStyleSheet("background-color: #E3F2FD; color: #1976D2; border: none; border-radius: 4px;");
        connect(btn, SIGNAL(clicked()), this, SLOT(onVideoBtnClicked()));
        mainHBox->addWidget(btn);
    }
    QListWidgetItem *it = new QListWidgetItem(listDishes);
    it->setSizeHint(QSize(600, 100));
    listDishes->setItemWidget(it, w);
}
