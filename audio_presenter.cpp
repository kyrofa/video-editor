extern "C"
{
// All the libav headers go here
#include <libavcodec/avcodec.h>
}

#include "audio_presenter.h"

AudioPresenter::AudioPresenter(QObject *parent) :
   QIODevice(parent)
{
	open(QIODevice::ReadOnly);
}

AudioPresenter::~AudioPresenter()
{
	close();
}

bool AudioPresenter::present(const AVFrame &frame)
{
	return false;
}

qint64 AudioPresenter::readData(char *data, qint64 maxlen)
{
	qint64 total = 0;

	if (!m_buffer.isEmpty())
	{
		total = qMin(static_cast<qint64>(m_buffer.size()), maxlen);
		memcpy(data, m_buffer.constData(), total);
		m_buffer.remove(0, total);
	}

	return total;
}

qint64 AudioPresenter::writeData(char *data, qint64 maxlen)
{
	Q_UNUSED(data);
	Q_UNUSED(maxlen);

	return 0;
}

qint64 AudioPresenter::bytesAvailable()
{
	return m_buffer.size() + QIODevice::bytesAvailable();
}
