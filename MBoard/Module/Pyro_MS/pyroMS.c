#include "pyroMS.h"

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_pyroMS_Thread;
osThreadDef(pyroMS_Thread,osPriorityNormal,1,512);
			 
osPoolId  pyroMS_pool;								 
osPoolDef(pyroMS_pool, 10, pyroMS_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_pyroMS;
osMessageQDef(MsgBox_pyroMS, 2, &pyroMS_MEAS);            // ��Ϣ���ж��壬����ģ���߳�������ͨѶ�߳�
osMessageQId  MsgBox_MTpyroMS;
osMessageQDef(MsgBox_MTpyroMS, 2, &pyroMS_MEAS);          // ��Ϣ���ж���,��������ͨѶ�߳���ģ���߳�
osMessageQId  MsgBox_DPpyroMS;
osMessageQDef(MsgBox_DPpyroMS, 2, &pyroMS_MEAS);          // ��Ϣ���ж��壬����ģ���߳�����ʾģ���߳�

pyroDIO_Init(void){

	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	 //ʹ��PB�˿�ʱ��

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);		
}

void pyroMS_Init(void){

	pyroDIO_Init();
}

void pyroMS_Thread(const void *argument){
	
	osEvent  evt;
    osStatus status;	
	
	const uint8_t dpSize = 30;
	const uint8_t dpPeriod = 40;
	
	static uint8_t Pcnt = 0;
	char disp[dpSize];
	
	pyroMS_MEAS	sensorData;
	static pyroMS_MEAS Data_temp = {1};
	
	pyroMS_MEAS *mptr = NULL;
	fireMS_MEAS *rptr = NULL;
	
	for(;;){
		
		evt = osMessageGet(MsgBox_MTpyroMS, 100);
		if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
			
			rptr = evt.value.p;
			/*�Զ��屾���߳̽������ݴ��������������������������*/
			

			do{status = osPoolFree(pyroMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
			rptr = NULL;
		}
		
		sensorData.VAL = PYRO_DATA;	//���ݲɼ�
		
		if(Data_temp.VAL != sensorData.VAL){	//�������ͣ����ݸ���ʱ�Ŵ�����
		
			Data_temp.VAL = sensorData.VAL;
			
			do{mptr = (pyroMS_MEAS *)osPoolCAlloc(pyroMS_pool);}while(mptr == NULL);
			mptr->VAL = sensorData.VAL;
			osMessagePut(MsgBox_pyroMS, (uint32_t)mptr, 100);
			
			do{mptr = (pyroMS_MEAS *)osPoolCAlloc(pyroMS_pool);}while(mptr == NULL);
			mptr->VAL = sensorData.VAL;
			osMessagePut(MsgBox_DPpyroMS, (uint32_t)mptr, 100);
			osDelay(10);
		}
		
		if(Pcnt < dpPeriod){osDelay(10);Pcnt ++;}
		else{
		
			Pcnt = 0;
			memset(disp,0,dpSize * sizeof(char));
			sprintf(disp,"\n\ris anybody here now? : %d\n\r", !sensorData.VAL);			
			Driver_USART1.Send(disp,strlen(disp));
			osDelay(20);
		}
		
		osDelay(10);
	}
}

void pyroMSThread_Active(void){

	static bool memInit_flg = false;
	
	if(!memInit_flg){
	
		pyroMS_pool   = osPoolCreate(osPool(pyroMS_pool));	//�����ڴ��
		MsgBox_pyroMS 	= osMessageCreate(osMessageQ(MsgBox_pyroMS), NULL);   //������Ϣ����
		MsgBox_MTpyroMS = osMessageCreate(osMessageQ(MsgBox_MTpyroMS), NULL);//������Ϣ����
		MsgBox_DPpyroMS = osMessageCreate(osMessageQ(MsgBox_DPpyroMS), NULL);//������Ϣ����
		
		memInit_flg = true;
	}

	pyroMS_Init();
	tid_pyroMS_Thread = osThreadCreate(osThread(pyroMS_Thread),NULL);
}
