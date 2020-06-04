#ifndef FACEEVENTTHREAD_H
#define FACEEVENTTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QImage>

//#include "FaceRecognition.h"

//_CUBLUNIX_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
//#include <signal.h> //yjkim


#include <linux/v4l2-mediabus.h>
#include <linux/videodev2.h>
#include <linux/videodev2_nxp_media.h>
#include <linux/nxp_ion.h>

#include <ion.h>
#include <nxp-v4l2.h>

//#include "opencv2/opencv.hpp"
// _OPENCV2_
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
// _OPENCV2_


using namespace cv;
using namespace std;


class FaceEventThread : public QThread
{
    Q_OBJECT

public:
    explicit FaceEventThread(QObject *parent);
    ~FaceEventThread();

	//bool isFace;
	//Mat		m_frame;
	int servoX;  //fw x
	int servoY;  //fw y
	
	void abort();
	 void doDetect();
	
	int do_preview(struct nxp_vid_buffer *bufs, int width, int height, int preview_frames);
	
	//bool do_faceDetect(cv::Mat &src);


signals:	

//void preview(QImage img, bool faceDetected);
	void preview(QImage img, bool faceDetected, int servoX, int servoY);

protected:
/*	
	int alloc_buffers(int ion_fd, int count, struct nxp_vid_buffer *bufs, int width, int height, int format);
	int get_size(int format, int num, int width, int height);
	int init_preview(int width, int height, int format);
	int do_preview(struct nxp_vid_buffer *bufs, int width, int height, int preview_frames);
	int detectAndDisplay();
	int end();
*/
	void run();
	
	

private:
   
    static QImage convert(cv::Mat &frame);
/*
	int get_size(int format, int num, int width, int height);
	
	int alloc_buffers(int ion_fd, int count, struct nxp_vid_buffer *bufs, int width, int height, int format);
	int init_preview(int width, int height, int format);
	int do_preview(struct nxp_vid_buffer *bufs, int width, int height, int preview_frames);
	int detectAndDisplay();
	int end();
*/
	int detectAndDisplay();
	bool do_faceDetect(cv::Mat &src);
			
    bool m_abort;
	bool m_doPreview_abort;
/*		
	int servoX;  //fw x
	int servoY;  //fw y
*/
};

#endif // FACEEVENTTHREAD_H
