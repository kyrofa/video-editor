extern "C"
{
// All the libav headers go here
#include <libavutil/mathematics.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <QDebug>
#include <QEventLoop>
#include <QTimer>
#include <QGLWidget>

#include "video_item.h"

#include "main_window.h"
#include "ui_main_window.h"

MainWindow::MainWindow(QWidget *parent) :
   QMainWindow(parent),
   m_ui(new Ui::MainWindow),
   m_videoItem(new VideoItem),
   m_lastPresentationTime(0),
   m_stop(false)
{
	m_ui->setupUi(this);

	// Initialize libavcodec
	av_register_all();

#ifndef QT_NO_OPENGL
	m_ui->graphicsView->setViewport(new QGLWidget);
#endif

	m_ui->graphicsView->setScene(new QGraphicsScene(this));
	m_ui->graphicsView->scene()->addItem(m_videoItem);
}

MainWindow::~MainWindow()
{
	delete m_ui;
}

void MainWindow::on_goButton_clicked()
{
	QString filename("/home/krf/Desktop/VIDEO0055.mp4");

	AVFormatContext *formatContext = NULL;

	int returnCode = avformat_open_input(&formatContext, filename.toUtf8().constData(), NULL, NULL);
	if (returnCode < 0)
	{
		char buffer[1024];
		av_strerror(returnCode, buffer, 1024);
		qCritical("Unable to open input file: %s", buffer);

		return;
	}

	returnCode = avformat_find_stream_info(formatContext, NULL);
	if (returnCode < 0)
	{
		char buffer[1024];
		av_strerror(returnCode, buffer, 1024);
		qCritical("Unable to find stream information: %s", buffer);

		avformat_close_input(&formatContext);
		return;
	}

	// Print information about the input file
	av_dump_format(formatContext, 0, filename.toUtf8().constData(), 0);

	// Find video stream (and associated codec)
	AVCodec *videoCodec = NULL;
	int videoCodecIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1,
	                                          -1, &videoCodec, 0);
	if (videoCodecIndex < 0)
	{
		char buffer[1024];
		av_strerror(videoCodecIndex, buffer, 1024);
		qCritical("Unable to find video stream: %s", buffer);

		avformat_close_input(&formatContext);
		return;
	}

	qDebug() << "Found video stream index:" << videoCodecIndex;
	AVStream *videoStream = formatContext->streams[videoCodecIndex];
	AVCodecContext *videoCodecContext = videoStream->codec;

	returnCode = avcodec_open2(videoCodecContext, videoCodec, NULL);
	if (returnCode < 0)
	{
		char buffer[1024];
		av_strerror(returnCode, buffer, 1024);
		qCritical("Unable to open video codec: %s", buffer);

		avformat_close_input(&formatContext);
		return;
	}

	// Find audio stream (and associated codec)
	AVCodec *audioCodec = NULL;
	int audioCodecIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1,
	                                          -1, &audioCodec, 0);
	if (audioCodecIndex < 0)
	{
		char buffer[1024];
		av_strerror(audioCodecIndex, buffer, 1024);
		qCritical("Unable to find audio stream: %s", buffer);

		avformat_close_input(&formatContext);
		return;
	}

	qDebug() << "Found audio stream index:" << audioCodecIndex;
	AVStream *audioStream = formatContext->streams[audioCodecIndex];
	AVCodecContext *audioCodecContext = audioStream->codec;

	returnCode = avcodec_open2(audioCodecContext, audioCodec, NULL);
	if (returnCode < 0)
	{
		char buffer[1024];
		av_strerror(returnCode, buffer, 1024);
		qCritical("Unable to open audio codec: %s", buffer);

		avformat_close_input(&formatContext);
		return;
	}

	AVPacket packet;
	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;

	// Read frames
	QSharedPointer<AVFrame> frame(avcodec_alloc_frame(), av_free);
	while (!m_stop && (av_read_frame(formatContext, &packet) >= 0))
	{
		int offsetInVideoData = 0;
		int offsetInAudioData = 0;

		// Is this a packet from the video stream?
		if (packet.stream_index == videoCodecIndex)
		{
			while (!m_stop && offsetInVideoData < packet.size)
			{
				AVPacket packetToSend;
				av_init_packet(&packetToSend);
				packetToSend.data = packet.data + offsetInVideoData;
				packetToSend.size = packet.size - offsetInVideoData;
				// Decode video frame
				int gotFrame = 0;
				int processedLength = avcodec_decode_video2(videoCodecContext, frame.data(),
				                                            &gotFrame, &packetToSend);
				if (processedLength < 0)
				{
					av_free_packet(&packet);
					avformat_close_input(&formatContext);
					qCritical("Unable to decode frame from video packet!");
					return;
				}

				offsetInVideoData += processedLength;

				// Did we get a video frame?
				if (gotFrame)
				{
					m_videoItem->present(*frame);
					m_ui->graphicsView->fitInView(m_ui->graphicsView->scene()->sceneRect(), Qt::KeepAspectRatio);

					if (m_lastPresentationTime > 0)
					{
						int msToWait = round(1000 *
						                     (packet.pts - m_lastPresentationTime) /
						                     videoCodecContext->ticks_per_frame *
						                     static_cast<double>(videoCodecContext->time_base.num) /
						                     static_cast<double>(videoCodecContext->time_base.den));

						QEventLoop loop;
						QTimer::singleShot(msToWait, &loop, SLOT(quit()));
						loop.exec();
					}

					m_lastPresentationTime = packet.pts;
				}

				av_free_packet(&packetToSend);
			}
		}
		else if (packet.stream_index == audioCodecIndex)
		{
			while (!m_stop && offsetInAudioData < packet.size)
			{
				AVPacket packetToSend;
				av_init_packet(&packetToSend);
				packetToSend.data = packet.data + offsetInAudioData;
				packetToSend.size = packet.size - offsetInAudioData;
				// Decode video frame
				int gotFrame = 0;
				int processedLength = avcodec_decode_audio4(audioCodecContext, frame.data(),
				                                            &gotFrame, &packetToSend);
				if (processedLength < 0)
				{
					av_free_packet(&packet);
					avformat_close_input(&formatContext);
					qCritical("Unable to decode frame from audio packet!");
					return;
				}

				offsetInAudioData += processedLength;

				// Did we get an audio frame?
				if (gotFrame)
				{
					// Do something with it...
//					m_videoItem->present(*frame);
//					m_ui->graphicsView->fitInView(m_ui->graphicsView->scene()->sceneRect(), Qt::KeepAspectRatio);

//					if (m_lastPresentationTime > 0)
//					{
//						int msToWait = round(1000 *
//						                     (packet.pts - m_lastPresentationTime) /
//						                     videoCodecContext->ticks_per_frame *
//						                     static_cast<double>(videoCodecContext->time_base.num) /
//						                     static_cast<double>(videoCodecContext->time_base.den));

//						QEventLoop loop;
//						QTimer::singleShot(msToWait, &loop, SLOT(quit()));
//						loop.exec();
//					}

//					m_lastPresentationTime = packet.pts;
				}

				av_free_packet(&packetToSend);
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
		qApp->processEvents();
	}

	qDebug() << "Done!";

	avformat_close_input(&formatContext);
}

void MainWindow::on_stopButton_clicked()
{
	m_stop = true;
}
