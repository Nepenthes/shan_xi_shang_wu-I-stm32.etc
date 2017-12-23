#include <dataTrans_USART.h>

extern ARM_DRIVER_USART Driver_USART2;

extern osThreadId tid_keyMboard_Thread;	//�������尴������ID�����ڴ�����Ϣ����ʹ���ź�
extern osThreadId tid_keyIFR_Thread;	//��������ת����չ�尴������ID�����ڴ�����Ϣ����ʹ���ź�

osThreadId tid_USARTWireless_Thread;

osThreadDef(USARTWireless_Thread,osPriorityNormal,1,1024);

const u8 dataTransFrameHead_size = 1;
const u8 dataTransFrameHead[dataTransFrameHead_size + 1] = {

	0x7f
};

const u8 dataTransFrameTail_size = 2;
const u8 dataTransFrameTail[dataTransFrameTail_size + 1] = {

	0x0d,0x0a
};

void *memmem(void *start, unsigned int s_len, void *find,unsigned int f_len){
	
	char *p, *q;
	unsigned int len;
	p = start, q = find;
	len = 0;
	while((p - (char *)start + f_len) <= s_len){
			while(*p++ == *q++){
					len++;
					if(len == f_len)
							return(p - f_len);
			};
			q = find;
			len = 0;
	};
	return(NULL);
}

void USART2Wirless_Init(void){
	
	/*Initialize the USART driver */
	Driver_USART2.Initialize(myUSART2_callback);
	/*Power up the USART peripheral */
	Driver_USART2.PowerControl(ARM_POWER_FULL);
	/*Configure the USART to 4800 Bits/sec */
	Driver_USART2.Control(ARM_USART_MODE_ASYNCHRONOUS |
									ARM_USART_DATA_BITS_8 |
									ARM_USART_PARITY_NONE |
									ARM_USART_STOP_BITS_1 |
							ARM_USART_FLOW_CONTROL_NONE, 115200);

	/* Enable Receiver and Transmitter lines */
	Driver_USART2.Control (ARM_USART_CONTROL_TX, 1);
	Driver_USART2.Control (ARM_USART_CONTROL_RX, 1);

	Driver_USART2.Send("i'm usart2 for wireless datstransfor\r\n", 38);
}

void myUSART2_callback(uint32_t event){

	;
}

/*****************����֡������װ*****************/
//����֡���棬���ģ���ַ�����ݳ��ȣ��������ݰ����������ݰ���
u16 dataTransFrameLoad_TX(u8 bufs[],u8 cmd,u8 Maddr,u8 dats[],u8 datslen){

	u16 memp;
	
	memp = 0;
	
	memcpy(&bufs[memp],dataTransFrameHead,dataTransFrameHead_size); //֡ͷ���
	memp += dataTransFrameHead_size;	//ָ�����
	bufs[memp ++] = cmd;
	bufs[memp ++] = Maddr;
	bufs[memp ++] = datslen;
	memcpy(&bufs[memp],dats,datslen);
	memp += datslen;
	memcpy(&bufs[memp],dataTransFrameTail,dataTransFrameTail_size);
	memp += dataTransFrameTail_size;
	
	return memp;
}

void USARTWireless_Thread(const void *argument){
	
	osEvent  evt;
	
	bool RX_FLG = false; //��Ч���ݻ�ȡ��־
	
	const u8 frameDatatrans_totlen = 100;	//֡�����޳�
	const u8 dats_BUFtemp_len = frameDatatrans_totlen - 20;	//�������ݰ������޳�
	u8 dataTrans_TXBUF[frameDatatrans_totlen] = {0};  //����֡����
	u8 dataTrans_RXBUF[frameDatatrans_totlen] = {0};	//����֡����
	u8 TXdats_BUFtemp[dats_BUFtemp_len] = {0};	//���ͺ������ݰ�����
	u8 RXdats_BUFtemp[dats_BUFtemp_len] = {0};	//���պ������ݰ�����
	u8 memp;
	char *p;
	
//	osSignalWait(WIRLESS_THREAD_EN,osWaitForever);		//�ȴ��߳�ʹ���ź�
//	osSignalClear(tid_USARTWireless_Thread,WIRLESS_THREAD_EN);	
	
	Moudle_GTA.Extension_ID = MID_SENSOR_FID;   /****�������*****/
	Moudle_GTA.Wirless_ID = 0xAA;   			/****�������*****/
	
	for(;;){
		
		memset(TXdats_BUFtemp, 0, sizeof(u8) * dats_BUFtemp_len);		//���л�������
		memset(RXdats_BUFtemp, 0, sizeof(u8) * dats_BUFtemp_len);
		memset(dataTrans_TXBUF, 0, sizeof(u8) * frameDatatrans_totlen);
		memset(dataTrans_RXBUF, 0, sizeof(u8) * frameDatatrans_totlen);
		memp = 0;
		
		osDelay(20);
		
		memcpy(&RXdats_BUFtemp[memp],dataTransFrameHead,dataTransFrameHead_size);
		memp += dataTransFrameHead_size;
		RXdats_BUFtemp[memp ++] = Moudle_GTA.Wirless_ID;
		RXdats_BUFtemp[memp ++] = datsTransCMD_DOWNLOAD;
		RXdats_BUFtemp[memp ++] = Moudle_GTA.Extension_ID;
		
		osDelay(20);
		
		Driver_USART2.Receive(dataTrans_RXBUF,frameDatatrans_totlen);	
		
		osDelay(50);
		
		p = memmem(dataTrans_RXBUF,frameDatatrans_totlen,RXdats_BUFtemp,memp);	//֡ͷ��ȡ��У��
		
		if(p){
			
			memset(RXdats_BUFtemp, 0, sizeof(u8) * dats_BUFtemp_len);
			memp = dataTransFrameHead_size + 3; //memp ��ֵΪ�����α�
			memcpy(RXdats_BUFtemp, (const char*)&p[4 + dataTransFrameHead_size + p[memp]], dataTransFrameTail_size); //ȡ֡β
			
//			Driver_USART2.Send(RXdats_BUFtemp,2);		/****�������*****/
//			osDelay(20);								/****�������*****/
			
			if(!memcmp((const char*)RXdats_BUFtemp,dataTransFrameTail,dataTransFrameTail_size)){	//֡βУ��
			
				memset(RXdats_BUFtemp, 0, sizeof(u8) * dats_BUFtemp_len);
				memcpy(RXdats_BUFtemp, (const char*)&p[memp + 1], p[memp]);		//�������ݻ�ȡ
				RX_FLG = true;
				
//				Driver_USART2.Send(RXdats_BUFtemp,2);		/****�������*****/
//				osDelay(20);								/****�������*****/
			}
			
			memset(dataTrans_RXBUF, 0, sizeof(u8) * frameDatatrans_totlen);
		}
		
		if(RX_FLG){
			
			RX_FLG = false;
		
			switch(Moudle_GTA.Extension_ID){	//���ݽ���
			
				case MID_SENSOR_FIRE :	break;
				
				case MID_SENSOR_PYRO :	break;
				
				case MID_SENSOR_SMOKE :	break;
				
				case MID_SENSOR_GAS  :	break;
				
				case MID_SENSOR_TEMP :	break;
				
				case MID_SENSOR_LIGHT:	break;
				
				case MID_SENSOR_SIMU :	break;
				
				case MID_SENSOR_FID :	
					
						{
							FID_MEAS *mptr;
							
							mptr = osPoolAlloc(FID_pool); 
							mptr -> CMD = RXdats_BUFtemp[0];  //�����������
							mptr -> DAT = RXdats_BUFtemp[1];  //�������ݼ���
							
							osMessagePut(MsgBox_MTFID, (uint32_t)mptr, osWaitForever);	//ָ��������ģ������
							osDelay(100);
						}break;
				
				case MID_EXEC_IFR	 :	break;
				
				case MID_EXEC_SOURCE :  break;
				
				
				default:break;
			}
			
			memset(RXdats_BUFtemp, 0, sizeof(u8) * dats_BUFtemp_len); //���ݻ�������
		}
		
		switch(Moudle_GTA.Extension_ID){	//���ݷ���
		
			case MID_SENSOR_FIRE :	
				
					{
						fireMS_MEAS *rptr;
						
						evt = osMessageGet(MsgBox_fireMS, 100);
					}break;
			
			case MID_SENSOR_PYRO :	
				
					{
						pyroMS_MEAS *rptr;
						
						evt = osMessageGet(MsgBox_pyroMS, 100);
					}break;
			
			case MID_SENSOR_SMOKE :	
				
					{
						smokeMS_MEAS *rptr;
						
						evt = osMessageGet(MsgBox_smokeMS, 100);
					}break;
			
			case MID_SENSOR_GAS  :	
				
					{
						gasMS_MEAS *rptr;
						
						evt = osMessageGet(MsgBox_gasMS, 100);
					}break;
			
			case MID_SENSOR_TEMP :	
				
					{
						tempMS_MEAS *rptr;
						
						evt = osMessageGet(MsgBox_tempMS, 100);
					}break;
			
			case MID_SENSOR_LIGHT:	
				
					{
						lightMS_MEAS *rptr;
						
						evt = osMessageGet(MsgBox_lightMS, 100);
					}break;
			
			case MID_SENSOR_SIMU :	
				
					{
						simuMS_MEAS *rptr;
						
						evt = osMessageGet(MsgBox_simuMS, 100);
					}break;
					
			case MID_SENSOR_FID :

					{
						FID_MEAS *rptr;
						evt = osMessageGet(MsgBox_FID, 100);
						if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
							
							rptr = evt.value.p;
							
							if(rptr -> CMD == FID_EXERES_SUCCESS){
							
								TXdats_BUFtemp[0] = rptr -> CMD;
								TXdats_BUFtemp[1] = rptr -> DAT;
								memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,2);
								Driver_USART2.Send(dataTrans_TXBUF,memp);
								osDelay(20);
								
							}else{
							
							
								
							}
							osPoolFree(FID_pool, rptr); 	//�ڴ��ͷ�
						}						
					}break;			
			
			case MID_EXEC_IFR	 :	
				
					
					keyIFRActive();
					break;
			
			case MID_EXEC_SOURCE :  break;
			
			
			default:break;
		}	
	}
}

void USART_WirelessInit(void){

	USART2Wirless_Init();
}

void wirelessThread_Active(void){
	
	USART_WirelessInit();
	tid_USARTWireless_Thread = osThreadCreate(osThread(USARTWireless_Thread),NULL);
}
