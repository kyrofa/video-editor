#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

namespace Ui
{
	class MainWindow;
}

struct AVFrame;
struct AVStream;
struct AVFormatContext;

class VideoItem;

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
		bool m_stop;
};

#endif // MAIN_WINDOW_H
