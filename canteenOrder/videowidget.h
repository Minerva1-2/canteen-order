#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QProcess>
#include <QTimer>
#include <QListWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>

class VideoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoWidget(QWidget *parent = nullptr);
    ~VideoWidget();

    void addDishItem(const QString &name, const QString &desc, const QString &path);

    // [修正] 将 stopVideo 移动到 public，这样主界面切换时可以调用它停止播放
    void stopVideo();

signals:
    void videoStarted();
    void videoStopped();

private slots:
    void onBtnPlayPauseClicked();
    void onBtnStopClicked();
    void onSliderPressed();
    void onSliderReleased();
    void onSliderMoved(int value);
    void updateVideoProgress();
    void onMPlayerReadOutput();
    void onVideoBtnClicked();

private:
    void initUI();
    void initList();
    void playVideo(const QString &path);
    // void stopVideo(); // [删除] 这里原来的声明
    void sendMplayerCommand(const QString &cmd);

private:
    // UI 布局与控件
    QVBoxLayout *mainLayout;
    QListWidget *listDishes;

    // 播放器容器
    QWidget *playerContainer;

    // 上半部分的视频背景区域
    QWidget *videoSurface;

    // 下半部分的控制栏
    QWidget *controlBar;
    QPushButton *btnPlayPause;
    QSlider *seekSlider;
    QLabel *lblTime;

    // MPlayer 逻辑
    QProcess *mplayerProcess;
    QTimer *progressTimer;
    QString currentVideoPath;

    bool m_isPlaying;
    bool m_isPaused;
    bool m_isDragging;
    int m_totalDuration;
};

#endif // VIDEOWIDGET_H
