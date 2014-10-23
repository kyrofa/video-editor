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

		virtual qint64 readData(char *data, qint64 maxlen);
		virtual qint64 writeData(char *data, qint64 maxlen);

		virtual qint64 bytesAvailable();

	private:
		QByteArray m_buffer;
};

#endif // AUDIO_TRANSLATOR_H
