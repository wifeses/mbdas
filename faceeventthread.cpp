#include "faceeventthread.h"
#include <QDebug>
#include <signal.h> 

#include "yuv2rgb.neon.h"

//#define HD_DEBUG
//#define PRINT_DEBUG
//#define PRINT_DEBUG_codi
//#define multi_cam_debug
#define multi_cam_debug_1

#define err(fmt, arg...)			\
	fprintf(stderr, "E/%.3d: " fmt, __LINE__, ## arg)

// --------------------------------------------------------
#ifndef ALIGN
#define ALIGN(x, a)		(((x) + (a) - 1) & ~((a) - 1))
#endif

#define FMT_PREVIEW	V4L2_PIX_FMT_YUV422P
#define MAX_BUFFER_COUNT	4
#define FMT_SENDOR		V4L2_MBUS_FMT_YUYV8_2X8

int ion_fd;
int isFace;
int dCount; //debbuging, yjkim
//struct nxp_vid_buffer bufs[MAX_BUFFER_COUNT];
//cv::Mat out;
cv::Mat frame;

#define __V4L2_S(cmd)		\
	do {					\
		int ret = cmd;		\
		if (ret < 0) {		\
			fprintf(stderr, "%.3d: `%s' = %d\n", __LINE__, #cmd, ret);	\
			/*return ret;*/		\
		}					\
	} while (0)


//////////////////////////////////////////////////////////////////////////////
FaceEventThread::FaceEventThread(QObject *parent) :
    QThread(parent)
{
    m_abort = false;
	m_doPreview_abort = false;
	isFace = 0;   //templary
	dCount = 0; //debbuging, yjkim
#if 0	
	int servoX=0;  //fw x
	int servoY=0;  //fw y
#endif	
}

FaceEventThread::~FaceEventThread()
{
    if (isRunning()) {
        abort();
	}
}

void FaceEventThread::abort()
{
    m_abort = true;
	m_doPreview_abort = true;
    wait();
}

static inline int get_size(int format, int num, int width, int height)
{
	int size;

	width = ALIGN(width, 32);
	height = ALIGN(height, 32);

	switch (format) {
		case V4L2_PIX_FMT_YUYV:
			//printf("V4L2_PIX_FMT_YUYV\n");
			if (num > 0) return 0;
			size = (width * height) * 2;
			//printf("size : (%d)\n", size);			
			break;
		case V4L2_PIX_FMT_YUV420M:
			//printf("V4L2_PIX_FMT_YUV420M\n");
		case V4L2_PIX_FMT_YUV422P:
			//printf("V4L2_PIX_FMT_YUV422P\n");
			if (num == 0){
				size = width * height;
			//printf("size : (%d)\n", size);
			}
			else
			{ 
				size = (width * height) >> 1;
				//printf("size : (%d)\n", size);
			}
		
			break;
		case V4L2_PIX_FMT_YUV444:
			//printf("V4L2_PIX_FMT_YUV444\n");
			size = width * height;
			break;
		default:
			size = width * height * 2;
			break;
	}

	return size;
}

int alloc_buffers(int ion_fd, int count, struct nxp_vid_buffer *bufs,
		int width, int height, int format)
{
	struct nxp_vid_buffer *vb;
	int plane_num;
	int i, j;
	int ret;

	if (format == V4L2_PIX_FMT_YUYV || format == V4L2_PIX_FMT_RGB565)
		plane_num = 1;
	else
		plane_num = 3;

	int size[plane_num];
	for (j = 0; j < plane_num; j++)
	{
		size[j] = get_size(format, j, width, height);
	}
	for (i = 0; i < count; i++) 
	{
		vb = &bufs[i];
		vb->plane_num = plane_num;
		for (j = 0; j < plane_num; j++) {
			ret = ion_alloc_fd(ion_fd, size[j], 0,
					ION_HEAP_NXP_CONTIG_MASK, 0, &vb->fds[j]);
			if (ret < 0) {
				err("failed to ion alloc %d\n", size[j]);
				return ret;
			}
			vb->virt[j] = (char *)mmap(NULL, size[j],
					PROT_READ | PROT_WRITE, MAP_SHARED, vb->fds[j], 0);
			if (!vb->virt[j]) {
				err("failed to mmap\n");
				return -1;
			}
			ret = ion_get_phys(ion_fd, vb->fds[j], &vb->phys[j]);
			if (ret < 0) {
				err("failed to get phys\n");
				return ret;
			}
			vb->sizes[j] = size[j];
		}
	}

	return 0;
}

// --------------------------------------------------------

/*
 * Note:
 *  - S5P6818: clipper0 + YUV422P
 */
static int init_preview(int width, int height, int format)
{
	__V4L2_S(v4l2_set_format(nxp_v4l2_clipper0, width, height, format));
	//__V4L2_S(v4l2_set_crop_with_pad(nxp_v4l2_clipper0, 2, 0, 0, width, height));
	__V4L2_S(v4l2_set_crop(nxp_v4l2_clipper0, 0, 0, width, height)); //03

	// Set to default format (800x600) at first
	if (width != 800)
		__V4L2_S(v4l2_set_format(nxp_v4l2_sensor0, 800, 600, FMT_SENDOR));
	__V4L2_S(v4l2_set_format(nxp_v4l2_sensor0, width, height, FMT_SENDOR));

	//__V4L2_S(v4l2_set_ctrl(nxp_v4l2_sensor0, V4L2_CID_EXPOSURE_AUTO, 0));
	 __V4L2_S(v4l2_set_format(nxp_v4l2_sensor0, width, height, FMT_SENDOR));//05
	__V4L2_S(v4l2_reqbuf(nxp_v4l2_clipper0, MAX_BUFFER_COUNT));
	return 0;
}

 int FaceEventThread::do_preview(struct nxp_vid_buffer *bufs, int width, int height,
		int preview_frames)
{
	struct nxp_vid_buffer *buf;
	int i, skips = 0;  

	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		buf = &bufs[i];
		v4l2_qbuf(nxp_v4l2_clipper0, buf->plane_num, i, buf, -1, NULL);
	}
	
	__V4L2_S(v4l2_streamon(nxp_v4l2_clipper0));

	// fill all buffer for display
	int index = 0;
	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		buf = &bufs[index];
		v4l2_dqbuf(nxp_v4l2_clipper0, buf->plane_num, &index, NULL);
		v4l2_qbuf(nxp_v4l2_clipper0, buf->plane_num, index, buf, -1, NULL);
		index++;
	}

	int cap_index = 0;
	int out_index = 0;
	skips = preview_frames; //preview_frames param init
	
	//for (i = 0; i < preview_frames; i++) {
	while(!m_doPreview_abort)
	{
		buf = &bufs[cap_index];
		v4l2_dqbuf(nxp_v4l2_clipper0, buf->plane_num, &cap_index, NULL);
#ifdef multi_cam_debug
		printf("after v4l2_dqbuf() \n\n");	
#endif
		++out_index %= MAX_BUFFER_COUNT;	
#if 0						
		out = cv::Mat(height, width, CV_8UC3);
		//yuv422_2_rgb8888_neon((uint8_t *)out.ptr<unsigned char>(0), (const uint8_t*)buf->virt[0], (const uint8_t*)buf->virt[1], (const uint8_t*)buf->virt[2], width, height, ALIGN(width, 32), width >> 1, width * 4);
		//printf("buf->sizes[0] ' size : %d\n"  buf->sizes[0]);

		//1.
		//cv::Mat myuv(height + height/2, width, CV_8UC1, buf->virt[0]); // pass buffer pointer, not its address
		cv::Mat myuv(height, width, CV_8UC1, buf->virt[0]); // pass buffer pointer, not its address

		//cv::Mat mrgb(height, width, CV_8UC4, &dest);
		//cv::Mat mrgb(height, width, CV_8UC4, out);

		//cv::cvtColor(myuv, mrgb, CV_YCrCb2RGB);
		cv::cvtColor(myuv, out, CV_YUV2RGB_NV12);  // are you sure you don't want BGRA?

		cv::imshow("cu", out);
		cv::waitKey(1);
#else
		cv::Mat out;

		out = cv::Mat(height, width, CV_8UC4);
		yuv422_2_rgb8888_neon((uint8_t *)out.ptr<unsigned char>(0), (const uint8_t*)buf->virt[0], (const uint8_t*)buf->virt[1], (const uint8_t*)buf->virt[2], width, height, ALIGN(width, 32), width >> 1, width * 4);

		frame = out.clone();		
		if (frame.empty())
		{
			printf("frame.empty() no captured frame!! Break!");				
		}
#endif
		//face detection and send codi
		do_faceDetect(frame);

//////////////////// event  processing///////////////////////////////////////////////
		usleep(100*1000);		
		emit preview(convert(frame), (bool)isFace, servoX, servoY);		
#ifdef multi_cam_debug_1
		//usleep(30*1000); //yjkim		
#endif
///////////////////////////////////////////////////////////////////////////////////////////
		v4l2_qbuf(nxp_v4l2_clipper0, buf->plane_num, cap_index, buf, -1, NULL);
#ifdef multi_cam_debug
		printf("after v4l2_qbuf(nxp_v4l2_clipper0) !!!!\n\n");
#endif
	}
	
	__V4L2_S(v4l2_streamoff(nxp_v4l2_clipper0));

#ifdef multi_cam_debug
	printf("after v4l2_streamoff() !!!!\n\n");
#endif
	return 0;
}

bool FaceEventThread::do_faceDetect(cv::Mat &src)
{
	cv::Mat frame_gray, frame_org; 
	//frame
	//int count_p = 30; //multi_cam
	std::vector<Rect> faces;
	
	//얼굴 중앙값 초기화
	int midFaceY=0;
	int midFaceX=0;
	
	int m_servoX=0;
	int m_servoY=0;

	int weight = 10;

//	int skip_frame=1;

	CascadeClassifier face_classifier;
    face_classifier.load("./haarcascade_frontalface_alt.xml");
	
#ifdef HD_DEBUG	
	CascadeClassifier eye_classifier;
	eye_classifier.load("./haarcascade_eye_tree_eyeglasses.xml");
#endif
	frame_org = frame.clone();

	cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
	equalizeHist(frame_gray, frame_gray);

	//face_classifier.detectMultiScale(frame_gray, faces, 1.1, 2, 0|CASCADE_SCALE_IMAGE, Size(30, 30));
	face_classifier.detectMultiScale(frame_gray, faces, 1.1, 5, CV_HAAR_FIND_BIGGEST_OBJECT|CV_HAAR_SCALE_IMAGE, Size(100, 100));

	//int isFace = faces.size();
	isFace = faces.size();

	if(isFace > 0)
	{
#ifdef multi_cam_debug_1
		printf("in face.. .cpp : face_classifier() detect = [%d], , count = [%d]!!\n", isFace, dCount);
		printf("\n==============================================================!!\n");
		dCount++;
#endif
	}
	else
	{
#ifdef multi_cam_debug_1
		printf("in face.. .cpp : face_classifier() no detect = [%d], count = [%d]!!\n", isFace, dCount);
		printf("\n==============================================================!!\n");
		dCount++;
#endif
	}
			
#ifdef PRINT_DEBUG_codi
	printf("face_classifier.detectMultiScale_face() success!!\n");
#endif
    for (int i = 0; i < isFace; i++)
	{
		//Point center(faces[i].x + faces[i].width / 2, faces[i].y + faces[i].height / 2);
		Point center(faces[i].x + faces[i].width / 2 - 320, faces[i].y + faces[i].height / 2 - 240);
		rectangle(frame, Point(faces[i].x, faces[i].y), Point(faces[i].x + faces[i].width, faces[i].y + faces[i].height), Scalar(255, 0, 0), 4, 8, 0);
		i = i+1;//add one face
		
#ifdef PRINT_DEBUG_codi
/*
		printf("face_Point_center in => !!!!!!\n");
		cout << "Roi Location "<< Point center(faces[i].x + faces[i].width / 2, faces[i].y + faces[i].height / 2) << endl; 		
		cout << "1. Roi x,y "<< Point(faces[i].x, faces[i].y) << endl; 
		cout << "2. converting Roi x,y "<< Point(faces[i].x - 320, faces[i].y -240) << endl; 
		cout << "Roi x,y "<< Point(faces[i].width, faces[i].height) << endl; 
		cout << "3-1. converting Roi center "<< Mat(center) << endl; 			
		cout << "3-2. converting Roi center_re "<< Mat(center_re) << endl; 			
		cout << "Roi x+width, y+height "<< Point(faces[i].x + faces[i].width, faces[i].y + faces[i].height) << endl; 
		printf("face_Point_center out => !!!!\n\n\n");
*/
#endif
    }
	
	if(isFace > 0)
	{
#if 0		
		midFaceX = (faces[0].x + faces[0].width/2 - 320); //0
		midFaceY = (faces[0].y + faces[0].height/2 -240); //0
#else
		midFaceX = (faces[0].x + faces[0].width/2); //320
		midFaceY = (faces[0].y + faces[0].height/2); //240
#endif

#ifdef multi_cam_debug_1
	int	midFaceX_1 = (faces[0].x + faces[0].width/2 - 320); //0
	int	midFaceY_1 = (faces[0].y + faces[0].height/2 -240); //0
		//printf("in face.. .cpp : midFaceX = [%d], midFaceY = [%d] !!\n", midFaceX, midFaceY);
		printf("in face.. .cpp : midFaceX = [%d], midFaceY = [%d] !!\n", midFaceX_1, midFaceY_1);
#endif
		//midFaceX = (~midFaceX + 1);
		//midFaceY = (~midFaceY + 1);
		m_servoX =  (320-midFaceX);
		m_servoY =  (240-midFaceY);
				
#ifdef multi_cam_debug		
		printf("in face.. .cpp : m_servoX = [%d], m_servoY = [%d]!!\n", m_servoX, m_servoY);
#endif

#if 1
		servoX = m_servoX/weight;
		servoY = m_servoY/weight;
#else		
		servoX = m_servoX;
		servoY = m_servoY;
#endif

#if 1
		if((servoX > -8) && (servoX <8))  
	   {
		   servoX = 0;
		   printf("in face.. .cpp : center_Dont move!! codi_servoX = %d, servoY=%d\n", servoX, servoY);
	   }
		if((servoY > -5) && (servoY < 5))  	
	 	{
			servoY = 0;
			printf("in face.. .cpp : center_Dont move!! codi_servoX = %d, servoY=%d\n", servoX, servoY);
	 	}
/*
		else
			printf("default codi!!!\n");
			*/
#endif

		//emit preview(convert(frame), (bool)isFace, servoX, servoY);
#ifdef multi_cam_debug
		printf("in face.. .cpp : servoX = [%d], servoY = [%d] !!\n", servoX, servoY);
#endif
		return true;
	} 
	return false;
}

void sig_handler(int sig)
{
    if(sig==SIGINT)
    {
		__V4L2_S(v4l2_streamoff(nxp_v4l2_clipper0));
		v4l2_exit();
		close(ion_fd);
		 printf("process stop!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
         //if (getchar() == 'y')
         exit(0);
    }
}


int FaceEventThread::detectAndDisplay()
{
	signal(SIGINT, sig_handler);//yjkim
	int width_p = 0, height_p = 0, count_p = 100;
/////////// motor and face detection

	 width_p = 640;
     height_p = 480;

	ion_fd = ion_open();
	
	if (ion_fd < 0) {
		err("failed to ion_open, errno = %d\n", errno);
		return -EINVAL;
	}

	struct V4l2UsageScheme s;
	memset(&s, 0, sizeof(s));
	s.useDecimator0 = false;
	s.useClipper0 = true;
	s.useMlc0Video = true;
	int ret = v4l2_init(&s);

	if (ret < 0) {
		err("initialize V4L2 failed, %d\n", ret);
		close(ion_fd);
		return ret;
	}

	//printf("before alloc_buffers()\n\n");
	struct nxp_vid_buffer bufs[MAX_BUFFER_COUNT];
	ret = alloc_buffers(ion_fd, MAX_BUFFER_COUNT, bufs, width_p, height_p, FMT_PREVIEW);

	if (ret >= 0 && width_p > 0) {
		//printf("before init_preview()\n\n");
		init_preview(width_p, height_p, FMT_PREVIEW);//init_preview
		//printf("before do_preview()\n\n");
		do_preview(bufs, width_p, height_p, count_p);//do_preview
	}

	v4l2_exit();
	close(ion_fd);

	return ret;
}

//////////////////////original ////////////////////////////////////////////////////////////////////////////////
QImage FaceEventThread::convert(cv::Mat &frame)
{
    // 8-bits unsigned, NO. OF CHANNELS=1
    if(frame.type()==CV_8UC1)
    {
        // Set the color table (used to translate colour indexes to qRgb values)
        QVector<QRgb> colorTable;
        for (int i=0; i<256; i++)
            colorTable.push_back(qRgb(i,i,i));
        // Copy input Mat
        const uchar *qImageBuffer = (const uchar*)frame.data;
        // Create QImage with same dimensions as input Mat
        QImage img(qImageBuffer, frame.cols, frame.rows, frame.step, QImage::Format_Indexed8);
        img.setColorTable(colorTable);
        return img;
    }
    // 8-bits unsigned, NO. OF CHANNELS=3
    else if(frame.type()==CV_8UC3)
    {
        // Copy input Mat
        const uchar *qImageBuffer = (const uchar*)frame.data;
        // Create QImage with same dimensions as input Mat
        QImage img(qImageBuffer, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
        return img.rgbSwapped();
    }// 8-bits unsigned, NO. OF CHANNELS=4
	else if(frame.type()==CV_8UC4)
	{
		const uchar *qImageBuffer = (const uchar*)frame.data;
		QImage img(qImageBuffer, frame.cols, frame.rows, frame.step, QImage::Format_ARGB32);
 		return img;
	}
    else
    {
        qDebug() << "ERROR: Mat could not be converted to QImage.";
        return QImage();
    }
}

void FaceEventThread::doDetect()
{	
	m_abort = false;
	//bool isFace = true; 

	while(!m_abort)
	{
#ifdef multi_cam_debug
		printf("before detectAndDisplay()\n\n");	
#endif
		detectAndDisplay();	
		//emit preview(convert(frame), (bool)isFace);
		printf("after preview(convert(out), (bool)isFace)\n\n");			
		//usleep(33*1000);
	}
		
}

void FaceEventThread::run() 
{
 	 m_abort = false;

	//while(!m_abort) 
	//{
		doDetect();
	//}
}
