#include "finger_ID.h"

extern ARM_DRIVER_USART Driver_USART1;								//�豸�����⴮��һ�豸����
extern osThreadId tid_USARTDebug_Thread;

osThreadId tid_fingerID_Thread;
osThreadDef(fingerID_Thread,osPriorityNormal,1,512);

osPoolDef(FIDpool, 16, FID_MEAS);                    // �ڴ�ض���
osPoolId  FIDpool;									 //ID
osMessageQDef(MsgBox, 16, &FID_MEAS);              // ��Ϣ���ж���
osMessageQId  MsgBox;							   //ID

/*******ָ��ʶ��ģ��������ݰ������ʽ���ݼ�����*******/
const u8 FID_FrameHead_size = 2;
const u8 FID_ADDR_size = 4;
const u8 FID_FrameHead[FID_FrameHead_size + 1] = {

	0xEF,0x01
};
const u8 FID_ADDR[FID_ADDR_size + 1] = {

	0xFF,0xFF,0xFF,0xFF
};

/*******ָ��ʶ��ģ�����ָ�����ݼ�����*******/
const u8 FID_CMD_TYPENUM = 20;
const u8 FID_CMD_SIZEMAX = 20;
const u8 FID_CMD[FID_CMD_TYPENUM][FID_CMD_SIZEMAX] = {

	{0x01},
	{0x02},
	{0x02},
	{0x0e,0x04,0x0c},
};
const u8 FID_CMDSIZE[FID_CMD_TYPENUM] = {

	1,
	1,
	1,
	3,
};


void FID_USARTInitCallback(uint32_t event){

	;
}

void USART1fingerID_Init(void){

	/*Initialize the USART driver */
	Driver_USART1.Initialize(FID_USARTInitCallback);
	/*Power up the USART peripheral */
	Driver_USART1.PowerControl(ARM_POWER_FULL);
	/*Configure the USART to 4800 Bits/sec */
	Driver_USART1.Control(ARM_USART_MODE_ASYNCHRONOUS |
									ARM_USART_DATA_BITS_8 |
									ARM_USART_PARITY_NONE |
									ARM_USART_STOP_BITS_1 |
							ARM_USART_FLOW_CONTROL_NONE, 115200);

	/* Enable Receiver and Transmitter lines */
	Driver_USART1.Control (ARM_USART_CONTROL_TX, 1);
	Driver_USART1.Control (ARM_USART_CONTROL_RX, 1);

	Driver_USART1.Send("i'm usart1 for debug log\r\n", 26);
}

u16 ADD_CHECK(u8 dats[],u8 length){  //��У��

	u8 loop;
	u16	result = 0;
	
	for(loop = 0;loop < length;loop ++){
	
		result += dats[loop];
	}
	return result;
}

/*���ݻ������ָ�����ָ�����*/
u16 frameLoad_TX(u8 bufs[],u8 cmd[],u16 cmdlen){  //����ʽװ�����ݻ�������������ݰ���
	
	u8  temp;
	u16 memp;
	
	memp = 0;

	memcpy(&bufs[memp],FID_FrameHead,FID_FrameHead_size); //֡ͷ���
	memp += FID_FrameHead_size;	//ָ�����
	memcpy(&bufs[memp],FID_ADDR,FID_ADDR_size);	//��ַ���
	memp += FID_ADDR_size;	//ָ�����
	bufs[memp ++] = FID_IDENT_CMD;	//��־��䣬��ָ��
	temp = cmdlen + 2;	//����+2 2ΪУ��
	bufs[memp ++] = (u8)(temp >> 8);	//������䣬��ָ��
	bufs[memp ++] = (u8)temp;
	memcpy(&bufs[memp],cmd,cmdlen);		//ָ������
	memp += cmdlen;	//ָ�����
	temp = ADD_CHECK(&bufs[6],memp - 6); //��У�����
	bufs[memp ++] = (u8)(temp >> 8);	//��У����䣬��ָ��
	bufs[memp ++] = (u8)temp;	
	
	return memp;
}

FID_MEAS *fingerID_CMDTX(u8 CMD_ID,u8 rpt_num){	//ָ���ţ��ط�����

	const u16 FRAME_SIZE = 100; //���ݰ����泤��
	u16 TX_num;		//ʵ�ʷ������ݰ�����
	u8  TX_BUF[FRAME_SIZE],RX_BUF[FRAME_SIZE]; //���ݰ�����
	u8  datsbuf[FRAME_SIZE - 20]; //ָ�������
	char  *p_rec;	//���ܰ�����ָ��
	u16 ADD_RES,RX_num;	
	u8  TX_CNT = 0;
	FID_MEAS *result;
	
	result = osPoolAlloc(FIDpool);
	
	osDelay(100);
	memset(TX_BUF,0,FRAME_SIZE * sizeof(u8));	//�������
	memset(datsbuf,0,(FRAME_SIZE - 20) * sizeof(u8));
	memcpy(datsbuf,FID_CMD[CMD_ID],FID_CMDSIZE[CMD_ID]);	//ָ��װ��
	TX_num = frameLoad_TX(TX_BUF,datsbuf,FID_CMDSIZE[CMD_ID]);	//ָ���װ������ݰ�����
	
	do{
		
		Driver_USART1.Send(TX_BUF,TX_num);	//���ݷ���
		osDelay(20);
		Driver_USART1.Receive(RX_BUF,FRAME_SIZE);		
		osDelay(100);						
		p_rec = strstr((const char*)RX_BUF,(const char*)FID_FrameHead);
		TX_CNT ++;
		if(p_rec){	//֡ͷУ��
		
			if(p_rec[6] == FID_IDENT_ANS){	//��ʶУ��
			
				RX_num |= ((u16)p_rec[7]) << 8;		//ȡ����
				RX_num |= (u16)p_rec[8];
				
				ADD_RES = ADD_CHECK((u8 *)&p_rec[6],RX_num + 1);
				if(((u8)ADD_RES >> 8) == p_rec[RX_num + 7] && (u8)ADD_RES == p_rec[RX_num + 8]){	//��У��
				
					result -> CMD = 0xAA;	//�ɹ�ʶ��
					return result;
				}
			}
		}
		memset(RX_BUF,0,FRAME_SIZE * sizeof(u8));	//���ݰ��������	
	}while(TX_CNT < rpt_num);	
	
	memset(TX_BUF,0,FRAME_SIZE * sizeof(u8));	//���ݰ��������
	
	result -> CMD = 0xBB;	//ʶ��ʧ��
	return result;
}

void fingerID_Thread(const void *argument){
	
	FID_MEAS *rptr;		//��Ϣ���л���
	osEvent  evt;	//�¼�����

	osThreadTerminate(tid_USARTDebug_Thread);   //�ر�debug���������軥��
	USART1fingerID_Init();	//���³�ʼ��
	
	rptr = osPoolAlloc(FIDpool); 	//�ڴ�����
	
	for(;;){
	
		evt = osMessageGet(MsgBox, osWaitForever);
		if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
		 
			rptr = evt.value.p;
			switch(rptr -> CMD){
			
				case FID_MSGCMD_FIDSAVE:

						rptr = fingerID_CMDTX(0,5);	//����0��ָ��ؽ��
						if(rptr -> CMD == 0xAA){	//ȡ���

							osPoolFree(FIDpool, rptr);	//�ͷ��ڴ�
							while(1);
						}
						osPoolFree(FIDpool, rptr);	//�ͷ��ڴ�
						break;	   
					
				case FID_MSGCMD_FIDDELE:		break;
					
				case FID_MSGCMD_FIDIDEN:		break;
			}
		}
	}
}

void fingerID_Active(void){

	FIDpool = osPoolCreate(osPool(FIDpool));	//�����ڴ��
	MsgBox = osMessageCreate(osMessageQ(MsgBox), NULL);	//������Ϣ����
	tid_fingerID_Thread	= osThreadCreate(osThread(fingerID_Thread),NULL);
}

