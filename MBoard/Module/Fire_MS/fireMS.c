#include "fireMS.h"

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_fireMS_Thread;
osThreadDef(fireMS_Thread,osPriorityNormal,1,512);
			 
osPoolId  fireMS_pool;								 
osPoolDef(fireMS_pool, 10, fireMS_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_fireMS;
osMessageQDef(MsgBox_fireMS, 2, &fireMS_MEAS);            // ��Ϣ���ж��壬����ģ�����������ͨѶ����
osMessageQId  MsgBox_MTfireMS;
osMessageQDef(MsgBox_MTfireMS, 2, &fireMS_MEAS);          // ��Ϣ���ж���,��������ͨѶ������ģ�����
osMessageQId  MsgBox_DPfireMS;
osMessageQDef(MsgBox_DPfireMS, 2, &fireMS_MEAS);          // ��Ϣ���ж��壬����ģ���������ʾģ�����

void fireDIO_Init(void){

	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	 //ʹ��PB�˿�ʱ��

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);		
}

void fireMS_Init(void){

	fireDIO_Init();
}

void fireMS_Thread(const void *argument){

	osEvent  evt;
    osStatus status;	
	
	const bool UPLOAD_MODE = false;	//1�����ݱ仯ʱ���ϴ� 0�����ڶ�ʱ�ϴ�
	
	const uint8_t upldPeriod = 5;	//�����ϴ�����������UPLOAD_MODE = false ʱ��Ч��
	
	uint8_t UPLDcnt = 0;
	bool UPLD_EN = false;

	const uint8_t dpSize = 30;
	const uint8_t dpPeriod = 40;
	
	static uint8_t Pcnt = 0;
	char disp[dpSize];
	
	fireMS_MEAS	sensorData;
	static fireMS_MEAS Data_temp = {1};
	static fireMS_MEAS Data_tempDP = {1};
	
	fireMS_MEAS *mptr = NULL;
	fireMS_MEAS *rptr = NULL;
	
	for(;;){
		
	/***********************���ؽ������ݽ���***************************************************/
	//�������������ݽ����ϴ����������ݹ��ܱ�������ʱ����
		evt = osMessageGet(MsgBox_MTfireMS, 100);
		if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
			
			rptr = evt.value.p;
			/*�Զ��屾�ؽ��̽������ݴ��������������������������*/
			

			do{status = osPoolFree(fireMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
			rptr = NULL;
		}

	/***********************�������ݲɼ�*****************************************************/
		sensorData.VAL = FIRE_DATA;		//���ݲɼ�
		
		if(!UPLOAD_MODE){	//ѡ���ϴ�����ģʽ
		
			if(UPLDcnt < upldPeriod)UPLDcnt ++;
			else{
			
				UPLDcnt = 0;
				UPLD_EN = true;
			}
		}else{
		
			if(Data_temp.VAL != sensorData.VAL){	//�������ͣ����ݸ���ʱ�Ŵ�����
				
				Data_temp.VAL = sensorData.VAL;
				UPLD_EN = true;
			}
		}

	/***********************������������*****************************************************/		
		if(UPLD_EN){
			
			UPLD_EN = false;
			
			do{mptr = (fireMS_MEAS *)osPoolCAlloc(fireMS_pool);}while(mptr == NULL);	//�������ݴ�����Ϣ����
			mptr->VAL = sensorData.VAL;
			osMessagePut(MsgBox_fireMS, (uint32_t)mptr, 100);
			osDelay(500);
		}
		
		if(Data_tempDP.VAL != sensorData.VAL){	//�������ͣ����ݸ���ʱ�Ŵ�����
		
			Data_tempDP.VAL = sensorData.VAL;
			
			do{mptr = (fireMS_MEAS *)osPoolCAlloc(fireMS_pool);}while(mptr == NULL);	//1.44��Һ����ʾ��Ϣ����
			mptr->VAL = sensorData.VAL;
			osMessagePut(MsgBox_DPfireMS, (uint32_t)mptr, 100);
			osDelay(10);
		}
	/***********************Debug_log*********************************************************/		
		if(Pcnt < dpPeriod){osDelay(10);Pcnt ++;}
		else{
		
			Pcnt = 0;
			memset(disp,0,dpSize * sizeof(char));
			sprintf(disp,"\n\ris firing now? : %d\n\r", Data_temp.VAL);			
			Driver_USART1.Send(disp,strlen(disp));
			osDelay(20);
		}
		osDelay(10);
	}
}

void fireMSThread_Active(void){

	static bool memInit_flg = false;
	
	if(!memInit_flg){
	
		fireMS_pool   = osPoolCreate(osPool(fireMS_pool));	//�����ڴ��
		MsgBox_fireMS 	= osMessageCreate(osMessageQ(MsgBox_fireMS), NULL);   //������Ϣ����
		MsgBox_MTfireMS = osMessageCreate(osMessageQ(MsgBox_MTfireMS), NULL);//������Ϣ����
		MsgBox_DPfireMS = osMessageCreate(osMessageQ(MsgBox_DPfireMS), NULL);//������Ϣ����
		
		memInit_flg = true;
	}

	fireMS_Init();
	tid_fireMS_Thread = osThreadCreate(osThread(fireMS_Thread),NULL);
}
