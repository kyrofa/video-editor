#include <QDebug>

#include "av_gl_widget.h"

AVGLWidget::AVGLWidget(QWidget *parent) :
	QGLWidget(parent),
	m_width(0),
	m_height(0)
{
}

void AVGLWidget::setFrame(const AVFrame &frame)
{
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
