#include "serialComthread.h"
#include "mainwindow.h"

#include <QDebug>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <dirent.h> 

#include "log_util.h"


#define serial_read_debug_1  


int skip_serial_cmd = 1;

unsigned char compare_buffer[20];
int compare_buffer_checker;

unsigned char temp_buffer[20];
bool control_motroPack_checker;

extern int tx_test;

static int open_serial(int baud)
{

	int     handle;
    struct  termios  oldtio,newtio;
	//yjkim
	
//	const char *filename = "/dev/ttyUSB-CU";
	const char *filename = "/dev/ttySAC4";
	handle = open( filename, O_RDWR | O_NOCTTY );

    if( handle < 0 )
    {
        printf( "Serial Open Fail /dev/ttySAC4 !! \r\n ");
        exit(0);
    }

    printf("device serial device opened. \n\n");
   
    tcgetattr( handle, &oldtio );
	memset( &newtio, 0, sizeof(newtio) );

	switch( baud )
	{	
		case 115200 : 
			newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD ;
			break;

		case 57600 : 
			newtio.c_cflag = B57600 | CS8 | CLOCAL | CREAD ; 
			break;

		case 38400 : 
			newtio.c_cflag = B38400 | CS8 | CLOCAL | CREAD ; 
			break;

		case 19200 : 
			newtio.c_cflag = B19200 | CS8 | CLOCAL | CREAD ;
			break;

		case 9600 : 
			newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD ;
			break;

		case 4800 : 
			newtio.c_cflag = B2400 | CS8 | CLOCAL | CREAD ;
			break;

		case 2400 : 
			newtio.c_cflag = B2400 | CS8 | CLOCAL | CREAD ;
			break;

		default : newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD ; break;
    }

	//newtio.c_cflag = B57600 | CS8 | CLOCAL | CREAD ;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    //set input mode (non-canonical, no echo,.....)
    newtio.c_lflag = 0;
  /* 
    newtio.c_cc[VTIME] = 30;    
    newtio.c_cc[VMIN]  = 0;     
   */
	newtio.c_cc[VTIME] = 0;    
    newtio.c_cc[VMIN]  = 1;     

  	
    tcflush( handle, TCIFLUSH );
    tcsetattr( handle, TCSANOW, &newtio );

	memset(compare_buffer, 0x00, sizeof(compare_buffer)); //yjkim
	compare_buffer_checker = 0;
	memset(temp_buffer, 0x00, sizeof(temp_buffer)); //yjkim
    control_motroPack_checker=true;

	return handle;

}

static void close_serial(int handle)
{
	close( handle );
}

///////////////////////////multu_cam//////////////////////////////////////////////
int serialComthread::rs232_write( int _rs232_fd, unsigned char *_buf, int _buf_size)
{
	char Debug_Buf[1024] = {0x00, };
	int debug_index = 0;
#ifdef serial_read_debug_1	
	sprintf(Debug_Buf, "%s%d's rs232 write[%d] :  ",Debug_Buf, _rs232_fd, _buf_size);
	for(debug_index = 0; debug_index < _buf_size; debug_index++)
	{		
		sprintf(Debug_Buf, "%s%x ", Debug_Buf, _buf[debug_index]); // %dï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½Ï¿ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½Ú¿ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½
	}
	//LOG_DEBUG("%s", Debug_Buf);
	printf("%s\n", Debug_Buf);
#endif
  return  write(_rs232_fd,_buf,_buf_size);
}
bool serialComthread::Rs232_TX_Generate(unsigned char _cmd, unsigned char _subcmd, unsigned char _type, unsigned char *_data, int _nlen)
{
	int nRet = -1;
	int nReadSize;
	//fd_device *object = NULL;
#ifdef multi_cam_debug	
	printf("Rs232_TX_Generate()!!!!!!!!!!!!!!!!!!!!!\n");	
#endif	
	_Rs232_Data_Packet packet_data;	
	memset(&packet_data, 0x00, sizeof(_Rs232_Data_Packet));

	nReadSize = generate_rs232_packet(_cmd, _subcmd, _type, (unsigned char*)&packet_data, _data, _nlen);
	if(nReadSize < 0)
	{
		//LOG_ERROR("rs232 write error = %d", errno);
		printf("rs232 generate_rs232_packet error = %d\n", nReadSize);
		return false;
	}		

		nRet = rs232_write(m_desc, (unsigned char*)&packet_data, nReadSize);				
#ifdef multi_cam_debug_1			
	printf("in serial.. .cpp : skip_serial_cmd = [%d] !! sendDetection()_Rs232_TX_Generate()\n", nRet);
#endif		
	if(nRet > 0)
	{		
		printf("send packet success !!!\n");
		return true;
	}
	else 
    {
		printf("write packet no !!!\n");
		return false;
	}

	return false;
}



int serialComthread::generate_rs232_packet(unsigned char byCmd, unsigned char byCmdtype, unsigned char byType, unsigned char *_outdata , unsigned char *_indata, int nlen)
{
	unsigned char chk_sum = 0;
	int i = 0;

	rs232_packet tx_req_pkt;

	tx_req_pkt.rs232_d.stx = CONST_STX;
	tx_req_pkt.rs232_d.size = (unsigned char)(nlen + PACKET_HEADER_SIZE);	// 6(data)+3(cmd, cmdtyp, type)=9 byte (cmd ~ data ±îÁö °¹¼ö)

	tx_req_pkt.rs232_d.cmd = byCmd; //control, 0x02
	chk_sum ^= tx_req_pkt.rs232_d.cmd;

	tx_req_pkt.rs232_d.subcmd = byCmdtype; //cmd type motor... 0x52
	chk_sum ^= tx_req_pkt.rs232_d.subcmd;

	//channel	
	tx_req_pkt.rs232_d.channel = byType; //1 channel ... 0x01
	chk_sum ^= tx_req_pkt.rs232_d.channel;

	for(i = 0; i < nlen; i++)
	{
		tx_req_pkt.rs232_d.data[i] = _indata[i];
		chk_sum ^= _indata[i];
#ifdef multi_cam_debug    
/*
		for(count = 0; count < sizeof val/sizeof *val; count++){
			printf("%02x", val[count]);
		}
		printf("\n");
*/	
		printf("%02x\n", _indata[i]);	
#endif		
	}
	printf("\n");
	
	tx_req_pkt.rs232_d.data[i++] = chk_sum;
	tx_req_pkt.rs232_d.data[i] = CONST_ETX;

	memcpy(_outdata, &tx_req_pkt, (size_t)(i+6));
	return i+6;
}

#if 1
int serialComthread::rs232_detach_packet(rs232_packet *_packet, unsigned char *_buf, int _buf_size, int _device_type)
{	
	int i, remide_index= 0;
	unsigned char State = RS232_STX;
	unsigned char KeyIn, packet_start_index = 0, packet_size = 0;
	
	for (i = 0; i < _buf_size; i++) 
	{
		KeyIn = *(_buf+i);		
#ifdef serial_read_debug_1
		//printf("rs232_detach_packet key value = 0x%x\n", KeyIn);		
		printf("READ--> STX:%02x, LEN:%02x, CMD:%02x, TYPE:%02x, CHNNEL:%02x, DATA:%02x %02x %02x %02x %02x %02x, CRC:%02x, ETX:%02x\n",
			_buf[0],_buf[1],_buf[2],_buf[3],_buf[4],_buf[5],_buf[6],_buf[7],_buf[8],_buf[9],_buf[10],_buf[11],_buf[12]);
#endif
		switch(State)
		{		

			case RS232_STX:
				if (KeyIn == CONST_STX)
				{
					State = RS232_SIZE;
					packet_start_index = i;
				}
			break;
			
			case RS232_SIZE:
				packet_size = KeyIn + PACKET_NONBODY_SIZE;	// Ã€Ã¼ÃƒÂ¼ packet size 
#ifdef serial_read_debug	
				printf("packet size =%d, buf_zie = %d, packet_start_index = %d\n", packet_size,_buf_size,packet_start_index);
#endif
				if(packet_size <= _buf_size - packet_start_index && CONST_ETX == *(_buf+(packet_size+packet_start_index-1)))
				{
#ifdef serial_read_debug	
					printf("ext index = %d\n" , packet_size+packet_start_index-1);
					printf("Ok = %d\n", *(_buf+(packet_size+packet_start_index-1)));
#endif					
					State = RS232_PACKET_FIND;
					memcpy(_packet, _buf + packet_start_index , packet_size);
					for(i = 0; i < packet_size; i++)
					{
#ifdef serial_read_debug	
						printf("index = %d , packet value = %d(0x%x)\n",i, _packet->packet_buffer[i], _packet->packet_buffer[i]);
#endif
					}

					remide_index = _buf_size - packet_size - packet_start_index;
					if(remide_index > 0)
					{
#ifdef serial_read_debug	
						printf("remind rs232 buf(start index = %d, _buf_size = %d, remind size = %d)\n" ,packet_start_index, _buf_size, remide_index);
#endif
						SetPacketData(&_buf[packet_size+packet_start_index], remide_index, _device_type);
					}		
				}
				else
				{
					State = RS232_STX;
#ifdef serial_read_debug	
					printf("fail\n");
#endif
				}
	
			break;			
			}

		if(State == RS232_PACKET_FIND)
		{
			break;
		}
	}
	return State;
}

int serialComthread::rs232_integrity_check(rs232_packet *_packet)
{
	unsigned char State = RS232_STX;
	unsigned char KeyIn, check_sum = 0;
	int i = 0, datasize = 0, packet_size; 

	datasize = _packet->rs232_d.size;  //ex)motor full : 13, size:9, 
	packet_size = datasize + PACKET_NONBODY_SIZE;  //9+4=13
	
	for (i = 0; i <  packet_size; i++) //13
	{	
		KeyIn = _packet->packet_buffer[i];
#ifdef serial_read_debug
		printf("rs232_integrity_check key value = 0x%x\n", KeyIn);
#endif		
		switch(State)
		{
			case RS232_STX:
				if (KeyIn == CONST_STX)
					State = RS232_SIZE;
			break;
			
			case RS232_SIZE:					
				State = RS232_CMD;
			break;
		
			case	RS232_CMD:					
				check_sum ^= KeyIn;
				datasize--;    //8
#ifdef multi_cam_debug
				printf("rs232_integrity_check datasize value = %d\n", datasize);
#endif
				if(datasize < 2)
					State = RS232_CMD_ERR;	
				else
					State = RS232_TYPE;			
			break;
				
			case	RS232_TYPE: 				
				check_sum ^= KeyIn;					
#ifdef multi_cam_debug
				printf("rs232_integrity_check datasize value = %d\n", datasize);
#endif
				datasize--; 			
#ifdef multi_cam_debug
				printf("rs232_integrity_check datasize value = %d\n", datasize);
#endif
				if(datasize < 1)	
					State = RS232_SUM;
				else
					State = RS232_CHNNEL; 
			break;
				
			case	RS232_CHNNEL: 		
				check_sum ^= KeyIn;
				datasize--; 			
				if (datasize == 0)		
					State = RS232_SUM;
				else	
					State = RS232_DATA;		
			break;

			case	RS232_DATA: 		
				check_sum ^= KeyIn;
				datasize--; 			
				if (datasize == 0)		
					State = RS232_SUM;
				else	
					State = RS232_DATA;		
			break;

			case RS232_SUM:
				if (KeyIn == check_sum) 	
					State = RS232_ETX;
				else					
					State = RS232_SUM_ERR;
			break;

			case RS232_ETX:			
				State = RS232_FINISHED;				
				break;
		}		
	}
	return State;
}

#endif
/////////////////////////////////////multi_cam/////////////////////////////////////////////

serialComthread::serialComthread(QObject *parent) :
    QThread(parent)
{
	m_abort = false;
	m_desc = open_serial(115200);
}

serialComthread::~serialComthread()
{
	
    if (isRunning()) {
        abort();
	}
	
	if(m_desc >=0){
		close_serial(m_desc);
	}
}

void serialComthread::abort()
{
    m_abort = true;
    wait();
}

void serialComthread::run()
{
	int	fd = m_desc;
	int i=0;
	int nRet;
	rs232_packet packet_data;
	Queue_Data *temp_queue;
	unsigned char cmd, subcmd, channel, data[PACKET_MAX_DATA_SIZE], data_size, sum; 	
	unsigned char buf[256];  //recieved serial buf
	char Debug_Buf[1024] = {0x00,}; 	

	printf("serialComthread::run()!!! \n"); 
	
	InitSystemQueue();

	while (1)
	{	
		memset(buf, 0x00, sizeof(buf));
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
		nRet = rs232_read(fd , buf);
		if(nRet > 0)
		{
			SetPacketData(&buf[0], nRet, 0);

			temp_queue = GetPacketData(0);

			do
			{	
				memset(&packet_data, 0x00, sizeof(rs232_packet));

				nRet = rs232_detach_packet(&packet_data, &temp_queue->buf[0], temp_queue->buf_size, 0);
				printf("rs232_detach_packet return value = %d\n" , nRet);
				if(nRet == RS232_PACKET_FIND)
				{			
					nRet = rs232_integrity_check(&packet_data);
					if(nRet == RS232_FINISHED)
					{			
						cmd = packet_data.rs232_d.cmd;
						subcmd = packet_data.rs232_d.subcmd;
						channel = packet_data.rs232_d.channel;  //add, yjkim
						data_size = (unsigned char)(packet_data.rs232_d.size - PACKET_HEADER_SIZE);
						memcpy(data, packet_data.rs232_d.data, data_size);
						
					#ifdef serial_read_debug
						int i;
						sprintf(Debug_Buf, "rs232 integrity success cmd[0x%x], sub_cmd[0x%x], channel[0x%x], size[0x%x], data : ", cmd, subcmd, channel, data_size);
						for(i = 0; i < data_size; i++)					
							sprintf(Debug_Buf, "%s0x%x ", Debug_Buf, data[i]); 
						//LOG_DEBUG("%s", Debug_Buf);
						printf("%s\n", Debug_Buf);
					#endif
					}
					else
					{
					#ifdef serial_read_debug
						int i;
						sprintf(Debug_Buf, "rs232 parsing fail[%d] packet data : ", nRet);
						for(i = 0; i < (packet_data.rs232_d.size - PACKET_HEADER_SIZE); i++)
						sprintf(Debug_Buf, "%s%x ", Debug_Buf, packet_data.packet_buffer[i]);
					#endif
					}

					memset(temp_queue, 0x00, sizeof(Queue_Data));
				}
				else
				{
#ifdef serial_read_debug
					int i;
					sprintf(Debug_Buf, "rs232 detach fail[%d] packet data : ", nRet);
					for(i = 0; i < temp_queue->buf_size; i++)
					sprintf(Debug_Buf, "%s%x ", Debug_Buf, temp_queue->buf[i]);
					//LOG_DEBUG(Debug_Buf);
					printf("%s\n", Debug_Buf);
#endif
					SetPacketData(&temp_queue->buf[0], temp_queue->buf_size, 0);
#ifdef serial_read_debug
					printf("rs232_detach_packet faile value = %d\n", nRet);
#endif
					break;
				}				
			}while(temp_queue = GetPacketData(0), temp_queue->buf_size != 0);
		}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		#ifdef serial_read_debug_1
			printf("yjkim!!!! READ--> STX:%02x LEN:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x CRC:%02x ETX:%02x\n",
			buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10], buf[11], buf[12]);
		#endif

		#ifdef serial_read_debug_1
			printf("yjkim!!!! READ--> cmd: %02x, subcmd: %02x, channel: %02x, data: [%02x %02x %02x %02x %02x %02x] !!!!!\n", 
			           cmd,subcmd,channel, data[0],data[1],data[2],data[3],data[4],data[5]);
		#endif
			if((cmd==0x72)&&(subcmd == 0x52)){
				//control_motroPack_checker = true;
				printf("yjkim!!!! READ--> control_motroPack_checker : [%d] !!!!!\n", control_motroPack_checker);

				
			}
			if((cmd == 0x71) && (subcmd == 0x52)){

				bool ret_val = recieved_status_check(compare_buffer,temp_buffer,data);
				if (ret_val){
					tx_test = 0;
					printf(" recieved_status_check() SUCCESS, tx_test : [%d] !!!!!\n", tx_test);
				}
				else
					{
						tx_test = 1;
						printf(" recieved_status_check() FAIL, tx_test : [%d] !!!!!\n", tx_test);

					}				}		

	}
		
}

void serialComthread::rs232_data_paring()
{
}


bool serialComthread::recieved_status_check(unsigned char *compare_buffer, unsigned char *temp_buffer, unsigned char *_indata)
{
	bool ret = false;
 	char compareBuf[6]={0,};
	char tempBuf[6]={0,};
	char rev_buf[6]={0,};

	char compareBuf_x[6]={0,};
	char compareBuf_y[6]={0,};
	char tempBuf_x[6]={0,};
	char tempBuf_y[6]={0,};
	char rev_buf_x[6]={0,};
	char rev_buf_y[6]={0,};


	sprintf(compareBuf,"%s", compare_buffer);
	sprintf(tempBuf,"%s", temp_buffer);
	sprintf(rev_buf,"%s", _indata);


	printf("in recieved_status_check ... compareBuf : %s\n",compareBuf);
	printf("in recieved_status_check ... tempBuf : %s\n",tempBuf);
	printf("in recieved_status_check ... rev_buf : %s\n",rev_buf);

	
	sprintf(compareBuf_x,"%c%c%c", (const char*)compareBuf[0],(const char*)compareBuf[1],(const char*)compareBuf[2]);
	sprintf(compareBuf_y,"%c%c%c", (const char*)compareBuf[3],(const char*)compareBuf[4],(const char*)compareBuf[5]);
	sprintf(tempBuf_x,"%c%c%c", (const char*)tempBuf[0],(const char*)tempBuf[1],(const char*)tempBuf[2]);
	sprintf(tempBuf_y,"%c%c%c", (const char*)tempBuf[3],(const char*)tempBuf[4],(const char*)tempBuf[5]);
	sprintf(rev_buf_x,"%c%c%c", (const char*)rev_buf[0],(const char*)rev_buf[1],(const char*)rev_buf[2]);
	sprintf(rev_buf_y,"%c%c%c", (const char*)rev_buf[3],(const char*)rev_buf[4],(const char*)rev_buf[5]);

	printf("in recieved_status_check ... compareBuf_x : %s !!!!!!\n",compareBuf_x);
	printf("in recieved_status_check ... compareBuf_y : %s !!!!!!\n",compareBuf_y);
	printf("in recieved_status_check ... tempBuf_x : %s !!!!!!\n",tempBuf_x);
	printf("in recieved_status_check ... tempBuf_y : %s !!!!!!\n",tempBuf_y);
	printf("in recieved_status_check ... rev_buf_x : %s !!!!!!\n",rev_buf_x);
	printf("in recieved_status_check ... rev_buf_y : %s !!!!!!\n",rev_buf_y);


	int num1 = atoi(compareBuf_x);
	int num2 = atoi(compareBuf_y);
	int num3 = atoi(tempBuf_x);
	int num4 = atoi(tempBuf_y);
	int num5 = atoi(rev_buf_x);
	int num6 = atoi(rev_buf_y);


	printf("in recieved_status_check ... atoi() compareBuf_x : [%d] !!\n", num1);
	printf("in recieved_status_check ... atoi() compareBuf_y : [%d] !!\n", num2);
	printf("in recieved_status_check ... atoi() tempBuf_x : [%d] !!\n", num3);
	printf("in recieved_status_check ... atoi() tempBuf_y : [%d] !!\n", num4);
	printf("in recieved_status_check ... atoi() rev_buf_x : [%d] !!", num5);
	printf("in recieved_status_check ... atoi() rev_buf_x : [%d] !!", num6);

	int result_x = (num1+num3);
	int result_y = (num2+num4);

	printf("in recieved_status_check ... result_x, result_y : [%d], [%d]  !!!!!!\n", result_x, result_y);

	if((result_x==num5) && (result_y==num6)) {
		printf("in recieved_status_check final result success : [%d], [%d], [%d], [%d]  !!!!!!!!!!!!\n", result_x,num5,result_y,num6);
		ret = true;

	}
	else {		
		printf("in recieved_status_check final result fail : [%d], [%d], [%d], [%d]	!!!!!!!!!!!!\n", result_x,num5,result_y,num6);
		ret = false;
	}

	return ret;

}

int serialComthread::rs232_read(int _fd, unsigned char *_buf)
{
	int  sz_read = 0;	
#ifdef serial_read_debug		
	int debug_index = 0;
	char Debug_Buf[1024] = {0x00,};
#endif

	while(sz_read = read(_fd, _buf, 1024), sz_read == -1 && errno == EINTR);

#ifdef serial_read_debug  	
	sprintf(Debug_Buf, "%d's rs232 read[cnt = %d] :  ", _fd, sz_read);
	for(debug_index = 0; debug_index < sz_read; debug_index++)
	{		
		sprintf(Debug_Buf, "%s%x ", Debug_Buf, _buf[debug_index]); // %dï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½Ï¿ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½Ú¿ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½
	}
	printf("int serialComthread::rs232_read() : [%d] !!!\n", sz_read);
	printf("%s\n", Debug_Buf);
#endif
	return sz_read;
}

void serialComthread::InitSystemQueue(void)
{
	int i;	
	
	for(i = 0; i < MAX_QUEUE_CNT; i++)
	{	
		queue_objs[i] = (Queue*)malloc(sizeof(Queue));
		InitQueue(queue_objs[i]);
	}
}
Queue_Data* serialComthread::GetPacketData(unsigned char _device_type)
{
	int queue_size = 0;
	unsigned char temp_data[QUEUE_SIZE] = {0x00,}, temp_buf;
	static Queue_Data temp_packet;
	Queue *temp_queue = queue_objs[_device_type];	

	char Debug_Buf[2048] = {0x00,}; 
	sprintf(Debug_Buf, "%d's Load Queue : ", _device_type);
	
	while(!IsEmpty(temp_queue))				
	{
		temp_buf = Dequeue(temp_queue);
		temp_data[queue_size++] = temp_buf; 
		sprintf(Debug_Buf, "%s%x ", Debug_Buf, temp_buf);
	}

	if(queue_size != 0)
	{
		printf("data size = %d\n" , queue_size);
		temp_packet.buf_size = queue_size;
		memcpy(&temp_packet.buf[0], &temp_data[0], queue_size);
	}
	else
	{	
		memset(&temp_packet, 0x00, sizeof(Queue_Data));
	}
	
	printf("%s\n", Debug_Buf);
	return &temp_packet;
}

void serialComthread::SetPacketData(unsigned char *_data_buf, int _data_size, unsigned char _device_type)
{
	int i;
	unsigned char temp_data;

	char Debug_Buf[2048] = {0x00, };	
	sprintf(Debug_Buf, "%d's Save Queue[%d] : ", _device_type , _data_size);

	for(i = 0; i < _data_size; i++)
	{
		temp_data = _data_buf[i];
		sprintf(Debug_Buf, "%s%x ", Debug_Buf, temp_data); 
		Enqueue(queue_objs[_device_type], temp_data);
	}
	printf("%s\n", Debug_Buf);
}

void serialComthread::InitQueue(Queue *queue)
{
	queue->front = queue->rear = 0;			
	memset(&queue->data, 0x00, sizeof(Queue_Data));  
}

int serialComthread::IsFull(Queue *queue)
{	  
	 return NEXT(queue->rear) == queue->front; 
}

int serialComthread::Enqueue(Queue *queue, char data)
{
	if (IsFull(queue)) 
	{
		printf("queue full");	
		return -1;
	}
	queue->data.buf[queue->rear] = data; 
	queue->data.buf_size++;
	queue->rear = NEXT(queue->rear); 	
	return 0;
}

unsigned char serialComthread::Dequeue(Queue *queue)
{
	unsigned char re = 0;
	if (IsEmpty(queue)) 
	{
		printf("queue empty\n");
		return re;
	}
	re = queue->data.buf[queue->front];
	queue->data.buf_size--;
	queue->front = NEXT(queue->front); 
	return re;
}

int serialComthread::IsEmpty(Queue *queue)
{
	 return queue->front == queue->rear;	  
}

bool serialComthread::sendDetection(bool existence, unsigned char* servoFullPacket)
{

	bool ret = false;
	const char* pos;
	unsigned char val[6];
	size_t count;
	pos = (const char*)servoFullPacket;
	    
    for (count = 0; count < sizeof val/sizeof *val; count++) {
        sscanf(pos, "%2hhx", &val[count]);
        pos += 2;
    }
#ifdef multi_cam_debug    
    for(count = 0; count < sizeof val/sizeof *val; count++){
        printf("%02x", val[count]);
    }
		printf("\n");
		printf("recieve data --> %02x %02x %02x %02x %02x %02x\n", val[0],val[1],val[2],val[3],val[4],val[5]);	
#endif

#ifdef serial_read_debug_1  
	

		//1. control pack copy
		memcpy(&compare_buffer, &val, sizeof(val));
		printf("sendDetection!! compare_buffer[5] recieve data --> %02x %02x %02x %02x %02x %02x [%02x] !!_____ compare_buffer_ckecker : %d\n", 
		compare_buffer[0],compare_buffer[1],compare_buffer[2],compare_buffer[3],compare_buffer[4],compare_buffer[5], compare_buffer[6], compare_buffer_checker);	

		printf("sendDetection!! temp_buffer[6] recieve data --> %02x %02x %02x %02x %02x %02x [%02x] !!\n", 
		temp_buffer[0],temp_buffer[1],temp_buffer[2],temp_buffer[3],temp_buffer[4],temp_buffer[5], temp_buffer[6]);	
				
		
		
		//temp_buffer[6] = temp_buffer_checker;
		//temp_buffer_checker++;
		//compare_buffer[6] = compare_buffer_checker;		

#endif
	if(existence)
	{
		bool motorCon_retVal=Rs232_TX_Generate(MAIN_DEVICE_ACTION_REQUEST, SUB_RS232_CMD_MOTOR, MAIN_1_CH, val, 6);				
		printf(" before sendDetection!! motorCon_retVal : [%d]  !!\n", motorCon_retVal);	
		printf(" before sendDetection!! motorCon_retVal : [%d]  !!\n", motorCon_retVal);	

		if(motorCon_retVal){			
			ret = true;
			//control_motroPack_checker=false;
			printf("sendDetection!! motor contol packet writing send !!\n");	
		}
		if(control_motroPack_checker)
		{			
			bool motorStatu_retVal=Rs232_TX_Generate(MAIN_DEVICE_STATUS_REQUEST, SUB_RS232_CMD_MOTOR, MAIN_1_CH, NULL, 0);
			if(motorStatu_retVal)
			{
				printf("sendDetection!! motor status packet writing send !!\n");	
				ret = true;
				tx_test = 1;  //motorCon_pack no send!!
				printf("sendDetection!! motor status packe, tx_test =[%d] !!\n", tx_test);	
				memcpy(&temp_buffer, &val, sizeof(val));
				printf("sendDetection!! in motorStatu_retVal, temp_buffer[6] recieve data --> %02x %02x %02x %02x %02x %02x [%02x] !!\n", 
		temp_buffer[0],temp_buffer[1],temp_buffer[2],temp_buffer[3],temp_buffer[4],temp_buffer[5], temp_buffer[6]);	
		
	
			}
		}		
	}
	return ret;
}

bool serialComthread::sendStatusDistance(int tx_status_motor)
{
//vncl : AA 03 01 50 01  50 55
	bool ret = false;
	const char* pos;
	unsigned char val[6];
	size_t count;
	//pos = (const char*)servoFullPacket;
	//bool existence = false;
	
	bool retVal=Rs232_TX_Generate(MAIN_DEVICE_STATUS_REQUEST, SUB_RS232_CMD_DISTANCE, MAIN_1_CH, NULL, 0);
		printf("sendStatusMotor()!!!!!!!!!!!!!!! !!\n");	
	if(retVal)
	{
#ifdef multi_cam_debug    
		printf("motor contol packet writing send !!\n");	
#endif
		ret = true;
	}
	else
	{		
#ifdef multi_cam_debug    
		printf("motor contol No packet !!\n");	
#endif
		ret = false;
	}

	return ret;
}

bool serialComthread::sendStatusMotor(int tx_status_motor)
{

	bool ret = false;
	const char* pos;
	unsigned char val[6];
	size_t count;
	//pos = (const char*)servoFullPacket;
	bool existence = false;
	
	bool retVal=Rs232_TX_Generate(MAIN_DEVICE_STATUS_REQUEST, SUB_RS232_CMD_MOTOR, MAIN_1_CH, NULL, 0);
		printf("sendStatusMotor()!!!!!!!!!!!!!!! !!\n");	
	if(retVal)
	{
#ifdef multi_cam_debug    
		printf("motor contol packet writing send !!\n");	
#endif
		ret = true;
	}
	else
	{		
#ifdef multi_cam_debug    
		printf("motor contol No packet !!\n");	
#endif
		ret = false;
	}

	return ret;
}
