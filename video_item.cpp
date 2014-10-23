extern "C"
{
// All the libav headers go here
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <QDebug>
#include <QVideoSurfaceFormat>
#include <QPainter>

#include "video_item.h"

VideoItem::VideoItem(QGraphicsItem *parent) :
   QGraphicsItem(parent),
   m_imageFormat(QImage::Format_Invalid),
   m_framePainted(true)
{
}

QList<QVideoFrame::PixelFormat> VideoItem::supportedPixelFormats(
      QAbstractVideoBuffer::HandleType handleType) const
{
	switch (handleType)
	{
		case QAbstractVideoBuffer::NoHandle:
			return QList<QVideoFrame::PixelFormat>()
			      << QVideoFrame::Format_RGB32
			      << QVideoFrame::Format_ARGB32_Premultiplied
			      << QVideoFrame::Format_ARGB32
			      << QVideoFrame::Format_RGB565
			      << QVideoFrame::Format_RGB555;
		default:
			return QList<QVideoFrame::PixelFormat>();
	}
}

QRectF VideoItem::boundingRect() const
{
	return QRectF(QPointF(0,0), surfaceFormat().sizeHint());
}

void VideoItem::paint(QPainter *painter,
                      const QStyleOptionGraphicsItem *option,
                      QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	if (m_currentFrame.map(QAbstractVideoBuffer::ReadOnly))
	{
		const QTransform oldTransform = painter->transform();

		if (surfaceFormat().scanLineDirection() == QVideoSurfaceFormat::BottomToTop)
		{
			painter->scale(1, -1);
			painter->translate(0, -boundingRect().height());
		}

		painter->drawImage(boundingRect(), QImage(
		                      m_currentFrame.bits(),
		                      m_imageSize.width(),
		                      m_imageSize.height(),
		                      m_imageFormat));

		painter->setTransform(oldTransform);

		m_framePainted = true;

		m_currentFrame.unmap();
	}
}

bool VideoItem::start(const QVideoSurfaceFormat &format)
{
	if (isFormatSupported(format))
	{
		m_imageFormat = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
		m_imageSize = format.frameSize();
		m_framePainted = true;

		QAbstractVideoSurface::start(format);

		prepareGeometryChange();

		return true;
	}
	else
	{
		return false;
	}
}

void VideoItem::stop()
{
	m_currentFrame = QVideoFrame();
	m_framePainted = false;

	QAbstractVideoSurface::stop();
}

bool VideoItem::present(const QVideoFrame &frame)
{
	if (!m_framePainted)
	{
		if (!QAbstractVideoSurface::isActive())
		{
			setError(StoppedError);
		}
	}
	else
	{
		if (frame.isValid())
		{
			QVideoSurfaceFormat currentFormat = surfaceFormat();

			if (frame.pixelFormat() != currentFormat.pixelFormat() ||
			    frame.size() != currentFormat.frameSize())
			{
				QVideoSurfaceFormat format(frame.size(), frame.pixelFormat());

				if (!start(format))
				{
					return false;
				}
			}

			m_currentFrame = frame;
			m_framePainted = false;

			update();

			return true;
		}
	}

	stop();
	return false;
}

bool VideoItem::present(const AVFrame &frame)
{
	int width = frame.width;
	int height = frame.height;

	QSharedPointer<AVFrame> rgbFrame(avcodec_alloc_frame(), av_free);
	int numBytes= avpicture_get_size(PIX_FMT_RGB24,
	                                 width, height);
	quint8 buffer[numBytes];

	avpicture_fill((AVPicture*)rgbFrame.data(), buffer, PIX_FMT_RGB24,
	               width, height);

	int dst_fmt = PIX_FMT_RGB24;

	// TODO: cache following conversion context for speedup,
	//       and recalculate only on dimension changes
	SwsContext *img_convert_ctx_temp = sws_getContext(
	                                      width, height,
	                                      static_cast<PixelFormat>(frame.format),
	                                      width, height, (PixelFormat)dst_fmt,
	                                      SWS_BICUBIC, NULL, NULL, NULL);

	sws_scale(img_convert_ctx_temp,
	          frame.data, frame.linesize, 0, height,
	          rgbFrame->data,
	          rgbFrame->linesize);

	QImage image(width, height, QImage::Format_RGB32);

	unsigned char *src = static_cast<unsigned char*>(rgbFrame->data[0]);
	for (int y = 0; y < height; y++)
	{
		QRgb *scanLine = reinterpret_cast<QRgb*>(image.scanLine(y));
		for (int x = 0; x < width; x++)
		{
			scanLine[x] = qRgb(src[3*x], src[3*x+1], src[3*x+2]);
		}
		src += rgbFrame->linesize[0];
	}

	QVideoFrame videoFrame(image);

	return present(videoFrame);
}
