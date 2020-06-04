#ifndef SERIALCOMTHREAD_H
#define SERIALCOMTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

////////////////////////////////////////////////////////////////////////////////////
//////////////(cmd, subcmd, channel) header + data(body) //////////////////////////
//stx, size, cmd, subcmd, channel, data, crc, etx  ///////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
#define PACKET_NONBODY_SIZE				4	// sizeof(stx+etx+size+checksum)
#define PACKET_RS232_BODY_SIZE			2	// sizeof(cmd + subcmd )
#define RS232_PACKET_DEFAULT_SIZE  PACKET_NONBODY_SIZE+PACKET_RS232_BODY_SIZE    //6

/////////////////////////////////////////////////////////////////////////////////////

#define PACKET_HEADER_SIZE		2
#define PACKET_DEFAULT_SIZE		7 
#define PACKET_LENGTH_OTHER_SIZE (PACKET_DEFAULT_SIZE - PACKET_HEADER_SIZE)
#define PACKET_MAX_DATA_SIZE		20	//122
#define PACKET_MAX_SIZE (PACKET_DEFAULT_SIZE + PACKET_MAX_DATA_SIZE)   // 27

/////////////////////////////////////////////////////////////////////////////////
#if 1
#define QUEUE_SIZE  2048
#define NEXT(index)	 ((index+1)%QUEUE_SIZE)  //원형 큐에서 인덱스를 변경하는 매크로 함수

//static Queue *queue_objs[4];

//Queue *rx_queue;
//Queue *tx_queue;

typedef enum // Device_Type 값과 일치 
{	
	MAIN_SOCKET_MW_CLIENT_QUEUE = 0x00, 		
	MAIN_SOCKET_GCU_CLIENT_QUEUE, 		

	MAIN_RS232_FRONT_QUEUE, 		
	MAIN_RS232_REAR_QUEUE,		
	
	MAX_QUEUE_CNT
} Queue_Type;

typedef struct Queue_Data_
{
	unsigned char buf[QUEUE_SIZE];
	int buf_size;
}Queue_Data;

typedef struct Queue_ 
{
	Queue_Data data;
	int front; //꺼낼 인덱스(가장 오래전에 보관한 데이터가 있는 인덱스)
	int rear;	//보관할 인덱스
}Queue;

#endif

typedef enum 
{	
	MAIN_1_CH = 0x01, 		
	MAIN_2_CH,	
	MAX_DEVICE_CNT
} Device_Type;


typedef enum Main_Command_Category
{
	MAIN_DEVICE_STATUS_REQUEST = 0x01,
	MAIN_DEVICE_ACTION_REQUEST,
	MAIN_DEVICE_STATUS_RESPONSE = 0x71,
	MAIN_DEVICE_ACTION_RESPONSE,
	MAIN_DEVICE_STATUS_EVENT_INTERRUT,	
	MAIN_DEIVE_NOTIFY_ALARM	// GCU <-> Middleware �� ��� 
}_Main_Command_List;
typedef enum Sub_RS232_Command_Category
{
	SUB_RS232_CMD_DISTANCE = 0x50,
	SUB_RS232_CMD_LEDBRIRHTNESS, 
	SUB_RS232_CMD_MOTOR,		// 0x52 => ��ȸ, ����, �̺�Ʈ 
	SUB_RS232_CMD_LED = 0x31,				// 0x31 => ��ȸ, ����
	SUB_RS232_CMD_SWITCH,			// 0x32 => ��ȸ, �̺�Ʈ 
	SUB_RS232_CMD_SENSOR,			// 0x33 => ��ȸ, �̺�Ʈ 
	SUB_RS232_CMD_OTHER,				// 0x34 => ��ȸ 
	SUB_RS232_CMD_ON_OFF,			// 0x35 => ���� 
	SUB_RS232_CMD_MATRIX,			// 0x36 => ��ȸ, ����
	SUB_RS232_CMD_BACKLIGHT,		// 0x37 => ��ȸ, ���� 
	SUB_RS232_CMD_RELAY,				// 0x38 => 	
}_Sub_RS232_Command_List;



enum CONST {
	CONST_STX = 0xaa, 	
	CONST_ETX = 0x55				
};

enum RS232_STATE 
{
	RS232_STX,					// 0 
	RS232_SIZE, 					// 1
	RS232_CMD, 					// 2
	RS232_TYPE, 				// 3
	RS232_CHNNEL, 				// 4	
	RS232_DATA, 				// 5
	RS232_SUM, 					// 6
	RS232_ETX, 					// 7
	RS232_FINISHED, 			// 8
	RS232_SIZE_ERR,			// 9
	RS232_CMD_ERR,				// 10
	RS232_SUM_ERR,				// 11
	RS232_ERR,					// 12
	RS232_PACKET_NOT_FIND,	// 13
	RS232_PACKET_FIND			// 14
};

typedef struct Rs232_Status_Response_Packet
{
	unsigned char stx;
	unsigned char size;		// cmd ~ data 까지의 길이 
	unsigned char cmd;		// command 
	unsigned char type;		// sub command
	unsigned char channel;	// defalt : 1
	unsigned char data[23];
	unsigned char sum;		// cmd ~ data 까지의 데이터 값 합 
	unsigned char etx;
}status_packet;

typedef struct Rs232_request_packet
{
	unsigned char stx;
	unsigned char size;		// cmd ~ data 까지의 길이 
	unsigned char cmd;		// command 
	unsigned char type;		// sub command
	unsigned char channel;	// defalt : 1
	unsigned char data;		
	unsigned char sum;		// cmd ~ data 까지의  데이터 값 합 
	unsigned char etx;
}control_packet;
/*
typedef union
{
	status_packet p_4th_status;		// status packet	
	control_packet p_4th_control; 	// tx packet and control reponse packet 
	unsigned char packet_buffer[29];// 최대 패킷 크기 
} rs232_packet;
*/

typedef struct Rs232_Data_Packet
{
	unsigned char stx;					
	unsigned char size;		
	unsigned char cmd;		
	unsigned char subcmd;			
	unsigned char channel;			
	unsigned char data[PACKET_MAX_DATA_SIZE]; 	// 20 byte
	unsigned char sum;		
	unsigned char etx;
}_Rs232_Data_Packet;	// total 27 byte

typedef union
{
	_Rs232_Data_Packet rs232_d;  //control packet		 
	status_packet p_status;   //status packet		
	unsigned char packet_buffer[PACKET_MAX_SIZE];// �ִ� ��Ŷ ũ�� // 27
}rs232_packet;


////////////////////////multi_cam///////////////////////////////////
typedef enum {
	WALLPAD_INIT = 0,
	WALLPAD_DETECT_REPLY,
	WALLPAD_DETECT_NO_REPLY,
	WALLPAD_AUT_STARTED,
} WALLPAD_STATE;

static Queue *queue_objs[MAX_QUEUE_CNT];

class serialComthread : public QThread
{
    Q_OBJECT

public:
    explicit serialComthread(QObject *parent);
    ~serialComthread();
		
    void abort();
	//bool send(unsigned char* data, unsigned int len);
	//bool send(char *Packet, unsigned int len);
	//bool send2(char* packetX, char* packetY);

	
	WALLPAD_STATE getState()
	{
		return m_state;
	}
	
	//bool sendDetection(bool existence); //sendDetection  
////////////////////////multi_cam/////////////////////////////////////////////////
	//bool sendDetection(bool existence,     char *servoFullPacket2);
	bool sendDetection(bool existence, unsigned char* servoFullPacket);
	bool sendStatusMotor(int tx_status_motor);
	bool sendStatusDistance(int tx_status_motor);
///////////////////////////////////////////////////////////////////////////////////
	
	bool sendInitCmd();  
	bool sendFaceRecogResult();
	
////////////////////////multi_cam/////////////////////////////////////////////////
	//bool Rs232_TX_Generate(unsigned char _cmd, unsigned char _subcmd, unsigned char _type, unsigned char *_data, int _nlen, bool existence);		
	bool Rs232_TX_Generate(unsigned char _cmd, unsigned char _subcmd, unsigned char _type, unsigned char *_data, int _nlen);
	int generate_rs232_packet(unsigned char byCmd, unsigned char byCmdtype, unsigned char byType, unsigned char *_outdata , unsigned char *_indata, int nlen);
	int rs232_detach_packet(rs232_packet *_packet, unsigned char *_buf, int _buf_size, int _device_type);
	int rs232_integrity_check(rs232_packet *_packet);
	int rs232_write( int _rs232_fd, unsigned char *_buf, int _buf_size);
	int rs232_read(int _fd, unsigned char *_buf);
	
//////////////////////////////////////////////////////////////////////////////////////
	void rs232_data_paring(void);

	//int RecvByte(int fd, unsigned char* recv);

signals:	
	void askDetection();
//	void askInitCmd();
//	void askFaceRecogResult();
	
protected:
    void run();

private:
	bool m_abort;
	WALLPAD_STATE m_state;
	int m_desc;

	void InitSystemQueue(void);
	Queue_Data* GetPacketData(unsigned char _device_type);
	void SetPacketData(unsigned char *_data_buf, int _data_size, unsigned char _device_type);
	void InitQueue(Queue *queue);
	int IsFull(Queue *queue);
	int Enqueue(Queue *queue, char data);
	unsigned char Dequeue(Queue *queue);
	int IsEmpty(Queue *queue);
	bool recieved_status_check(unsigned char *compare_buffer, unsigned char *temp_buffer, unsigned char *_indata);
   
};

#endif // SERIALCOMTHREAD_H
