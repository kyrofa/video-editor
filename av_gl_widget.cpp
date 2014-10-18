#include <QDebug>
#include <QLabel>

#include "av_gl_widget.h"

AVGLWidget::AVGLWidget(QWidget *parent) :
	QGLWidget(parent),
	m_label(new QLabel()),
	m_width(0),
	m_height(0)
{
}

// Assumes AVFrame is in RGB24 format
void AVGLWidget::showImage(const AVFrame &frame)
{
	int width = frame.width;
	int height = frame.height;

	AVFrame *rgbFrame = avcodec_alloc_frame();
	int numBytes= avpicture_get_size(PIX_FMT_RGB24,
												width, height);
	quint8 *buffer = new quint8[numBytes];

	avpicture_fill((AVPicture*)rgbFrame, buffer, PIX_FMT_RGB24,
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

	QImage *image = new QImage(width, height, QImage::Format_RGB32);

	unsigned char *src = static_cast<unsigned char*>(frame.data[0]);
	for (int y = 0; y < height; y++)
	{
		QRgb *scanLine = reinterpret_cast<QRgb*>(image->scanLine(y));
		for (int x = 0; x < width; x++)
		{
			scanLine[x] = qRgb(src[3*x], src[3*x+1], src[3*x+2]);
		}
		src += frame.linesize[0];
	}

	m_label->setPixmap(QPixmap::fromImage(*image));
	m_label->show();

	return;
}

void AVGLWidget::setFrame(const AVFrame &frame)
{
	showImage(frame);
	return;
	if (m_width && m_height)
	{
		avpicture_free(&m_picture);
	}

	m_width = frame.width;
	m_height = frame.height;

	avpicture_alloc(&m_picture, PIX_FMT_RGB24, m_width, m_height);
	SwsContext *conversionContext = sws_getContext(
												  m_width, m_height,
												  static_cast<PixelFormat>(frame.format),
												  m_width, m_height, PIX_FMT_RGB24,
												  SWS_BILINEAR, NULL, NULL, NULL);

	if (!conversionContext)
	{
		qCritical("Unable to allocate conversion context!");
		return;
	}

	sws_scale(conversionContext, frame.data, frame.linesize, 0, m_height,
				 m_picture.data, m_picture.linesize);
	updateGL();
}

void AVGLWidget::initializeGL()
{
	initializeOpenGLFunctions();

	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &m_texture);
}

void AVGLWidget::resizeGL(int width, int height)
{
	int side = qMin(width, height);
	glViewport((width - side) / 2, (height - side) / 2, side, side);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
}

void AVGLWidget::paintGL()
{
	qglClearColor(QColor::fromRgb(0, 0, 0));
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (m_width && m_height)
	{
		qDebug() << "Drawing texture!";
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();

		glShadeModel( GL_FLAT );
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
		glColor3f(0.5, 0.5, 0);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0,
						 GL_RGB, GL_UNSIGNED_BYTE, m_picture.data);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(-0.5f, 0.5f);  // vertex 1
		glTexCoord2f(0.0f, 0.0f); glVertex2f(-0.5f, -0.5f); // vertex 2
		glTexCoord2f(1.0f, 0.0f); glVertex2f(0.5f, -0.5f); // vertex 3
		glTexCoord2f(1.0f, 1.0f); glVertex2f(0.5f, 0.5f); // vertex 4
		glEnd();
		glDisable(GL_TEXTURE_2D);

		glFlush();
	}
}
