#include "fingerID.h"

osThreadId tid_fingerID_Thread;
osThreadDef(fingerID_Thread,osPriorityNormal,1,1024);

extern ARM_DRIVER_USART Driver_USART1;								//�豸�����⴮��һ�豸����
extern osThreadId tid_USARTDebug_Thread;

/*******ָ��ʶ��ģ��������ݰ������ʽ���ݼ�����*******/
const u8 FID_FrameHead_size = 2;
const u8 FID_ADDR_size = 4;
const u8 FID_FrameHead[FID_FrameHead_size + 1] = {	//+1����β����\0,����ʹ���ַ����⺯��

	0xEF,0x01
};
const u8 FID_ADDR[FID_ADDR_size + 1] = {	

	0xFF,0xFF,0xFF,0xFF
};

/*******ָ��ʶ��ģ�����ָ�����ݼ�����*******/
const u8 FID_CMD_TYPENUM = 20;		//ָ�������
const u8 FID_CMD_SIZEMAX = 20;		//��ָ�������
const u8 FID_CMD[FID_CMD_TYPENUM][FID_CMD_SIZEMAX] = {		//ָ�����ָ��������ݶ�һ���Ŵ˻���

	{0x01},				//ָ��ͼ���ȡ
	{0x02,0x01},		//ͼ���������������buffer1
	{0x02,0x02},		//ͼ���������������buffer2
	{0x04,0x01,0x00,0x01,0x00,0xff},	//buffer1 ͼ��ʶ�� page1 - page255
	{0x05},				//buffer1��buffer2 �ϲ�ע��ģ��
	{0x06,0x01},		//��̬����ָ�ģ��洢������buffer1������ʶ��Ҳֻʶ��buffer1   ���������ֽ�page �룬��Ϊҳ���������ƣ����ֽڸ߰�λ��̬����ָ���ʱ��0x00��䣬����λ���ṩ ���ֽڵͰ�λ������ 
	{0x0c},				//��̬����ָ�ģ��ɾ�����������ֽڵ�pageID�� �� ���ֽڵ�����
	{0x0e,0x04,0x0c},	//ģ���������
};
const u8 FID_CMDSIZE[FID_CMD_TYPENUM] = {	//ָ�����ָ���Ӧ����

	1,
	2,
	2,
	6,
	1,
	2,
	1,
	3,
};

const u8 FID_CMDlen = 20;	//��̬�����Զ���ָ���޳�
u8 FID_CMDptr; 		//��̬�����Զ���ָ���α�
u8 FID_CMDusr[];	//��̬�����Զ���ָ���

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
	
		result += (u16)dats[loop];
	}
	return result;
}

/*���ݻ������ָ�����ָ�����*/
u16 FIDframeLoad_TX(u8 bufs[],u8 cmd[],u16 cmdlen){  //��ָ���ʽװ�ؽ����ݻ�������������ݰ�����ָ�����ָ���뼰ָ�����У����ǰ��������
	
	u16 temp;
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
	memcpy(&bufs[memp],cmd,cmdlen);		//ָ������ ��ָ�������ָ���뼰ָ�����У����ǰ��������
	memp += cmdlen;	//ָ�����
	temp = ADD_CHECK(&bufs[6],memp - 6); //��У�����
	bufs[memp ++] = (u8)(temp >> 8);	//��У����䣬��ָ��
	bufs[memp ++] = (u8)temp;	

	return memp;	//���������α�ָ�룬������
}

/******��ָ���ŷ���ָ��********************/
/******CMD_IDΪ0xffʱ�����Ͷ�̬����ָ��******/
EGUARD_MEAS *fingerID_CMDTX(u8 CMD_ID,u8 rpt_num){	//ָ���ţ��ط�����

	const u16 FRAME_SIZE = 100; //���ݰ����泤��
	u16 TX_num;		//ʵ�ʷ������ݰ�����
	u8  TX_BUF[FRAME_SIZE],RX_BUF[FRAME_SIZE]; //���ݰ�����
	u8  datsbuf[FRAME_SIZE - 20]; //ָ�������
	char  *p_rec;	//���ܰ�����ָ��
	u16 ADD_RES,RX_num;	
	u8  TX_CNT = 0;
	EGUARD_MEAS *result = NULL;
	
	do{result = (EGUARD_MEAS *)osPoolCAlloc(EGUD_pool);}while(result == NULL);	//���뷵����������ָ��
	
	osDelay(50);
	memset(TX_BUF,0,FRAME_SIZE * sizeof(u8));	//�������
	memset(datsbuf,0,(FRAME_SIZE - 20) * sizeof(u8));
	if(CMD_ID != 0xff){		//�Ƿ�Ϊ��ָ̬��
		
		memcpy(datsbuf,FID_CMD[CMD_ID],FID_CMDSIZE[CMD_ID]);	//ָ��װ��
		TX_num = FIDframeLoad_TX(TX_BUF,datsbuf,FID_CMDSIZE[CMD_ID]);	//ָ���װ������ݰ�����
	}else{
	
		memcpy(datsbuf,FID_CMDusr,FID_CMDptr);
		TX_num = FIDframeLoad_TX(TX_BUF,datsbuf,FID_CMDptr);
	}

	do{
		Driver_USART1.Send(TX_BUF,TX_num);	//���ݷ���
		osDelay(10);
		Driver_USART1.Receive(RX_BUF,FRAME_SIZE);		
		osDelay(100);						
		p_rec = strstr((const char*)RX_BUF,(const char*)FID_FrameHead);
		TX_CNT ++;
		if(p_rec){	//֡ͷУ��
		
			if(p_rec[6] == FID_IDENT_ANS){	//��ʶУ��
			
				RX_num  = 0;
				RX_num |= ((u16)p_rec[7]) << 8;		//ȡ����
				RX_num |= (u16)p_rec[8];
				
				ADD_RES = ADD_CHECK((u8 *)&p_rec[6],RX_num + 1);
				if(((u8)ADD_RES >> 8) == p_rec[RX_num + 7] && (u8)ADD_RES == p_rec[RX_num + 8]){	//��У��
					
//					Driver_USART2.Send(&p_rec[9],1);	 /****�������*****/
//					osDelay(100);						 /****�������*****/
				
					if(!p_rec[9]){	//�ɹ�ʶ��,���ݴ���,ȷ����ʶ��
					
						result -> CMD = FID_EXERES_SUCCESS;
						if(RX_num > 3)result -> DAT = p_rec[11];	//���ݳ��ȴ���3��Ԥ������������Ӧ����ID��ȡ��page����Ϊ2���ֽڣ���Ϊ��λ��Page1-255��ȡ���ֽ�
						else result -> DAT = 0;
						osDelay(100);
						return result;
					}	
				}
			}
		}
		memset(RX_BUF,0,FRAME_SIZE * sizeof(u8));	//���ݰ��������
		osDelay(100);	
	}while(TX_CNT < rpt_num);	//�ظ������þ�
	
	result -> CMD = FID_EXERES_FAIL;	//ʶ��ʧ��
	result -> DAT = 0;
	osDelay(100);
	return result;
}

void fingerID_Thread(const void *argument){
	
	EGUARD_MEAS *mptr = NULL;		//������Ϣ���л���
	EGUARD_MEAS *rptr = NULL;		//������Ϣ���л���
	EGUARD_MEAS *sptr = NULL;		//ָ��ָ���
	osEvent   evt;	 	//�¼�����
	osStatus  status;
	
	u8 loop;
	const u8 cmdQ_fidSave_len = 5;
	const u8 cmdQ_fidIden_len = 3;
	const u8 cmdQ_fidSave[cmdQ_fidSave_len] = {0,1,0,2,4};	//ָ�ƴ洢ָ����У�5����ָ̬�� + 1����̬����ָ�/**ָ��ɾ����һ����̬����ָ��޾�ָ̬���**/
	const u8 cmdQ_fidIden[cmdQ_fidIden_len] = {0,1,3}; //ָ��ʶ��ָ����У�3����ָ̬��
	const u8 TX_rept = 2;
	
	bool cmd_continue = false;

	osThreadTerminate(tid_USARTDebug_Thread);   //�ر�debug���������軥��
	USART1fingerID_Init();	//���³�ʼ��
	
	for(;;){
	
		evt = osMessageGet(MsgBox_MTEGUD_FID, 200);
		if (evt.status == osEventMessage){		//�ȴ���Ϣָ��
		 
			rptr = evt.value.p;
			switch(rptr -> CMD){
			
				case FID_MSGCMD_FIDSAVE:	//ִ��ָ�ƴ洢ָ��
					
						do{mptr = (EGUARD_MEAS *)osPoolCAlloc(EGUD_pool);}while(mptr == NULL); 	//�ⷢ��Ϣ�ڴ�����
					
						osDelay(3000);
						sptr = fingerID_CMDTX(0,20);
						if(sptr -> CMD == FID_EXERES_SUCCESS){		//ɨ��20���Ƿ�����ָȡ���
							
							do{status = osPoolFree(EGUD_pool, sptr);}while(status != osOK);
							sptr = NULL;							
							osDelay(100);
							for(loop = 0;loop < cmdQ_fidSave_len;loop ++){
							
								//-----------------------------------------------------------//�������;�ָ̬�
								sptr = fingerID_CMDTX(cmdQ_fidSave[loop],TX_rept );	//��ָ̬����з���
								if(sptr -> CMD == FID_EXERES_SUCCESS){		//ȡ���
									
									if(loop == cmdQ_fidSave_len - 1)cmd_continue = true;	//ǰ��������ָ̬����ɹ�ִ�У���ʹ�����һ����̬����ָ��
									osDelay(500);
								}else{
									
									osDelay(200);
									mptr -> CMD = FID_EXERES_FAIL;								
									mptr -> DAT = 0x00;	
									osMessagePut(MsgBox_EGUD, (uint32_t)mptr, 100);
									do{status = osPoolFree(EGUD_pool, sptr);}while(status != osOK);
									sptr = NULL;	
									break;
								}
								do{status = osPoolFree(EGUD_pool, sptr);}while(status != osOK);
								sptr = NULL;
							}
							
							if(cmd_continue){	//ִ�����һ����̬����ָ��
								
								cmd_continue = false;
							
								//-----------------------------------------------------------//(��̬����ָ��)
								memset(FID_CMDusr, 0, FID_CMDlen * sizeof(u8));	//�������
								memcpy(FID_CMDusr,FID_CMD[5],FID_CMDSIZE[5]);	//ָ���ָ���
								FID_CMDusr[FID_CMDSIZE[5]] = 0x00;	//��Ϊҳ���������ƣ�pageҳ��߰�λ�ֽ�0���
								FID_CMDusr[FID_CMDSIZE[5] + 1] = rptr -> DAT;	//��λ��ֻ�ṩ���ֽڵͰ�λ
								FID_CMDptr = FID_CMDSIZE[5] + 2; //ָ��� +2
								sptr = fingerID_CMDTX(0xff,TX_rept);	//�����Զ��嶯ָ̬��ؽ��
								if(sptr -> CMD == FID_EXERES_SUCCESS){	//ȡ�����ͬʱ����ָ����гɹ�ִ�����
								
									mptr -> CMD = FID_EXERES_SUCCESS;	//�����ݷ����������������������0x00���
									mptr -> DAT = 0x00;		
									osMessagePut(MsgBox_EGUD, (uint32_t)mptr, 100);
								}else{
									
									mptr -> CMD = FID_EXERES_FAIL;
									mptr -> DAT = 0x00;	
									osMessagePut(MsgBox_EGUD, (uint32_t)mptr, 100);
								}osDelay(200);
								do{status = osPoolFree(EGUD_pool, sptr);}while(status != osOK);
								sptr = NULL;
							}
						}else{
							
							osDelay(200);
							mptr -> CMD = FID_EXERES_FAIL;								
							mptr -> DAT = 0x00;	
							osMessagePut(MsgBox_EGUD, (uint32_t)mptr, 100);
							do{status = osPoolFree(EGUD_pool, sptr);}while(status != osOK);
							sptr = NULL;
							break;
						}
						osPoolFree(EGUD_pool, sptr);	//�ͷ�ָ��ָ����ڴ�
						sptr = NULL;
						break;	   
					
				case FID_MSGCMD_FIDDELE:		//ִ��ָ��ɾ��ָ��
					
						do{mptr = (EGUARD_MEAS *)osPoolCAlloc(EGUD_pool);}while(mptr == NULL); 	//�ⷢ��Ϣ�ڴ�����
					
						//-----------------------------------------------------------//(��̬����ָ��)
						memset(FID_CMDusr, 0, FID_CMDlen * sizeof(u8));	//�������
						memcpy(FID_CMDusr,FID_CMD[6],FID_CMDSIZE[6]);	//ָ���ָ���
						FID_CMDusr[FID_CMDSIZE[6]] = 0x00;	//��Ϊҳ���������ƣ�pageҳ��߰�λ�ֽ�0���
						FID_CMDusr[FID_CMDSIZE[6] + 1] = rptr -> DAT;	//��λ��ֻ�ṩ���ֽڵͰ�λ
						FID_CMDusr[FID_CMDSIZE[6] + 2] = 0x00;	//ָ��򻯣�ֻɾ��ָ��pageID����ɾ������Ϊ1
						FID_CMDusr[FID_CMDSIZE[6] + 3] = 0x01;
						FID_CMDptr = FID_CMDSIZE[6] + 4;	//ָ��� +4
						sptr = fingerID_CMDTX(0xff,TX_rept);	//�����Զ��嶯ָ̬��ؽ��
						if(sptr -> CMD == FID_EXERES_SUCCESS){	//ȡ�����ͬʱ����ָ����гɹ�ִ�����
						
							mptr -> CMD = FID_EXERES_SUCCESS;	//�����ݷ����������������������0x00���
							mptr -> DAT = 0x00;				
						}else{
							
							mptr -> CMD = FID_EXERES_FAIL;
							mptr -> DAT = 0x00;								
						}
						osMessagePut(MsgBox_EGUD, (uint32_t)mptr, 100);
						do{status = osPoolFree(EGUD_pool, sptr);}while(status != osOK);	//�ͷ�ָ��ָ����ڴ�
						sptr = NULL;
						sptr = fingerID_CMDTX(0,1);			//������������ã���ϴ��ָ��
						do{status = osPoolFree(EGUD_pool, sptr);}while(status != osOK);	//�ͷ�ָ��ָ����ڴ�
						sptr = NULL;
						break;
					
				case FID_MSGCMD_FIDIDEN:		//ִ��ָ��ʶ��ָ�����
					
						do{mptr = (EGUARD_MEAS *)osPoolCAlloc(EGUD_pool);}while(mptr == NULL); 	//�ⷢ��Ϣ�ڴ�����
				
						for(loop = 0;loop < cmdQ_fidIden_len;loop ++){
						
							//-----------------------------------------------------------//�������;�ָ̬�
							sptr = fingerID_CMDTX(cmdQ_fidIden[loop],TX_rept );	//��ָ̬����з���
							if(sptr -> CMD == FID_EXERES_SUCCESS){//ȡ���
								
								if(loop == cmdQ_fidIden_len - 1){	//����ָ����гɹ�ִ�����
								
									mptr -> CMD = FID_EXERES_SUCCESS;	//�����ݷ���
									mptr -> DAT = sptr -> DAT;
									osMessagePut(MsgBox_EGUD, (uint32_t)mptr, 100);
								}
								osDelay(200);
							}else{
		
								mptr -> CMD = FID_EXERES_FAIL;
								mptr -> DAT = 0x00;	
								osMessagePut(MsgBox_EGUD, (uint32_t)mptr, 100);
								do{status = osPoolFree(EGUD_pool, sptr);}while(status != osOK);	//�ͷ�ָ��ָ����ڴ�
								sptr = NULL;
								break;
							}osDelay(500);
							
							do{status = osPoolFree(EGUD_pool, sptr);}while(status != osOK);	//�ͷ�ָ��ָ����ڴ�
							sptr = NULL;
						}
						break;
						
				default:break;
			}
			osPoolFree(EGUD_pool, rptr);	//�ͷ���Ϣ�����ڴ�
		}
		/*��λ��������ָ�����ִ������ѭ�����*/
		
		sptr = fingerID_CMDTX(0,1);
		if(sptr -> CMD == FID_EXERES_SUCCESS){
			
			do{status = osPoolFree(EGUD_pool, sptr);}while(status != osOK);
			sptr = NULL;
	
			for(loop = 0;loop < cmdQ_fidIden_len;loop ++){
			
				//-----------------------------------------------------------//�������;�ָ̬�
				sptr = fingerID_CMDTX(cmdQ_fidIden[loop],TX_rept );	//��ָ̬����з���
				if(sptr -> CMD == FID_EXERES_SUCCESS){//ȡ���
					
					if(loop == cmdQ_fidIden_len - 1){	//����ָ����гɹ�ִ�����
						
						do{mptr = (EGUARD_MEAS *)osPoolCAlloc(EGUD_pool);}while(mptr == NULL); 	//�ⷢ��Ϣ�ڴ�����
						mptr -> CMD = FID_EXERES_TTIT;	//�����ݷ���,�����ϴ�
						mptr -> DAT = sptr -> DAT;
						osMessagePut(MsgBox_EGUD, (uint32_t)mptr, 100);
						
						do{mptr = (EGUARD_MEAS *)osPoolCAlloc(EGUD_pool);}while(mptr == NULL); 	//1.44LCD����
						mptr -> CMD = FID_EXERES_TTIT;	
						mptr -> DAT = sptr -> DAT;
						osMessagePut(MsgBox_DPEGUD, (uint32_t)mptr, 100);
						
						beeps(2);
					}
					osDelay(200);
				}else{					//�������ڼ�⣬ʧ���޶���
					
//					do{mptr = (EGUARD_MEAS *)osPoolCAlloc(EGUD_pool);}while(mptr == NULL); 	//�ⷢ��Ϣ�ڴ�����
//					mptr -> CMD = FID_EXERES_FAIL;
//					mptr -> DAT = 0x00;	
//					osMessagePut(MsgBox_EGUD, (uint32_t)mptr, 100);
					do{status = osPoolFree(EGUD_pool, sptr);}while(status != osOK);
					sptr = NULL;
					break;
				}osDelay(500);
				
				do{status = osPoolFree(EGUD_pool, sptr);}while(status != osOK);
				sptr = NULL;
			}
		}else {do{status = osPoolFree(EGUD_pool, sptr);}while(status != osOK); sptr = NULL;}
		osDelay(20);
	}
}

void fingerIDThread_Active(void){
	
	tid_fingerID_Thread	= osThreadCreate(osThread(fingerID_Thread),NULL);
}

