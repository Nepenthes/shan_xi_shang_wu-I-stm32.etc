#include "doorLock.h"

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_doorLock_Thread;
osThreadDef(doorLock_Thread,osPriorityNormal,1,512);

void doorLock_Init(void){

	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	 //ʹ��PB�˿�ʱ��

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);					 //�����趨������ʼ��GPIOA.	
	
	PAout(0) = 1;
}

void doorLock_Thread(const void *argument){

	osEvent  evt;
    osStatus status;	
	
	const u8 doorOpenTime = 50;
	static bool DL_OPENflg = false;
	u8 DLcnt;
	
	u8 UPLD_cnt;
	const u8 UPLD_period = 15;
	
	EGUARD_MEAS *rptr = NULL;
	EGUARD_MEAS *mptr = NULL;
	
	for(;;){
	
		evt = osMessageGet(MsgBox_MTEGUD_DLOCK, 100);
		if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
			
			rptr = evt.value.p;
			/*�Զ��屾���߳̽������ݴ��������������������������*/
			
			if(rptr->CMD == DLOCK_MSGCMD_LOCK){
			
				if(rptr->DAT == CMD_DOOROPEN){
				
					DL_OPENflg = true;
					DLcnt = doorOpenTime;
					DLOCK_CON = 0;
					
					do{mptr = (EGUARD_MEAS *)osPoolCAlloc(EGUD_pool);}while(mptr == NULL); 	//1.44LCD����
					mptr -> CMD = DLOCK_MSGCMD_LOCK;	
					mptr -> DAT = CMD_DOOROPEN;
					osMessagePut(MsgBox_DPEGUD, (uint32_t)mptr, 100);
					beeps(0);
				}else if(rptr->DAT == CMD_DOORCLOSE){
				
					DLOCK_CON = 1;
					
					DL_OPENflg = false;
					DLcnt 	   = 0;
					
					do{mptr = (EGUARD_MEAS *)osPoolCAlloc(EGUD_pool);}while(mptr == NULL); 	//1.44LCD����
					mptr -> CMD = DLOCK_MSGCMD_LOCK;	
					mptr -> DAT = CMD_DOORCLOSE;
					osMessagePut(MsgBox_DPEGUD, (uint32_t)mptr, 100);
					beeps(0);
				}
			}
			
			do{status = osPoolFree(EGUD_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
			rptr = NULL;
		}
		
		if(DL_OPENflg){
		
			if(DLcnt){
			
				DLcnt --;
			}else{
			
				DL_OPENflg = false;
				DLOCK_CON = 1;
				do{mptr = (EGUARD_MEAS *)osPoolCAlloc(EGUD_pool);}while(mptr == NULL); 	//1.44LCD����
				mptr -> CMD = DLOCK_MSGCMD_LOCK;	
				mptr -> DAT = CMD_DOORCLOSE;
				osMessagePut(MsgBox_DPEGUD, (uint32_t)mptr, 100);
				beeps(0);
			}
		}
		
		if(UPLD_cnt < UPLD_period)UPLD_cnt ++;		//����״̬��ʱ�ϴ�
		else{
		
			UPLD_cnt = 0;
			do{mptr = (EGUARD_MEAS *)osPoolCAlloc(EGUD_pool);}while(mptr == NULL);	//�������ݴ�����Ϣ����
			mptr->CMD = DLOCK_EXERES_TTIT;
			mptr->DAT = DL_OPENflg;
			osMessagePut(MsgBox_EGUD, (uint32_t)mptr, osWaitForever);
		}
		
		osDelay(20);
	}
}

void doorLockThread_Active(void){

	doorLock_Init();
	tid_doorLock_Thread = osThreadCreate(osThread(doorLock_Thread),NULL);
}
