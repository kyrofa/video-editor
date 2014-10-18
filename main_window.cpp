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

//static AVStream* addVideoStream(AVFormatContext &formatContext, CodecID codecId)
//{
//	AVStream *stream = avformat_new_stream(&formatContext, NULL);
//	if (stream)
//	{
//		AVCodecContext *codecContext = stream->codec;
//		codecContext->codec_id = codecId;
//		codecContext->codec_type = AVMEDIA_TYPE_VIDEO;

//		/* put sample parameters */
//		codecContext->bit_rate = 8818;
//		/* resolution must be a multiple of two */
//		codecContext->width = 1920;
//		codecContext->height = 1080;
//		/* time base: this is the fundamental unit of time (in seconds) in terms
//		 of which frame timestamps are represented. for fixed-fps content,
//		 timebase should be 1/framerate and timestamp increments should be
//		 identically 1. */
//		codecContext->time_base.den = 12;
//		codecContext->time_base.num = 1;
//		codecContext->gop_size = 12; /* emit one intra frame every twelve frames at most */
//		codecContext->pix_fmt = PIX_FMT_YUV420P;
//		if (codecContext->codec_id == CODEC_ID_MPEG2VIDEO)
//		{
//			/* just for testing, we also add B frames */
//			codecContext->max_b_frames = 2;
//		}

//		if (codecContext->codec_id == CODEC_ID_MPEG1VIDEO)
//		{
//			/* Needed to avoid using macroblocks in which some coeffs overflow.
//			  This does not happen with normal video, it just happens here as
//			  the motion of the chroma plane does not match the luma plane. */
//			codecContext->mb_decision=2;
//		}
//		// some formats want stream headers to be separate
//		if (formatContext.oformat->flags & AVFMT_GLOBALHEADER)
//		{
//			codecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
//		}
//	}
//	else
//	{
//		qCritical("Unable to allocate new video stream!");
//	}

//	return stream;
//}

//static AVStream* addAudioStream(AVFormatContext &formatContext, CodecID codecId)
//{
//	AVStream *stream = avformat_new_stream(&formatContext, NULL);
//	if (stream)
//	{
//		stream->id = 1;
//		AVCodecContext *codecContext = stream->codec;
//		codecContext->codec_id = codecId;
//		codecContext->codec_type = AVMEDIA_TYPE_AUDIO;

//		/* put sample parameters */
//		codecContext->sample_fmt = AV_SAMPLE_FMT_S16;
//		codecContext->bit_rate = 96000;
//		codecContext->sample_rate = 48000;
//		codecContext->channels = 2;

//		// some formats want stream headers to be separate
//		if (formatContext.oformat->flags & AVFMT_GLOBALHEADER)
//		{
//			codecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
//		}
//	}
//	else
//	{
//		qCritical("Unable to allocate new audio stream!");
//	}

//	return stream;
//}

//static AVFrame *newPicture(PixelFormat pixelFormat, int width, int height)
//{
//	AVFrame *picture;
//	quint8 *pictureBuffer;
//	int size;

//	picture = avcodec_alloc_frame();
//	if (picture)
//	{
//		size = avpicture_get_size(pixelFormat, width, height);
//		pictureBuffer = static_cast<quint8*>(av_malloc(size));
//		if (pictureBuffer)
//		{
//			avpicture_fill(reinterpret_cast<AVPicture*>(picture), pictureBuffer,
//								pixelFormat, width, height);
//		}
//		else
//		{
//			av_free(picture);
//			picture = NULL;
//		}
//	}

//	return picture;
//}

MainWindow::MainWindow(QWidget *parent) :
   QMainWindow(parent),
   m_ui(new Ui::MainWindow),
   m_videoItem(new VideoItem),
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
	AVCodecContext *videoCodecContext = formatContext->streams[videoCodecIndex]->codec;

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
	AVCodecContext *audioCodecContext = formatContext->streams[audioCodecIndex]->codec;

	returnCode = avcodec_open2(audioCodecContext, audioCodec, NULL);
	if (returnCode < 0)
	{
		char buffer[1024];
		av_strerror(returnCode, buffer, 1024);
		qCritical("Unable to open audio codec: %s", buffer);

		avformat_close_input(&formatContext);
		return;
	}

	//	AVPacket packet;
	//	av_init_packet(&packet);
	//	packet.data = NULL;

	//	// Read frames
	//	AVFrame *frame = avcodec_alloc_frame();
	//	size_t offsetInData = 0;
	//	while (!m_stop)
	//	{
	//		qDebug() << offsetInData << packet.size;
	//		qDebug() << __LINE__;
	//		if (offsetInData >= packet.size)
	//		{
	//			qDebug() << __LINE__;
	//			if (av_read_frame(formatContext, &packet) < 0)
	//			{
	//				qDebug() << __LINE__;
	//				break;
	//			}
	//			qDebug() << __LINE__;
	//		}

	//		qDebug() << __LINE__;
	//		// Is this a packet from the video stream?
	//		if (packet.stream_index == videoCodecIndex)
	//		{
	//			qDebug() << __LINE__;
	//			AVPacket packetToSend;
	//			packetToSend.data = packet.data + offsetInData;
	//			packetToSend.size = packet.size - offsetInData;

	//			qDebug() << __LINE__;
	//			// sending data to libavcodec
	//			int isFrameAvailable = 0;
	//			const int processedLength = avcodec_decode_video2(videoCodecContext, frame, &isFrameAvailable, &packetToSend);
	//			if (processedLength < 0)
	//			{
	//				qDebug() << __LINE__;
	//				av_free_packet(&packet);
	//				avformat_close_input(&formatContext);
	//				qCritical("Unable to decode frame from packet!");
	//				return;
	//			}

	//			qDebug() << __LINE__;
	//			offsetInData += processedLength;

	//			// Did we get a video frame?
	//			qDebug() << __LINE__;
	//			if (isFrameAvailable)
	//			{
	//				//qDebug() << "Got frame!" << ++counter;
	//				qDebug() << __LINE__;
	//				m_ui->widget->setFrame(*frame);
	//			}
	//			qDebug() << __LINE__;
	//		}

	//		// Free the packet that was allocated by av_read_frame
	//		av_free_packet(&packet);
	//		qDebug() << __LINE__;
	//		qApp->processEvents();
	//	}

	AVPacket packet;
	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;

	// Read frames
	AVFrame *frame = avcodec_alloc_frame();
	while (!m_stop && (av_read_frame(formatContext, &packet) >= 0))
	{
		int offsetInData = 0;

		// Is this a packet from the video stream?
		if (packet.stream_index == videoCodecIndex)
		{
			while (!m_stop && offsetInData < packet.size)
			{
				AVPacket packetToSend;
				av_init_packet(&packetToSend);
				packetToSend.data = packet.data + offsetInData;
				packetToSend.size = packet.size - offsetInData;
				// Decode video frame
				int gotFrame = 0;
				int processedLength = avcodec_decode_video2(videoCodecContext, frame,
				                                            &gotFrame, &packetToSend);
				if (processedLength < 0)
				{
					av_free_packet(&packet);
					avformat_close_input(&formatContext);
					qCritical("Unable to decode frame from packet!");
					return;
				}

				offsetInData += processedLength;

				// Did we get a video frame?
				if (gotFrame)
				{
					m_videoItem->present(*frame);

					const int msToWait = videoCodecContext->ticks_per_frame * 1000 * videoCodecContext->time_base.num / videoCodecContext->time_base.den;

					QEventLoop loop;
					QTimer::singleShot(msToWait, &loop, SLOT(quit()));
					loop.exec();
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

	//	AVOutputFormat *format = av_guess_format(NULL, filename.toUtf8().constData(), NULL);
	//	if (!format)
	//	{
	//		qWarning("Unable to deduce output format from file extension; using MPEG.");
	//		format = av_guess_format("mpeg", NULL, NULL);
	//	}

	//	if (!format)
	//	{
	//		qCritical("Unable to find suitable output format!");
	//		return;
	//	}

	//	AVFormatContext *context = avformat_alloc_context();
	//	if (!context)
	//	{
	//		qCritical("Unable to allocate format context!");
	//		return;
	//	}

	//	context->oformat = format;
	//	strncpy(context->filename, filename.toUtf8().constData(),
	//			  sizeof(context->filename));

	//	AVStream *videoStream = NULL;
	//	AVStream *audioStream = NULL;

	//	if (format->video_codec != CODEC_ID_NONE)
	//	{
	//		videoStream = addVideoStream(*context, format->video_codec);
	//	}

	//	if (format->audio_codec != CODEC_ID_NONE)
	//	{
	//		audioStream = addAudioStream(*context, format->audio_codec);
	//	}

	//	/* set the output parameters (must be done even if no
	//			 parameters). */
	//	if (av_set_parameters(context, NULL) < 0) {
	//		qCritical("Invalid output format parameters!");
	//		return;
	//	}

	//av_dump_format(context, 0, filename.toUtf8().constData(), 1);

	//	/* now that all the parameters are set, we can open the audio and
	//			 video codecs and allocate the necessary encode buffers */
	//	if (videoStream)
	//	{
	//		openVideo(*context, *videoStream);
	//	}

	//	if (audioStream)
	//	{
	//		openAudio(*context, *audioStream);
	//	}

	//	/* open the output file, if needed */
	//	if (!(format->flags & AVFMT_NOFILE))
	//	{
	//		if (avio_open(&context->pb, filename.toUtf8().constBegin(), AVIO_FLAG_WRITE) < 0)
	//		{
	//			qCritical("Unable to open file: %s", filename.toUtf8().constData());
	//			return;
	//		}
	//	}

	//	/* write the stream header, if any */
	//	av_write_header(context);

	//	double audioPoints;
	//	double videoPoints;

	//	while (true)
	//	{
	//		/* compute current audio and video time */
	//		if (audioStream)
	//		{
	//			audioPoints = (double)audioStream->pts.val * audioStream->time_base.num / audioStream->time_base.den;
	//		}
	//		else
	//		{
	//			audioPoints = 0.0;
	//		}

	//		if (videoStream)
	//		{
	//			videoPoints = (double)videoStream->pts.val * videoStream->time_base.num / videoStream->time_base.den;
	//		}
	//		else
	//		{
	//			videoPoints = 0.0;
	//		}

	//		if ((!audioStream || audioPoints >= 5) &&
	//			 (!videoStream || videoPoints >= 5))
	//		{
	//			break;
	//		}

	//		/* write interleaved audio and video frames */
	//		if (!videoStream || (videoStream && audioStream && audioPoints < videoPoints))
	//		{
	//			writeAudioFrame(context, audioStream);
	//		}
	//		else
	//		{
	//			writeVideoFrame(context, videoStream);
	//		}
	//	}

	//	/* write the trailer, if any.  the trailer must be written
	//	 * before you close the CodecContexts open when you wrote the
	//	 * header; otherwise write_trailer may try to use memory that
	//	 * was freed on av_codec_close() */
	//	av_write_trailer(context);

	//	/* close each codec */
	//	if (videoStream)
	//	{
	//		close_video(context, videoStream);
	//	}

	//	if (audioStream)
	//	{
	//		close_audio(context, audioStream);
	//	}

	//	/* free the streams */
	//	for (unsigned int i = 0; i < context->nb_streams; ++i)
	//	{
	//		av_freep(&context->streams[i]->codec);
	//		av_freep(&context->streams[i]);
	//	}

	//	if (!(format->flags & AVFMT_NOFILE))
	//	{
	//		/* close the output file */
	//		avio_close(context->pb);
	//	}

	//	/* free the stream */
	//	av_free(context);

	//	AVPacket avpkt;

	//	av_init_packet(&avpkt);

	//	AVCodec *codec = avcodec_find_decoder(CODEC_ID_MP2);
	//	if (!codec)
	//	{
	//		qFatal("Unable to load MP2 codec!");
	//		return;
	//	}

	//	AVCodecContext *context = avcodec_alloc_context3(codec);
	//	if (!context)
	//	{
	//		qFatal("Unable to allocate context!");
	//		return;
	//	}

	//	if (avcodec_open(context, codec) < 0)
	//	{
	//		qFatal("Unable to open codec!");
	//		return;
	//	}
}

//void MainWindow::openVideo(AVFormatContext &context, AVStream &stream)
//{
//	AVCodecContext *newContext = stream.codec;

//	/* find the video encoder */
//	AVCodec *codec = avcodec_find_encoder(newContext->codec_id);
//	if (!codec)
//	{
//		qCritical("Unable to find video codec!");
//		return;
//	}

//	/* open the codec */
//	if (avcodec_open(newContext, codec) < 0)
//	{
//		qCritical("Unable to open video codec!");
//		return;
//	}

//	m_videoBuffer = NULL;
//	if (!(context.oformat->flags & AVFMT_RAWPICTURE))
//	{
//		/* allocate output buffer */
//		/* XXX: API change will be done */
//		/* buffers passed into lav* can be allocated any way you prefer,
//			  as long as they're aligned enough for the architecture, and
//			  they're freed appropriately (such as using av_free for buffers
//			  allocated with av_malloc) */
//		m_videoBufferSize = 200000;
//		m_videoBuffer = static_cast<quint8*>(av_malloc(m_videoBufferSize));
//	}

//	/* allocate the encoded raw picture */
//	m_picture = newPicture(newContext->pix_fmt, newContext->width, newContext->height);
//	if (!m_picture)
//	{
//		qCritical("Unable to allocate picture!");
//		return;
//	}

//	/* if the output format is not YUV420P, then a temporary YUV420P
//			  picture is needed too. It is then converted to the required
//			  output format */
//	m_tmpPicture = NULL;
//	if (newContext->pix_fmt != PIX_FMT_YUV420P)
//	{
//		m_tmpPicture = newPicture(PIX_FMT_YUV420P, newContext->width, newContext->height);
//		if (!m_tmpPicture)
//		{
//			qCritical("Unable to allocate temporary picture!");
//			return;
//		}
//	}
//}

//void MainWindow::openAudio(AVFormatContext &formatContext, AVStream &stream)
//{
//	AVCodecContext *codecContext = stream.codec;

//	/* find the audio encoder */
//	AVCodec *codec = avcodec_find_encoder(codecContext->codec_id);
//	if (!codec)
//	{
//		qCritical("Unable to find audio codec!");
//		return;
//	}

//	/* open it */
//	if (avcodec_open(codecContext, codec) < 0)
//	{
//		qCritical("Unable to open audio codec!");
//		return;
//	}

//	/* init signal generator */
//	m_t = 0;
//	m_tincr = 2 * M_PI * 110.0 / codecContext->sample_rate;
//	/* increment frequency by 110 Hz per second */
//	m_tincr2 = 2 * M_PI * 110.0 / codecContext->sample_rate / codecContext->sample_rate;

//	m_audioBufferSize = 10000;
//	m_audioBuffer = static_cast<quint8*>(av_malloc(m_audioBufferSize));

//	/* ugly hack for PCM codecs (will be removed ASAP with new PCM
//		 support to compute the input frame size in samples */
//	if (codecContext->frame_size <= 1)
//	{
//		m_audioFrameSize = m_audioBufferSize / codecContext->channels;
//		switch(stream.codec->codec_id)
//		{
//			case CODEC_ID_PCM_S16LE:
//			case CODEC_ID_PCM_S16BE:
//			case CODEC_ID_PCM_U16LE:
//			case CODEC_ID_PCM_U16BE:
//				m_audioFrameSize >>= 1;
//				break;
//			default:
//				break;
//		}
//	}
//	else
//	{
//		m_audioFrameSize = codecContext->frame_size;
//	}

//	m_samples = static_cast<quint16*>(av_malloc(m_audioFrameSize * 2 * codecContext->channels));
//}

//void MainWindow::writeAudioFrame(AVFormatContext &formatContext, AVStream &stream)
//{
//	AVPacket packet;
//	av_init_packet(&packet);

//	AVCodecContext *codecContext = stream.codec;

//	getAudioFrame(codecContext->channels);

//	packet.size = avcodec_encode_audio(codecContext, m_audioBuffer, m_audioBufferSize, reinterpret_cast<qint16*>(m_samples));

//	if (codecContext->coded_frame && codecContext->coded_frame->pts != AV_NOPTS_VALUE)
//	{
//		packet.pts= av_rescale_q(codecContext->coded_frame->pts, codecContext->time_base, stream.time_base);
//	}

//	packet.flags |= AV_PKT_FLAG_KEY;
//	packet.stream_index = stream.index;
//	packet.data = m_audioBuffer;

//	/* write the compressed frame in the media file */
//	if (av_interleaved_write_frame(&formatContext, &packet) != 0)
//	{
//		fprintf(stderr, "Error while writing audio frame\n");
//		exit(1);
//	}
//}

//void MainWindow::getAudioFrame(int channelCount)
//{
//	quint16 *sample = m_samples;

//	for (int j = 0; j < m_audioFrameSize; ++j)
//	{
//		int tmp = sin(m_t) * 10000;
//		for (int i = 0; i < channelCount; ++i)
//		{
//			*sample++ = tmp;
//		}

//		m_t += m_tincr;
//	}
//}

void MainWindow::on_stopButton_clicked()
{
	m_stop = true;
}
