#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QAudioDeviceInfo>
#include <QAudioOutput>
#include <QAudioFormat>

#include <QMainWindow>

namespace Ui
{
	class MainWindow;
}

struct AVFrame;
struct AVStream;
struct AVFormatContext;

class VideoItem;
class AudioPresenter;

class MainWindow : public QMainWindow
{
		Q_OBJECT

	public:
		explicit MainWindow(QWidget *parent = 0);
		~MainWindow();

	private slots:
		void on_goButton_clicked();

		void on_stopButton_clicked();

	private:
		Ui::MainWindow *m_ui;
		VideoItem *m_videoItem;
		int64_t m_lastPresentationTime;

		QAudioDeviceInfo m_device;
		AudioPresenter *m_audioPresenter;
		QAudioOutput *m_audioOutput;
		QIODevice *m_output;
		QAudioFormat m_format;

		bool m_stop;
};

#endif // MAIN_WINDOW_H
