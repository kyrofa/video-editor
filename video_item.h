#ifndef VIDEO_ITEM_H
#define VIDEO_ITEM_H

#include <QAbstractVideoSurface>
#include <QGraphicsItem>

struct AVFrame;

class VideoItem : public QAbstractVideoSurface, public QGraphicsItem
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)

	public:
		explicit VideoItem(QGraphicsItem *parent = 0);

		virtual QList<QVideoFrame::PixelFormat> supportedPixelFormats(
		      QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const;

		virtual QRectF boundingRect() const;
		virtual void paint(QPainter *painter,
		                   const QStyleOptionGraphicsItem *option,
		                   QWidget *widget);

		virtual bool start(const QVideoSurfaceFormat &format);
		virtual void stop();

		virtual bool present(const QVideoFrame &frame);
		bool present(const AVFrame &frame);

	private:
		QImage::Format m_imageFormat;
		QSize m_imageSize;

		QVideoFrame m_currentFrame;
		bool m_framePainted;
};

#endif // VIDEO_ITEM_H
