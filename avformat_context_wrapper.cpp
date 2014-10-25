extern "C"
{
// libav headers go here
#include <libavformat/avformat.h>
}

#include "avpacket_wrapper.h"
#include "avformat_context_wrapper.h"

static const int ERROR_BUFFER_SIZE = 1024;

class AVFormatContextDeleter
{
	public:
		static inline void cleanup(AVFormatContext *pointer)
		{
			if (pointer)
			{
				avformat_close_input(&pointer);
			}
		}
};

AVFormatContextWrapper::AVFormatContextWrapper(QObject *parent) :
   QObject(parent),
   m_opened(false),
   m_filePath(""),
   m_formatContext(NULL)
{
}

AVFormatContextWrapper::~AVFormatContextWrapper()
{
}

bool AVFormatContextWrapper::open(const QString &filePath)
{
	m_opened = false; // Reset the open flag
	m_filePath = filePath;

	AVFormatContext *formatContext = NULL;

	// Open the file
	int returnCode = avformat_open_input(&formatContext,
	                                     filePath.toUtf8().constData(),
	                                     NULL, NULL);
	if (returnCode < 0)
	{
		char buffer[ERROR_BUFFER_SIZE];
		av_strerror(returnCode, buffer, ERROR_BUFFER_SIZE);
		qCritical("Unable to open input file: %s", buffer);

		return false;
	}

	m_formatContext.reset(formatContext);

	// Find all stream information within the file
	returnCode = avformat_find_stream_info(m_formatContext.data(), NULL);
	if (returnCode < 0)
	{
		char buffer[ERROR_BUFFER_SIZE];
		av_strerror(returnCode, buffer, ERROR_BUFFER_SIZE);
		qCritical("Unable to find stream information: %s", buffer);

		return false;
	}

	m_opened = true;
	return true;
}

bool AVFormatContextWrapper::getBestAudioStream(AVCodec *&audioCodec,
                                                AVStream *&audioStream)
{
	if (m_opened)
	{
		int index = av_find_best_stream(m_formatContext.data(),
		                                AVMEDIA_TYPE_AUDIO,
		                                -1, -1, &audioCodec, 0);
		if (index >= 0)
		{
			audioStream = m_formatContext->streams[index];

			return true;
		}

		char buffer[ERROR_BUFFER_SIZE];
		av_strerror(index, buffer, ERROR_BUFFER_SIZE);
		qCritical("Unable to find audio stream: %s", buffer);
	}
	else
	{
		qWarning("AVFormat is not opened! Cannot select audio stream.");
	}

	return false;
}

bool AVFormatContextWrapper::getBestVideoStream(AVCodec *&videoCodec,
                                                AVStream *&videoStream)
{
	if (m_opened)
	{
		int index = av_find_best_stream(m_formatContext.data(),
		                                AVMEDIA_TYPE_VIDEO,
		                                -1, -1, &videoCodec, 0);
		if (index >= 0)
		{
			videoStream = m_formatContext->streams[index];

			return true;
		}

		char buffer[ERROR_BUFFER_SIZE];
		av_strerror(index, buffer, ERROR_BUFFER_SIZE);
		qCritical("Unable to find video stream: %s", buffer);
	}
	else
	{
		qWarning("AVFormat is not opened! Cannot select video stream.");
	}

	return false;
}

bool AVFormatContextWrapper::readFrame(AVPacketWrapper &packet)
{
	return packet.readFrame(m_formatContext.data());
}

void AVFormatContextWrapper::printDebugInformation()
{
	if (m_opened)
	{
		av_dump_format(m_formatContext.data(), 0,
		               m_filePath.toUtf8().constData(), 0);
	}
	else
	{
		qWarning("AVFormat is not opened! Cannot print debug information.");
	}
}
