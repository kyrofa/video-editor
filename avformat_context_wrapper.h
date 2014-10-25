#ifndef AVFORMAT_CONTEXT_WRAPPER_H
#define AVFORMAT_CONTEXT_WRAPPER_H

#include <QObject>

struct AVFormatContext;
struct AVStream;
struct AVCodec;

class AVPacketWrapper;
class AVFormatContextDeleter;

class AVFormatContextWrapper : public QObject
{
	Q_OBJECT

	public:
		explicit AVFormatContextWrapper(QObject *parent = 0);
		~AVFormatContextWrapper();

		bool open(const QString &filePath);
		void close();

		bool getBestAudioStream(AVCodec *&audioCodec, AVStream *&audioStream);
		bool getBestVideoStream(AVCodec *&videoCodec, AVStream *&videoStream);

		bool readFrame(AVPacketWrapper &packet);

		void printDebugInformation();

	private:
		bool m_opened;
		QString m_filePath;
		QScopedPointer<AVFormatContext, AVFormatContextDeleter> m_formatContext;
};

#endif // AVFORMAT_CONTEXT_WRAPPER_H
