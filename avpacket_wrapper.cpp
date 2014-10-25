extern "C"
{
// libav headers go here
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <QDebug>
#include <QSharedData>

#include "avpacket_wrapper.h"

////////// BEGIN PRIVATE DATA //////////

class AVPacketWrapperPrivate: public QSharedData
{
	public:
		AVPacketWrapperPrivate()
		{
			av_init_packet(&packet);
			packet.data = NULL;
			packet.size = 0;
		}

		AVPacketWrapperPrivate(const AVPacketWrapperPrivate &other) :
		   QSharedData(other),
		   packet(other.packet)
		{
			av_dup_packet(&packet); // Create a deep copy of the packet data
		}

		~AVPacketWrapperPrivate()
		{
			av_free_packet(&packet);
		}

		AVPacket packet;
};

////////// END PRIVATE DATA //////////

AVPacketWrapper::AVPacketWrapper() :
   m_data(new AVPacketWrapperPrivate)
{
}

AVPacketWrapper::AVPacketWrapper(const AVPacketWrapper &other, size_t offset) :
   m_data(other.m_data)
{
	m_data->packet.data += offset;
	m_data->packet.size -= offset;
}

AVPacketWrapper::~AVPacketWrapper()
{
}

bool AVPacketWrapper::readFrame(AVFormatContext *formatContext)
{
	if (formatContext)
	{
		return (av_read_frame(formatContext, &m_data->packet) >= 0);
	}

	return false;
}

size_t AVPacketWrapper::size() const
{
	return m_data->packet.size;
}

int AVPacketWrapper::streamIndex() const
{
	return m_data->packet.stream_index;
}

qint64 AVPacketWrapper::presentationTime() const
{
	return m_data->packet.pts;
}

bool AVPacketWrapper::decodeAudioFrame(AVCodecContext *codecContext,
                                       QSharedPointer<AVFrame> &frame,
                                       int &decodedBytes)
{
	decodedBytes = -1;
	if (codecContext)
	{
		int gotFrame = 0;
		decodedBytes = avcodec_decode_audio4(codecContext, frame.data(),
		                                     &gotFrame, &m_data->packet);
		return gotFrame;
	}

	qCritical("Invalid codec context! Cannot decode audio frame.");
	return false;
}

bool AVPacketWrapper::decodeVideoFrame(AVCodecContext *codecContext,
                                       QSharedPointer<AVFrame> &frame,
                                       int &decodedBytes)
{
	decodedBytes = -1;
	if (codecContext)
	{
		int gotFrame = 0;
		decodedBytes = avcodec_decode_video2(codecContext, frame.data(),
		                                     &gotFrame, &m_data->packet);
		return gotFrame;
	}

	qCritical("Invalid codec context! Cannot decode video frame.");
	return false;
}

void AVPacketWrapper::detach()
{
	m_data.detach();
}
