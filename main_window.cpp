extern "C"
{
// libav headers go here
#include <libavutil/mathematics.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <QDebug>
#include <QEventLoop>
#include <QTimer>
#include <QGLWidget>

#include "audio_presenter.h"
#include "video_item.h"

#include "avformat_context_wrapper.h"
#include "avpacket_wrapper.h"

#include "main_window.h"
#include "ui_main_window.h"

MainWindow::MainWindow(QWidget *parent) :
   QMainWindow(parent),
   m_ui(new Ui::MainWindow),
   m_videoItem(new VideoItem),
   m_lastPresentationTime(0),
   m_device(QAudioDeviceInfo::defaultOutputDevice()),
   m_audioPresenter(new AudioPresenter(this)),
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

	AVFormatContextWrapper formatContext;

	if (!formatContext.open(filename))
	{
		return;
	}

	// Print information about the input file
	formatContext.printDebugInformation();

	// Find video stream (and associated codec)
	AVCodec *videoCodec = NULL;
	AVStream *videoStream = NULL;
	if (!formatContext.getBestVideoStream(videoCodec, videoStream))
	{
		return;
	}

	int videoStreamIndex = videoStream->index;
	AVCodecContext *videoCodecContext = videoStream->codec;

	int returnCode = avcodec_open2(videoCodecContext, videoCodec, NULL);
	if (returnCode < 0)
	{
		char buffer[1024];
		av_strerror(returnCode, buffer, 1024);
		qCritical("Unable to open video codec: %s", buffer);

		return;
	}

	// Find audio stream (and associated codec)
	AVCodec *audioCodec = NULL;
	AVStream *audioStream = NULL;
	if (!formatContext.getBestAudioStream(audioCodec, audioStream))
	{
		return;
	}

	int audioStreamIndex = audioStream->index;
	AVCodecContext *audioCodecContext = audioStream->codec;

	returnCode = avcodec_open2(audioCodecContext, audioCodec, NULL);
	if (returnCode < 0)
	{
		char buffer[1024];
		av_strerror(returnCode, buffer, 1024);
		qCritical("Unable to open audio codec: %s", buffer);

		return;
	}

	qDebug() << "Setting sample rate:" << audioCodecContext->sample_rate;
	m_format.setSampleRate(audioCodecContext->sample_rate);

	qDebug() << "Setting channel count:" << audioCodecContext->channels;
	m_format.setChannelCount(audioCodecContext->channels);
	m_audioPresenter->setChannelCount(audioCodecContext->channels);

	qDebug() << "Setting bits per sample:" << av_get_bytes_per_sample(audioCodecContext->sample_fmt)*8;
	m_format.setSampleSize(av_get_bytes_per_sample(audioCodecContext->sample_fmt)*8);

	m_format.setCodec("audio/pcm");

	m_format.setByteOrder(QAudioFormat::LittleEndian);
	m_format.setSampleType(QAudioFormat::SignedInt);

	QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
	if (!info.isFormatSupported(m_format))
	{
		qWarning("Default format not supported-- attempting to use nearest");
		m_format = info.nearestFormat(m_format);
	}

	m_audioOutput = new QAudioOutput(m_device, m_format, this);
	m_audioOutput->setVolume(1.0);
	m_audioOutput->start(m_audioPresenter);

	AVPacketWrapper packet;

	// Read frames
	QSharedPointer<AVFrame> frame(avcodec_alloc_frame(), av_free);
	while (!m_stop && formatContext.readFrame(packet))
	{
		size_t offsetInVideoData = 0;
		size_t offsetInAudioData = 0;

		// Is this a packet from the video stream?
		if (packet.streamIndex() == videoStreamIndex)
		{
			while (!m_stop && offsetInVideoData < packet.size())
			{
				AVPacketWrapper packetToSend(packet, offsetInVideoData);

				// Decode video frame
				int processedLength = 0;
				bool gotFrame = packetToSend.decodeVideoFrame(videoCodecContext,
				                                              frame,
				                                              processedLength);
				if (processedLength < 0)
				{
					qCritical("Unable to decode frame from video packet!");
					return;
				}

				offsetInVideoData += processedLength;

				// Did we get a video frame?
				if (gotFrame)
				{
					m_videoItem->present(*frame);
					m_ui->graphicsView->fitInView(m_ui->graphicsView->scene()->sceneRect(), Qt::KeepAspectRatio);

					qint64 thisPresentationTime = packet.presentationTime();

					if (m_lastPresentationTime > 0)
					{
						int msToWait = round(1000 *
						                     (thisPresentationTime - m_lastPresentationTime) /
						                     videoCodecContext->ticks_per_frame *
						                     static_cast<double>(videoCodecContext->time_base.num) /
						                     static_cast<double>(videoCodecContext->time_base.den));

						QEventLoop loop;
						QTimer::singleShot(msToWait, &loop, SLOT(quit()));
						loop.exec();
					}

					m_lastPresentationTime = thisPresentationTime;
				}
			}
		}
		else if (packet.streamIndex() == audioStreamIndex)
		{
			while (!m_stop && offsetInAudioData < packet.size())
			{
				AVPacketWrapper packetToSend(packet, offsetInAudioData);

				// Decode video frame
				int processedLength = 0;
				bool gotFrame = packetToSend.decodeAudioFrame(audioCodecContext,
				                                              frame,
				                                              processedLength);
				if (processedLength < 0)
				{
					qCritical("Unable to decode frame from audio packet!");
					return;
				}

				offsetInAudioData += processedLength;

				// Did we get an audio frame?
				if (gotFrame)
				{
					m_audioPresenter->present(*frame);
				}
			}
		}

		qApp->processEvents();
	}

	qDebug() << "Done!";
}

void MainWindow::on_stopButton_clicked()
{
	m_stop = true;
}
