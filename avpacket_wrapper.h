#ifndef AVPACKET_WRAPPER_H
#define AVPACKET_WRAPPER_H

#include <QScopedPointer>
#include <QSharedPointer>
#include <QExplicitlySharedDataPointer>

struct AVFormatContext;
struct AVCodecContext;

class AVPacketWrapperPrivate;

class AVPacketWrapper
{
	public:
		AVPacketWrapper();
		AVPacketWrapper(const AVPacketWrapper &other, size_t offset = 0);
		~AVPacketWrapper();

		bool readFrame(AVFormatContext *formatContext);

		size_t size() const;
		int streamIndex() const;
		qint64 presentationTime() const;

		bool decodeAudioFrame(AVCodecContext *codecContext,
		                      QSharedPointer<AVFrame> &frame,
		                      int &decodedBytes);

		bool decodeVideoFrame(AVCodecContext *codecContext,
		                      QSharedPointer<AVFrame> &frame,
		                      int &decodedBytes);

		void detach();

	private:
		QExplicitlySharedDataPointer<AVPacketWrapperPrivate> m_data;
};

Q_DECLARE_TYPEINFO(AVPacketWrapper, Q_MOVABLE_TYPE);

#endif // AVPACKET_WRAPPER_H
