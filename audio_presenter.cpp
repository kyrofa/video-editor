extern "C"
{
// All the libav headers go here
#include <libavcodec/avcodec.h>
}

#include <QDebug>

#include "audio_presenter.h"

AudioPresenter::AudioPresenter(QObject *parent) :
   QIODevice(parent),
   m_channelCount(1)
{
	open(QIODevice::ReadOnly);
}

AudioPresenter::~AudioPresenter()
{
	close();
}

void AudioPresenter::setChannelCount(int channelCount)
{
	m_channelCount = channelCount;
}

bool AudioPresenter::present(const AVFrame &frame)
{
	for (int i = 0; i < m_channelCount; ++i)
	{
		m_buffer.append(reinterpret_cast<char*>(frame.extended_data[i]),
		                frame.linesize[0]);
	}

	return true;
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

qint64 AudioPresenter::writeData(const char *data, qint64 maxlen)
{
	Q_UNUSED(data);
	Q_UNUSED(maxlen);

	return 0;
}

qint64 AudioPresenter::bytesAvailable()
{
	return m_buffer.size() + QIODevice::bytesAvailable();
}
