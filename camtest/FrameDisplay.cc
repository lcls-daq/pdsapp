/**display.cc
 * This file contain the display primitives for
 * camreceiver
 * It is using Qt and because of that is written in C++
 */

#include <stdio.h>
#include <errno.h>
#include <QtCore/QVector>
#include <QtGui/QPixmap>
#include <QtGui/QWidget>
#include <QtGui/QPainter>
#include <QtGui/QApplication>
#include "pds/camera/Frame.hh"
#include "TwoDGaussian.hh"
#include "FrameDisplay.hh"

static pthread_mutex_t display_lock;
static pthread_cond_t display_cond;

#define DEFAULT_WIDTH	640
#define DEFAULT_HEIGHT	480
#define NVIDEO_BUFFERS	2

class DisplayImage: public QWidget
{	//Q_OBJECT

	public:
		DisplayImage(QWidget *parent = 0);
		~DisplayImage();
		int display(const unsigned char *image, unsigned long size, unsigned int width, unsigned int height);
                int draw_reset();
                int draw(double xm, double ym, double a, double b, double phi);

	protected:
		void paintEvent(QPaintEvent *event);

	private:
		QImage *images[2];
		int image_display;
		QVector<QRgb> *gray8_color_table;
		unsigned int image_width;
		unsigned int image_height;
                double xmean, ymean;
                double major_width, minor_width, major_tilt;
                pthread_mutex_t lock;
};

DisplayImage::DisplayImage(QWidget *parent): QWidget(parent)
{	int i, ret;

	ret = pthread_mutex_init(&lock, NULL);
	if (ret != 0) {
		fprintf(stderr, "ERROR: failed to initialize mutex: %s.\n", strerror(ret));
	}
	setWindowTitle("Video");
	image_display = 0;
	image_width = 0;
	image_height = 0;
	gray8_color_table = new QVector<QRgb>(256);
	for (i = 0; i < 256; i++) {
// should be 		gray8_color_table[i]=qRgb(i,i,i);
		gray8_color_table->insert(i, qRgb(i,i,i));
	}
	for(i = 0; i < NVIDEO_BUFFERS; i++) {
	        images[i] = new QImage(DEFAULT_WIDTH, DEFAULT_HEIGHT, QImage::Format_Indexed8);
		images[i]->setColorTable(*gray8_color_table);
		images[i]->fill(128);
	}

	major_width = -1;
}

DisplayImage::~DisplayImage()
{	int i;
	for(i = 0; i < NVIDEO_BUFFERS; i++)
		delete images[i];
	delete gray8_color_table;
}

void DisplayImage::paintEvent(QPaintEvent *evt)
{
	QPainter painter(this);
	pthread_mutex_lock(&lock);
	painter.drawImage(QPoint(), *images[image_display]);
	if (major_width > 0) {
	  int a = int(0.5*major_width);
	  int b = int(0.5*minor_width);
	  painter.setRenderHint(QPainter::Antialiasing);
	  painter.setPen(Qt::red);
	  painter.translate(xmean, ymean);
	  painter.rotate(major_tilt);
	  painter.drawEllipse(-a, -b, 2*a, 2*b);
	}
	pthread_mutex_unlock(&lock);
}

int DisplayImage::display(const unsigned char *data, unsigned long size, unsigned int width, unsigned int height)
{	int image_next = (image_display + 1) % NVIDEO_BUFFERS;
	if (size != (unsigned long)width*height) {
		fprintf(stderr, "ERROR: only format support is 8bits gray images for now.\n");
		return -ENOTSUP;
	}
	if((image_width != width) || (image_height != height)) {
		int i;
		pthread_mutex_lock(&lock);
		for(i = 0; i < NVIDEO_BUFFERS; i++) {
			delete images[i];
			images[i] = new QImage(width, height, QImage::Format_Indexed8);
			images[i]->setColorTable(*gray8_color_table);
		}
		image_width = width;
		image_height = height;
		pthread_mutex_unlock(&lock);
	}
	memcpy(images[image_next]->bits(), data, size);
	image_display = image_next;
	return 0;
}

int DisplayImage::draw_reset()
{
  major_width = -1;
  return 0;
}

int DisplayImage::draw(double xm, double ym, double a, double b, double phi)
{
  xmean = xm; ymean = ym;
  major_width = a; minor_width = b; major_tilt = phi;
  return 0;
}

static DisplayImage* video_screen;

static void *display_main(void *arg)
{	int argc = 1;
	char *argv[] = { "app", NULL };
	QApplication app(argc, argv);

	// Create the display
	video_screen = new DisplayImage();
	video_screen->resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
	video_screen->show();
	// Now the program can use the display APIs
	pthread_mutex_lock(&display_lock);
	pthread_cond_signal(&display_cond);
	pthread_mutex_unlock(&display_lock);
	return (void *)app.exec();
}

using namespace Pds;

FrameDisplay::FrameDisplay()
{	pthread_t qt_thread;
	int ret;

	ret = pthread_mutex_init(&display_lock, NULL);
	if (ret != 0) {
		fprintf(stderr, "ERROR: failed to initialize mutex: %s.\n", strerror(ret));
		return;
	}
	ret = pthread_cond_init(&display_cond, NULL);
	if (ret != 0) {
		fprintf(stderr, "ERROR: failed to initialize condition variable: %s.\n", strerror(ret));
		return;
	}
	pthread_mutex_lock(&display_lock);
	ret = pthread_create(&qt_thread, NULL, display_main, NULL);
	if (ret != 0) {
		fprintf(stderr, "ERROR: failed to start the display thread: %s.\n", strerror(ret));
		return;
	}
	// Qt is very sensitive on using QT before the QApplication has been created
	pthread_cond_wait(&display_cond, &display_lock);
	pthread_mutex_unlock(&display_lock);
}

void FrameDisplay::show_frame(const Frame& frame)
{
        video_screen->display(frame.data(), frame.extent, frame.width, frame.height);
	video_screen->draw_reset();
	video_screen->update ();
}

void FrameDisplay::show_frame(const Frame& frame,
			const TwoDGaussian& fex)
{
        video_screen->display(frame.data(), frame.extent, frame.width, frame.height);
        video_screen->draw   (fex._xmean, fex._ymean, 
			      fex._major_axis_width, fex._minor_axis_width, fex._major_axis_tilt );
	video_screen->update ();
}

