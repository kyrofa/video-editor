#ifndef AUDIO_TRANSLATOR_H
#define AUDIO_TRANSLATOR_H

#include <QIODevice>

struct AVFrame;

class AudioPresenter : public QIODevice
{
	Q_OBJECT

	public:
		explicit AudioPresenter(QObject *parent = 0);
		~AudioPresenter();

		bool present(const AVFrame &frame);
		void setChannelCount(int channelCount);

		virtual qint64 readData(char *data, qint64 maxlen);
		virtual qint64 writeData(const char *data, qint64 maxlen);

		virtual qint64 bytesAvailable();

	private:
		int m_channelCount;
		QByteArray m_buffer;
};

#endif // AUDIO_TRANSLATOR_H
