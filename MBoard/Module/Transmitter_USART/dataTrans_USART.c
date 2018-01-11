#include <dataTrans_USART.h>

extern ARM_DRIVER_USART Driver_USART2;

extern osThreadId tid_keyMboard_Thread;	//�������尴������ID�����ڴ�����Ϣ����ʹ���ź�

osThreadId tid_USARTWireless_Thread;

osThreadDef(USARTWireless_Thread,osPriorityNormal,1,1024);

const u8 dataTransFrameHead_size = 1;
const u8 dataTransFrameHead[dataTransFrameHead_size + 1] = {

	0x7f
};

const u8 dataTransFrameTail_size = 2;
const u8 dataTransFrameTail[dataTransFrameTail_size + 1] = {

	0x0D,0x0A
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

bool ATCMD_INPUT(char *CMD,char *REPLY,u8 REPLY_LEN,u8 times,u16 timeDelay){
	
	const u8 dataLen = 60;
	u8 dataRXBUF[dataLen];
	u8 loop;
	
	for(loop = 0;loop < times;loop ++){
	
		Driver_USART2.Send(CMD,strlen(CMD));
		osDelay(20);
		Driver_USART2.Receive(dataRXBUF,dataLen);
		osDelay(timeDelay);
		osDelay(20);
		if(memmem(dataRXBUF,dataLen,REPLY,REPLY_LEN))return true;
	}return false;
}

void USART2Wireless_wifiESP8266Init(void){

	const u8 	InitCMDLen = 9;
	const u16 	timeTab_waitAnsr[InitCMDLen] = {
	
		100,
		100,
		100,
		100,
		500,
		5000,
		3000,
		200,
		200,
	};
	const char *wifiInit_reqCMD[InitCMDLen] = {
		
		"ATE0\r\n",
		"AT\r\n",
		"AT+CWMODE_DEF=1\r\n",
		"AT+CWDHCP_DEF=1,0\r\n",
		"AT+CIPSTA=\"10.2.8.200\",\"10.2.8.254\",\"255.255.255.0\"\r\n",
		"AT+CWJAP_DEF=\"GTA2018_IOT\",\"88888888\"\r\n",
		"AT+CIPSTART=\"TCP\",\"10.2.8.109\",8085\r\n",
		"AT+CIPMODE=1\r\n",
		"AT+CIPSEND\r\n"
	};
	const char *wifiInit_REPLY[InitCMDLen] = {
	
		"OK",
		"OK",
		"OK",
		"OK",
		"OK",
		"WIFI CONNECTED",
		"OK",
		"OK",
		">",
	};
	const u8 REPLY_DATLENTAB[InitCMDLen] = {
	
		2,
		2,
		2,
		2,
		2,
		14,
		2,
		2,
		1,
	};
	u8 loop;
	
	Driver_USART2.Send("+++",3);osDelay(100);
	Driver_USART2.Send("+++\r\n",5);osDelay(100);
	Driver_USART2.Send("+++",3);osDelay(100);
	Driver_USART2.Send("+++\r\n",5);osDelay(100);
	
	for(loop = 0;loop < InitCMDLen;loop ++)
		if(false == ATCMD_INPUT((char *)wifiInit_reqCMD[loop],
								(char *)wifiInit_REPLY[loop],
								(u8)REPLY_DATLENTAB[loop],
								3,
								timeTab_waitAnsr[loop])
								)loop = 0;
}

/*****************����֡������װ*****************/
//����֡���棬���ģ���ַ�����ݳ��ȣ��������ݰ����������ݰ���
u16 dataTransFrameLoad_TX(u8 bufs[],u8 cmd,u8 Maddr,u8 dats[],u8 datslen){

	u16 memp;
	
	memp = 0;
	
	memcpy(&bufs[memp],dataTransFrameHead,dataTransFrameHead_size); //֡ͷ���
	memp += dataTransFrameHead_size;	//ָ�����
	bufs[memp ++] = Moudle_GTA.Wirless_ID;
	bufs[memp ++] = cmd;
	bufs[memp ++] = Maddr;;
	bufs[memp ++] = datslen;
	memcpy(&bufs[memp],dats,datslen);
	memp += datslen;
	memcpy(&bufs[memp],dataTransFrameTail,dataTransFrameTail_size);
	memp += dataTransFrameTail_size;
	
	return memp;
}

void USARTWireless_Thread(const void *argument){
	
	osEvent  evt;
    osStatus status;
	
	bool RX_FLG = false; //��Ч���ݻ�ȡ��־
	
	const u8 frameDatatrans_totlen = 100;	//֡�����޳�
	const u8 dats_BUFtemp_len = frameDatatrans_totlen - 20;	//�������ݰ������޳�
	u8 dataTrans_TXBUF[frameDatatrans_totlen] = {0};  //����֡����
	u8 dataTrans_RXBUF[frameDatatrans_totlen] = {0};	//����֡����
	u8 TXdats_BUFtemp[dats_BUFtemp_len] = {0};	//���ͺ������ݰ�����
	u8 RXdats_BUFtemp[dats_BUFtemp_len] = {0};	//���պ������ݰ�����
	u8 memp;
	char *p = NULL;
	
	osSignalWait(WIRLESS_THREAD_EN,osWaitForever);		//�ȴ��߳�ʹ���ź�
	osSignalClear(tid_USARTWireless_Thread,WIRLESS_THREAD_EN);

	/******************************���ߴ���ģ���ʼ��*************************************/
	switch(Moudle_GTA.Wirless_ID){
	
		case MID_TRANS_Wifi:	USART2Wireless_wifiESP8266Init();break;
		
		case MID_TRANS_Zigbee:	break;
		
		default:break;
	}
	
//	Moudle_GTA.Extension_ID = MID_EGUARD;   	/****�������*****/
//	Moudle_GTA.Wirless_ID = 0xAA;   			/****�������*****/
	
	for(;;){

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
		
/*************�������������������������������������������������������������������������ݽ��գ������������������̡߳�������������������������������������������������������������������������*********************/
		if(RX_FLG){
			
			RX_FLG = false;
		
			switch(Moudle_GTA.Extension_ID){	//���ݽ���
			
				case MID_SENSOR_FIRE :{
				
						fireMS_MEAS *mptr = NULL;
						
						do{mptr = (fireMS_MEAS *)osPoolCAlloc(fireMS_pool);}while(mptr == NULL);
						/*�Զ������ݴ��������������������������*/
						
						//�����У����·�����
						
						osMessagePut(MsgBox_MTfireMS, (uint32_t)mptr, osWaitForever);	//ָ��������ģ������
						osDelay(100);
					}break;
				
				case MID_SENSOR_PYRO :{
				
						pyroMS_MEAS *mptr = NULL;
						
						do{mptr = (pyroMS_MEAS *)osPoolCAlloc(pyroMS_pool);}while(mptr == NULL);
						/*�Զ������ݴ��������������������������*/
						
						//�����У����·�����
						
						osMessagePut(MsgBox_MTpyroMS, (uint32_t)mptr, osWaitForever);	//ָ��������ģ������
						osDelay(100);
					}break;
				
				case MID_SENSOR_SMOKE :{
				
						smokeMS_MEAS *mptr = NULL;
						
						do{mptr = (smokeMS_MEAS *)osPoolCAlloc(smokeMS_pool);}while(mptr == NULL);
						/*�Զ������ݴ��������������������������*/
						
						//�����У����·�����
						
						osMessagePut(MsgBox_MTsmokeMS, (uint32_t)mptr, osWaitForever);	//ָ��������ģ������
						osDelay(100);
					}break;
				
				case MID_SENSOR_GAS  :{
				
						gasMS_MEAS *mptr = NULL;
						
						do{mptr = (gasMS_MEAS *)osPoolCAlloc(gasMS_pool);}while(mptr == NULL);
						/*�Զ������ݴ��������������������������*/
						
						//�����У����·�����
						
						osMessagePut(MsgBox_MTgasMS, (uint32_t)mptr, osWaitForever);	//ָ��������ģ������
						osDelay(100);
					}break;
				
				case MID_SENSOR_TEMP :{
				
						tempMS_MEAS *mptr = NULL;
						
						do{mptr = (tempMS_MEAS *)osPoolCAlloc(tempMS_pool);}while(mptr == NULL);
						/*�Զ������ݴ��������������������������*/
						
						//�����У����·�����
						
						osMessagePut(MsgBox_MTtempMS, (uint32_t)mptr, osWaitForever);	//ָ��������ģ������
						osDelay(100);
					}break;
				
				case MID_SENSOR_LIGHT:{
				
						lightMS_MEAS *mptr = NULL;
						
						do{mptr = (lightMS_MEAS *)osPoolCAlloc(lightMS_pool);}while(mptr == NULL);
						/*�Զ������ݴ��������������������������*/
						
						//�����У����·�����
						
						osMessagePut(MsgBox_MTlightMS, (uint32_t)mptr, osWaitForever);	//ָ��������ģ������
						osDelay(100);
					}break;
				
				case MID_SENSOR_ANALOG:{
				
						analogMS_MEAS *mptr = NULL;
						
						do{mptr = (analogMS_MEAS *)osPoolCAlloc(analogMS_pool);}while(mptr == NULL);
						/*�Զ������ݴ��������������������������*/
						
						//�����У����·�����
						
						osMessagePut(MsgBox_MTanalogMS, (uint32_t)mptr, osWaitForever);	//ָ��������ģ������
						osDelay(100);
					}break;
				
				case MID_EGUARD:{
					
						EGUARD_MEAS *mptr = NULL;
						
						do{mptr = (EGUARD_MEAS *)osPoolCAlloc(EGUD_pool);}while(mptr == NULL);
						/*�Զ������ݴ��������������������������*/
						
						mptr -> CMD = RXdats_BUFtemp[0];  //�����������
						mptr -> DAT = RXdats_BUFtemp[1];  //�������ݼ���
						
						osMessagePut(MsgBox_MTEGUD, (uint32_t)mptr, osWaitForever);	//ָ��������ģ������
						osDelay(100);
					}break;
				
				case MID_EXEC_DEVIFR:{
				
						IFR_MEAS *mptr = NULL;
						
						do{mptr = (IFR_MEAS *)osPoolCAlloc(IFR_pool);}while(mptr == NULL);
						/*�Զ������ݴ��������������������������*/
						
						mptr->Mod_addr = RXdats_BUFtemp[0];
						mptr->VAL_KEY  = RXdats_BUFtemp[1];
						
						osMessagePut(MsgBox_MTIFR, (uint32_t)mptr, osWaitForever);	//ָ��������ģ������
						osDelay(100);
					}break;
				
				case MID_EXEC_DEVPWM:{
					
						pwmCM_MEAS *mptr = NULL;
						
						do{mptr = (pwmCM_MEAS *)osPoolCAlloc(pwmCM_pool);}while(mptr == NULL);
						/*�Զ������ݴ��������������������������*/
						
//						mptr->Mod_addr 	 = RXdats_BUFtemp[0];
						mptr->Switch 	 = RXdats_BUFtemp[1];
						mptr->pwmVAL = RXdats_BUFtemp[2];
						
						osMessagePut(MsgBox_MTpwmCM, (uint32_t)mptr, osWaitForever);	//ָ��������ģ������
						osDelay(100);
					}break;
				
				case MID_EXEC_CURTAIN:{
					
						curtainCM_MEAS *mptr = NULL;
						
						do{mptr = (curtainCM_MEAS *)osPoolCAlloc(curtainCM_pool);}while(mptr == NULL);
						/*�Զ������ݴ��������������������������*/
						
						mptr->valACT = RXdats_BUFtemp[0];
						
						osMessagePut(MsgBox_MTcurtainCM, (uint32_t)mptr, osWaitForever); //ָ��������ģ������
						osDelay(100);
					}break;
				
				case MID_EXEC_SOURCE :{
				
						sourceCM_MEAS *mptr = NULL;
						
						do{mptr = (sourceCM_MEAS *)osPoolCAlloc(sourceCM_pool);}while(mptr == NULL);
						/*�Զ������ݴ��������������������������*/
						
						mptr->Switch = RXdats_BUFtemp[0];
						
						osMessagePut(MsgBox_MTsourceCM, (uint32_t)mptr, osWaitForever);	//ָ��������ģ������
						osDelay(100);
					}break;
				
				case MID_EXEC_SPEAK:{
				
						speakCM_MEAS *mptr = NULL;

						do{mptr = (speakCM_MEAS *)osPoolCAlloc(speakCM_pool);}while(mptr == NULL);
						/*�Զ������ݴ��������������������������*/
						
						mptr->spk_num = RXdats_BUFtemp[0];
						
						osMessagePut(MsgBox_MTspeakCM, (uint32_t)mptr, osWaitForever);	//ָ��������ģ������
						osDelay(100);
					}break;
				
				default:break;
			}
		}
		
/****************�����������������������������������������߳����ݽ��գ�������������������ݴ����������������������������������������*************************/
		memset(TXdats_BUFtemp, 0, sizeof(u8) * dats_BUFtemp_len);
		switch(Moudle_GTA.Extension_ID){	//���ݷ���
		
			case MID_SENSOR_FIRE :{
				
						fireMS_MEAS *rptr = NULL;
						evt = osMessageGet(MsgBox_fireMS, 100);
						
						if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
							
							rptr = evt.value.p;
							/*�Զ��巢�����ݴ��������������������������*/
							
							switch(rptr->VAL){
							
								case true:	TXdats_BUFtemp[0] = 0x02;	break;
									
								case false:	TXdats_BUFtemp[0] = 0x01;	break;
								
								default:	TXdats_BUFtemp[0] = ABNORMAL_DAT;	break;
							}TXdats_BUFtemp[0] = rptr->VAL;
							
							memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,1);

							Driver_USART2.Send(dataTrans_TXBUF,memp);
							osDelay(20);
							do{status = osPoolFree(fireMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}	
					}break;
			
			case MID_SENSOR_PYRO :{
				
						pyroMS_MEAS *rptr = NULL;
						evt = osMessageGet(MsgBox_pyroMS, 100);
						
						if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
							
							rptr = evt.value.p;
							/*�Զ��巢�����ݴ��������������������������*/
							
							switch(rptr->VAL){
							
								case true:	TXdats_BUFtemp[0] = 0x02;	break;
									
								case false:	TXdats_BUFtemp[0] = 0x01;	break;
								
								default:	TXdats_BUFtemp[0] = ABNORMAL_DAT;	break;
							}TXdats_BUFtemp[0] = rptr->VAL;
				
							memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,1);
							
							Driver_USART2.Send(dataTrans_TXBUF,memp);
							osDelay(20);
							do{status = osPoolFree(pyroMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_SENSOR_SMOKE :{
				
						smokeMS_MEAS *rptr = NULL;
						evt = osMessageGet(MsgBox_smokeMS, 100);
						
						if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
							
							rptr = evt.value.p;
							/*�Զ��巢�����ݴ��������������������������*/
							
							switch(rptr->VAL){
							
								case true:	TXdats_BUFtemp[0] = 0x02;	break;
									
								case false:	TXdats_BUFtemp[0] = 0x01;	break;
								
								default:	TXdats_BUFtemp[0] = ABNORMAL_DAT;	break;
							}TXdats_BUFtemp[0] = rptr->VAL;
							
							//ģ��ͨ��ֵ��ʱ���������ϴ�
							
							memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,1);
														
							Driver_USART2.Send(dataTrans_TXBUF,memp);
							osDelay(20);
							do{status = osPoolFree(smokeMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_SENSOR_GAS:{
				
						gasMS_MEAS *rptr = NULL;
						evt = osMessageGet(MsgBox_gasMS, 100);
						
						if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
							
							rptr = evt.value.p;
							/*�Զ��巢�����ݴ��������������������������*/
							
							switch(rptr->VAL){
							
								case true:	TXdats_BUFtemp[0] = 0x02;	break;
									
								case false:	TXdats_BUFtemp[0] = 0x01;	break;
								
								default:	TXdats_BUFtemp[0] = ABNORMAL_DAT;	break;
							}TXdats_BUFtemp[0] = rptr->VAL;
							
							//ģ��ͨ��ֵ��ʱ���������ϴ�
							
							memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,1);
							
							Driver_USART2.Send(dataTrans_TXBUF,memp);
							osDelay(20);
							do{status = osPoolFree(gasMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_SENSOR_TEMP:{
				
						tempMS_MEAS *rptr = NULL;
						evt = osMessageGet(MsgBox_tempMS, 100);
						
						if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
							
							rptr = evt.value.p;
							/*�Զ��巢�����ݴ��������������������������*/
							
							TXdats_BUFtemp[0] = (char)rptr->temp;
							TXdats_BUFtemp[1] = (char)((rptr->temp - (float)TXdats_BUFtemp[0])*100);
							
							TXdats_BUFtemp[2] = (char)rptr->hum;
							TXdats_BUFtemp[3] = (char)((rptr->hum - (float)TXdats_BUFtemp[2])*100);
							
							memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,4);
							
							Driver_USART2.Send(dataTrans_TXBUF,memp);
							osDelay(20);
							do{status = osPoolFree(tempMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_SENSOR_LIGHT:{
				
						lightMS_MEAS *rptr = NULL;
						evt = osMessageGet(MsgBox_lightMS, 100);
						
						if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
							
							rptr = evt.value.p;
							/*�Զ��巢�����ݴ��������������������������*/
							
							TXdats_BUFtemp[0] = (char)(rptr->illumination >> 24 & 0x000000ff);
							TXdats_BUFtemp[1] = (char)(rptr->illumination >> 16 & 0x000000ff);
							TXdats_BUFtemp[2] = (char)(rptr->illumination >> 8  & 0x000000ff);
							TXdats_BUFtemp[3] = (char)(rptr->illumination >> 0  & 0x000000ff);
							
							memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,4);
							
							Driver_USART2.Send(dataTrans_TXBUF,memp);
							osDelay(20);
							do{status = osPoolFree(lightMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_SENSOR_ANALOG:{
				
						analogMS_MEAS *rptr = NULL;
						evt = osMessageGet(MsgBox_analogMS, 100);
						
						if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
							
							rptr = evt.value.p;
							/*�Զ��巢�����ݴ��������������������������*/
							
							TXdats_BUFtemp[0] = (char)(rptr->Ich1 >> 8 & 0x00ff);
							TXdats_BUFtemp[1] = (char)(rptr->Ich1 >> 0 & 0x00ff);
							
							TXdats_BUFtemp[2] = (char)(rptr->Ich2 >> 8 & 0x00ff);
							TXdats_BUFtemp[3] = (char)(rptr->Ich2 >> 0 & 0x00ff);
							
							TXdats_BUFtemp[4] = (char)(rptr->Vch1 >> 8 & 0x00ff);
							TXdats_BUFtemp[5] = (char)(rptr->Vch1 >> 0 & 0x00ff);
							
							TXdats_BUFtemp[6] = (char)(rptr->Vch2 >> 8 & 0x00ff);
							TXdats_BUFtemp[7] = (char)(rptr->Vch2 >> 0 & 0x00ff);
							
							memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,8);

							Driver_USART2.Send(dataTrans_TXBUF,memp);
							osDelay(20);
							do{status = osPoolFree(analogMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
					
			case MID_EGUARD:{
				
						EGUARD_MEAS *rptr = NULL;
						evt = osMessageGet(MsgBox_EGUD_FID, 100);
						
						if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
							
							rptr = evt.value.p;
							/*�Զ��巢�����ݴ��������������������������*/
							
							switch(rptr->CMD){
							
								case FID_EXERES_SUCCESS:
								case FID_EXERES_FAIL:
								case FID_EXERES_TTIT:
									
										TXdats_BUFtemp[0] = rptr->CMD;
										TXdats_BUFtemp[1] = rptr->DAT;	
										memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,2);
										break;
								
								case RFID_EXERES_TTIT:
									
										TXdats_BUFtemp[0] = rptr->CMD;
										memcpy(&TXdats_BUFtemp[1],rptr->rfidDAT,4);
										memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,5);
										break;
								
								case PSD_EXERES_TTIT:
									
										TXdats_BUFtemp[0] = rptr->CMD;
										memcpy(&TXdats_BUFtemp[1],rptr->Psd,8);
										memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,9);
										break;
								
								default: 
									
										TXdats_BUFtemp[0] = ABNORMAL_DAT;
										memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,1);
							}

							Driver_USART2.Send(dataTrans_TXBUF,memp);
							osDelay(20);
							do{status = osPoolFree(EGUD_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}						
					}break;			
			
			case MID_EXEC_DEVIFR:{
			
						IFR_MEAS *rptr = NULL;
						evt = osMessageGet(MsgBox_IFR, 100);
						
						if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
							
							rptr = evt.value.p;
							/*�Զ��巢�����ݴ��������������������������*/
							
							TXdats_BUFtemp[0] = rptr->Mod_addr;
							TXdats_BUFtemp[1] = rptr->VAL_KEY;
							TXdats_BUFtemp[2] = rptr->STATUS;	
							memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,2);

							Driver_USART2.Send(dataTrans_TXBUF,memp);
							osDelay(20);
							do{status = osPoolFree(IFR_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_EXEC_DEVPWM:{
			
						pwmCM_MEAS *rptr = NULL;
						evt = osMessageGet(MsgBox_pwmCM, 100);
						
						if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
							
							rptr = evt.value.p;
							/*�Զ��巢�����ݴ��������������������������*/
							
							TXdats_BUFtemp[0] = rptr->Mod_addr;
							TXdats_BUFtemp[1] = rptr->Switch;
							TXdats_BUFtemp[2] = rptr->pwmVAL;	
							memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,3);

							Driver_USART2.Send(dataTrans_TXBUF,memp);
							osDelay(20);
							do{status = osPoolFree(pwmCM_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_EXEC_CURTAIN:{
				
						curtainCM_MEAS *rptr = NULL;
						evt = osMessageGet(MsgBox_curtainCM, 100);
						
						if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
							
							rptr = evt.value.p;
							/*�Զ��巢�����ݴ��������������������������*/
							
							TXdats_BUFtemp[0] = rptr->valACT;
							memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,1);

							Driver_USART2.Send(dataTrans_TXBUF,memp);
							osDelay(20);
							do{status = osPoolFree(curtainCM_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_EXEC_SOURCE :{
				
						sourceCM_MEAS *rptr = NULL;
						evt = osMessageGet(MsgBox_sourceCM, 100);
						
						if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
							
							rptr = evt.value.p;
							/*�Զ��巢�����ݴ��������������������������*/
							
							TXdats_BUFtemp[0] = rptr->Switch;
							TXdats_BUFtemp[1] = (char)(rptr->anaVal >> 8 & 0x00ff);
							TXdats_BUFtemp[2] = (char)(rptr->anaVal >> 0 & 0x00ff);
							memp = dataTransFrameLoad_TX(dataTrans_TXBUF,datsTransCMD_UPLOAD,Moudle_GTA.Extension_ID,TXdats_BUFtemp,3);

							Driver_USART2.Send(dataTrans_TXBUF,memp);
							osDelay(20);
							do{status = osPoolFree(sourceCM_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_EXEC_SPEAK:{
			
						speakCM_MEAS *rptr = NULL;
						evt = osMessageGet(MsgBox_speakCM, 100);
						
						if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
							
							rptr = evt.value.p;
							/*�Զ��巢�����ݴ��������������������������*/
							
							//�����У����Ϸ�����

							Driver_USART2.Send(dataTrans_TXBUF,memp);
							osDelay(20);
							do{status = osPoolFree(speakCM_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			default:break;
		}
		memset(dataTrans_TXBUF, 0, sizeof(u8) * frameDatatrans_totlen);
	}
}

void USART_WirelessInit(void){

	USART2Wirless_Init();
}

void wirelessThread_Active(void){
	
	USART_WirelessInit();
	tid_USARTWireless_Thread = osThreadCreate(osThread(USARTWireless_Thread),NULL);
}
