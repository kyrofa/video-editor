#ifndef AV_GL_WIDGET_H
#define AV_GL_WIDGET_H

extern "C"
{
// All the libav headers go here
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <QScopedPointer>

#include <QGLWidget>
#include <QOpenGLFunctions>

class AVGLWidget : public QGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

	public:
		explicit AVGLWidget(QWidget *parent = 0);
		void setFrame(const AVFrame &frame);

	protected:
		virtual void initializeGL();
		virtual void resizeGL(int width, int height);
		virtual void paintGL();

	private:
		AVPicture m_picture;
		GLsizei m_width;
		GLsizei m_height;
		GLuint m_texture;
};

#endif // AV_GL_WIDGET_H
