#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "FaceRecognition.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

//#define multi_cam_debug	
#define multi_cam_debug_1
//#define motor_test



int tx_test;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	

	this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAttribute  (Qt::WA_DeleteOnClose);



	ui->img_logo = new QLabel(this);

	ui->img_logo->setPixmap(NULL);
	ui->img_captured->setPixmap(NULL);
	
	tx_test = 0;
	
	
	//imgload();
	
	mbFaceDetected = false;
	mbWallpadComm = false;
	
	_evtThread = new FaceEventThread(NULL);
	if(_evtThread) 
	{
		connect(_evtThread, SIGNAL(preview(QImage, bool, int, int)), this, SLOT(onPreview(QImage, bool, int, int)));
		_evtThread->start();
		
	}
	
	_serialThread = new serialComthread(NULL);  //serialComthread
	if (_serialThread) 
		{
		connect(_serialThread, SIGNAL(askDetection()), this, SLOT(onDetection()));
		_serialThread->start();
	}
	
	_melesizThread = new Melesix_thread(NULL);	//serialComthread
		if (_melesizThread) 
			{
			connect(_melesizThread, SIGNAL(FinishTemp(double)), this, SLOT(on_thread_finish()));
			_melesizThread->start();
		}
		


	
	mTimer = new QTimer(this);
	if( mTimer )
 	{
 		connect(mTimer, SIGNAL(timeout()), this, SLOT(onTimer()));
		mTimer->start(300);	
	}
	else
	{
		printf("mainwindow mTimer erorr !!!!!!!!!!\n");	
	}	
	
}

MainWindow::~MainWindow()
{
	if(_evtThread) {
	
		delete _evtThread;
    }

	if(_serialThread) {
	
		delete _serialThread;
    }

	if (mTimer)
		delete mTimer;
	
	delete ui;
	
}

int asciiTohex(char decimal, char* _outdata)
{
    char hexadecimal[10] = {0,};
    int i;
    int position = 0;
    while (1)
    {
        int mod = decimal % 16;
        if (mod < 10)
        {

            hexadecimal[position] = 48 + mod;
        }
        else
        {
            hexadecimal[position] = 65 + (mod - 10);
        }

        decimal = decimal / 16;

        position++;

        if (decimal == 0)
            break;
    }


    for (i = position - 1; i >= 0; i--)
    {
#ifdef multi_cam_debug
        printf("%c", hexadecimal[i]);
#endif
    }
#ifdef multi_cam_debug
    printf("\n");
#endif
	memcpy(_outdata, &hexadecimal, sizeof(hexadecimal));
#ifdef multi_cam_debug
	printf("asciiTohex, _outdata : %s _____ %s\n", hexadecimal, _outdata);
#endif
    return 1;
}

void MainWindow::onPreview(QImage img, bool faceDetected, int servoX, int servoY)
{
	int nSize;
	char packet_data[6]={0,};
	//char packet_data2[6]={0,};
	unsigned char packet_outData[20]={0,};

#ifdef multi_cam_debug		
	printf("in mainwin.. .cpp : faceDetected = [%d] !!_onPreview()\n", faceDetected);
	printf("in mainwin.. .cpp : servoX = [%d], servoY= [%d] !!_onPreview()\n", servoX, servoY);
#endif
	// 얼굴 감지 state 인 경우에만 preview

	if (faceDetected) //|| mbFaceDetected || mbWallpadComm) //faceDetected  1 , mbFaceDetected 0,  mbWallpadComm ?
	{
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!mbFaceDetected = %d\n", faceDetected);
		if (!mbFaceDetected) {
			//printf("mbFaceDetected = false\n");
			ui->img_logo->hide();
			mbFaceDetected = true;
		}
		
#ifdef multi_cam_debug
		printf("in mainwindow servoX = %d, servoY=%d\n", servoX, servoY);
#endif		
	nSize = servoPacketGen(servoX, servoY, (char* )&packet_data);	
#ifdef multi_cam_debug
	printf(" servoPacketGen(): %d, %d, %s , return val: %d !!!!!\n",servoX, servoY, packet_data, nSize);
#endif
	//servoPacketConvert(packet_data1, (char* )&packet_data2);
	int nSize1 = servoPacketConvert(packet_data, (unsigned char* )&packet_outData);
#ifdef multi_cam_debug
	printf(" servoPacketConvert(): %s , return val: %d !!!!!\n", packet_outData, nSize);
#endif
		
// AA 09 02 52 01 2B 31 30 2B 31 30 51 55	
#ifdef multi_cam_debug
	printf("servoPacketConvert in mainwin : %s -------- %s !!!!!\n",packet_outData, packet_outData);
#endif
printf(" before if yjkim !!!!!!!!!!!!!!!!!!!! tx_test : %d !!!!!\n", tx_test);

////////////////////////////////////////////////
	if (tx_test == 0)
	{	//motor contro pack tx send
		bool ret1 = _serialThread->sendDetection(faceDetected, packet_outData);
		printf(" yjkim !!!!!!!!!!!!!!!!!!!! tx_test : %d !!!!!\n", tx_test);
		tx_test = 1;
	}
////////////////////////////////////////////////	
	}		
	ui->img_captured->setPixmap(QPixmap::fromImage(img).scaled(320, 240));	
}


void MainWindow::onTimer()
{
    //onMotorStatusPacketCheck();
    //onDistanceStatusPacketCheck();
	printf("mainwindow onTimer() !!!!!!!!!!\n");	

}

void MainWindow::onMotorStatusPacketCheck()
{
//	bool ret2 = _serialThread->sendStatusMotor(tx_test);
//		printf("mainwindow no send onMotorStatusPacketCheck() !!!!!!!!!!\n");	
}

void MainWindow::onDistanceStatusPacketCheck()
{
//	bool ret3 = _serialThread->sendStatusDistance(tx_test);
//	printf("mainwindow send sendStatusDistance() !!!!!!!!!!\n");	
}


//int MainWindow::servoPacketConvert(char* _indata, char* _outdata)
int MainWindow::servoPacketConvert(char* _indata, unsigned char* _outdata)
{	
	char buf[10], buf2[10] = {0,};
	char final_buf[20] = {0,};
	unsigned char convertData[20]={0,};
	char h1=0, h2=0, h3=0, h4=0, h5=0, h6=0;
	int i = 0;
	//+03-01 012345 index

	memcpy(&buf, _indata, sizeof(buf));
#ifdef multi_cam_debug
	printf("servoPacketConvert : %s -------- %s !!!!!\n",buf, buf);	
#endif	
	sscanf(buf+5, "%c", &h1);
    sscanf(buf+4, "%c", &h2);
    sscanf(buf+3, "%c", &h3);
	sscanf(buf+2, "%c", &h4);
	sscanf(buf+1, "%c", &h5);
	sscanf(buf+0, "%c", &h6);
#ifdef multi_cam_debug
	printf("buf element 6 itmes ...h: %c %c %c %c %c %c !!!!!!\n",h6, h5, h4, h3, h2, h1);
#endif
	/////h6 : +, 0x2b
	int nSize0 = asciiTohex(h6, (char* )&buf2);
#ifdef multi_cam_debug
	printf("asciiTohex(h6, (char* )&buf2) : [%s]  , retrun val : [%d] !!!!!!\n",buf2, nSize0);
#endif
	final_buf[0]=buf2[1];
	final_buf[1]=buf2[0];
#ifdef multi_cam_debug
	printf("final_buf[0] : [%c]  , final_buf[1] : [%c] !!!!!!\n",final_buf[0], final_buf[1]);
#endif
	memset(&buf2, 0x00, sizeof(buf2));

	/////h5 : 1, 0x31
	int nSize1 = asciiTohex(h5, (char* )&buf2);
#ifdef multi_cam_debug
	printf("asciiTohex(h5, (char* )&buf2) : [%s]  , retrun val : [%d] !!!!!!\n",buf2, nSize1);
#endif
	final_buf[2]=buf2[1];
	final_buf[3]=buf2[0];
#ifdef multi_cam_debug
	printf("final_buf[2] : [%c]  , final_buf[3] : [%c] !!!!!!\n",final_buf[2], final_buf[3]);
#endif
	memset(&buf2, 0x00, sizeof(buf2));

	/////h4 : 0, 0x30
	int nSize2 = asciiTohex(h4, (char* )&buf2);
#ifdef multi_cam_debug
	printf("asciiTohex(h4, (char* )&buf2) : [%s]  , retrun val : [%d] !!!!!!\n",buf2, nSize2);
#endif
	final_buf[4]=buf2[1];
	final_buf[5]=buf2[0];
#ifdef multi_cam_debug
	printf("final_buf[4] : [%c]  , final_buf[5] : [%c] !!!!!!\n",final_buf[4], final_buf[5]);
#endif
	memset(&buf2, 0x00, sizeof(buf2));

	/////h3 : -,  0x2b
	int nSize3 = asciiTohex(h3, (char* )&buf2);
#ifdef multi_cam_debug
	printf("asciiTohex(h3, (char* )&buf2) : [%s]  , retrun val : [%d] !!!!!!\n",buf2, nSize3);
#endif
	final_buf[6]=buf2[1];
	final_buf[7]=buf2[0];
#ifdef multi_cam_debug
	printf("final_buf[6] : [%c]  , final_buf[7] : [%c] !!!!!!\n",final_buf[6], final_buf[7]);
#endif
	memset(&buf2, 0x00, sizeof(buf2));
	
	/////h2 : 1, 0x31
	int nSize4 = asciiTohex(h2, (char* )&buf2);
#ifdef multi_cam_debug
	printf("asciiTohex(h2, (char* )&buf2) : [%s]  , retrun val : [%d] !!!!!!\n",buf2, nSize4);
#endif
	final_buf[8]=buf2[1];
	final_buf[9]=buf2[0];
#ifdef multi_cam_debug
	printf("final_buf[8] : [%c]  , final_buf[9] : [%c] !!!!!!\n",final_buf[8], final_buf[9]);
#endif
	memset(&buf2, 0x00, sizeof(buf2));
	
	/////h1 : 0, 0x30
	int nSize5 = asciiTohex(h1, (char* )&buf2);
#ifdef multi_cam_debug
	printf("asciiTohex(h1, (char* )&buf2) : [%s]  , retrun val : [%d] !!!!!!\n",buf2, nSize5);
#endif
	final_buf[10]=buf2[1];
	final_buf[11]=buf2[0];
#ifdef multi_cam_debug
	printf("final_buf[10] : [%c]  , final_buf[11] : [%c] !!!!!!\n",final_buf[10], final_buf[11]);
#endif
	memset(&buf2, 0x00, sizeof(buf2));
#ifdef multi_cam_debug
	printf("final_buf debug : %c %c %c %c %c %c %c %c %c %c %c %c !!!!!!!!!!\n\n", final_buf[0], final_buf[1], final_buf[2], final_buf[3], final_buf[4], final_buf[5],
		final_buf[6], final_buf[7], final_buf[8], final_buf[9], final_buf[10], final_buf[11]);	
#endif

	string str_array = final_buf;
#ifdef multi_cam_debug
	printf("string str_array : %s\n", str_array.c_str());
#endif
	strcpy((char*)convertData, str_array.c_str());  
#ifdef multi_cam_debug
	printf("strcpy((char*)convertData, str_array.c_str())  : %s\n", convertData);		
#endif
	memcpy(_outdata, &convertData, sizeof(convertData));
    return 1;
}

int MainWindow::servoPacketGen(int servoX, int servoY, char* _outdata)
{
	int	minusX = 0;
	char packetX[20];
	//char *dest = NULL;

	char packetY[10];
	int	minusY = 0;
	
	int m_servoX =0;
	int m_servoY =0;

	servoX = (-servoX);
	m_servoX =  servoX;
	m_servoY =  servoY;

/*
pServoX = servoX;
pServoY = servoY;                
servoX = servoX - pServoX;   //10   -5   5
servoY = servoY - pServoY;   //0     0   0
*/
#ifdef multi_cam_debug
	printf("in mainwindow servoX = %d, servoY=%d\n", servoX, servoY);
#endif
	
#ifdef multi_cam_debug_1			
	//printf("in mainwin.. .cpp : m_servoX=[%d], m_servoY=[%d]!!_onPreview_servoPacketGen()\n", m_servoX, m_servoY);
	printf("in mainwin.. .cpp : m_servoX=[%d], m_servoY=[%d]!!_onPreview_codi\n", m_servoX, m_servoY);
#endif	
	if( 0 == m_servoX )
	{
		sprintf(packetX,"-0%d", m_servoX);	
#ifdef multi_cam_debug
		printf("packetX : %s\n", packetX);
#endif //micom whenn 0, recived 2d 
	}
	else if( (0 < m_servoX) && (m_servoX < 10) )
	{
		sprintf(packetX,"+0%d", m_servoX);	
#ifdef multi_cam_debug
		printf("packetX : %s\n", packetX);
#endif
	}
	else if( 10 <= m_servoX )
	{
		sprintf(packetX,"+%d", m_servoX);
#ifdef multi_cam_debug	
		printf("packetX : %s\n", packetX);
#endif
	}
	else if( (-10 < m_servoX) && (m_servoX < 0) )
	{
		minusX = (-m_servoX);
		sprintf(packetX,"-0%d", minusX);
#ifdef multi_cam_debug		
		printf("packetX : %s\n", packetX);
#endif
	}
	else if( -10 >= m_servoX )
	{
		minusX = (-m_servoX);
		sprintf(packetX,"-%d", minusX);
#ifdef multi_cam_debug			
		printf("packetX : %s\n", packetX);
#endif
	}
	else
	{
		printf("packetX error!!!!!!!!!!!\n");
	}

	if( 0 == m_servoY )
	{
		sprintf(packetY,"-0%d", m_servoY);
#ifdef multi_cam_debug			
		printf("packetY : %s\n", packetY);
#endif
	}//micom when 0, recived 2d 
	else if( (0 < m_servoY) && (m_servoY < 10) )
	{
		sprintf(packetY,"+0%d", m_servoY);
#ifdef multi_cam_debug			
		printf("packetY : %s\n", packetY);
#endif
	}
	else if( 10 <= m_servoY )
	{
		sprintf(packetY,"+%d", m_servoY);
#ifdef multi_cam_debug		
		printf("packetY : %s\n", packetY);
#endif
	}
	else if( (-10 < m_servoY) &&  (m_servoY < 0) )
	{
		minusY = (-m_servoY);
		sprintf(packetY,"-0%d", minusY);
#ifdef multi_cam_debug		
		printf("packetY : %s\n", packetY);
#endif
	}
	else if( -10 >= m_servoY )
	{
		minusY = (-m_servoY);
		sprintf(packetY,"-%d", minusY);
#ifdef multi_cam_debug		
		printf("packetY : %s\n", packetY);
#endif
	}
	else
	{
		printf("packetY error!!!!!!!!!!!\n");
	}
	strncat(packetX, packetY, 3);	
	memcpy(_outdata, &packetX, sizeof(packetX));
#ifdef multi_cam_debug_1		
	printf("servoFullPacket, _outdata : %s !!!!!\n", _outdata);
#endif
	return 1;
}

void  MainWindow::on_thread_finish(const double value)
{
}

