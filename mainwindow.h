#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include <QTimer>
#include <QImage>                    
#include <QLabel>              
#include <QMessageBox> 
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

//#include "facedmeventthread.h"
#include "faceeventthread.h"
#include "serialComthread.h"
#include "melesixthread.h"


using namespace cv;
using namespace std;




/*
class QGraphicsScene;
class QImage;
class QGraphicsPixmapItem;
*/
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

	//int tx_test;

	typedef enum {
	    STATUS_INIT = 0,
	    STATUS_AUTHORIZING,
	    STATUS_AUTHORIZED,
	    STATUS_NO_AUTH,
	} STATUS_TEXT_TYPE;
	

private slots:
		//void onPreview(QImage img, bool faceDetected);
		void onMotorStatusPacketCheck();
		void onDistanceStatusPacketCheck();
		void onPreview(QImage img, bool faceDetected, int servoX, int servoY);
		void on_thread_finish(const double value);
	//	void onTimer();
		//void onPreview(QImage img, bool faceDetected);
/*	
		void imgload();   
		void onDetection();
		void onInitCmd();
		void onFaceRecogResult();
*/
protected slots:
	// void onDetectTimeout();
		void onTimer();
private:
    Ui::MainWindow *ui;

	// capture thread
    FaceEventThread *_evtThread;
	serialComthread *_serialThread;
	Melesix_thread *_melesizThread;

	QTimer *mTimer;
	
	volatile bool mbFaceDetected; // 얼굴 감지 여부 상태
	volatile bool mbWallpadComm; // 월패드 통신 진행중 상태
/*
	void setStatusText(STATUS_TEXT_TYPE type);
	
	void restartTimer();
	*/
	//char servoPacket(int servoX, int servoY);
	int servoPacketGen(int servoX, int servoY, char *_outdata);
	//int servoPacketConvert(char* _indata, char* _outdata);
	int servoPacketConvert(char* _indata, unsigned char* _outdata);
	//void onTimer();
};

#endif // MAINWINDOW_H
