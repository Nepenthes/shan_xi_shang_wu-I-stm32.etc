#include "RelaysCM.h"

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_RelaysCM_Thread;
osThreadDef(RelaysCM_Thread,osPriorityNormal,1,512);
			 
osPoolId  RelaysCM_pool;								 
osPoolDef(RelaysCM_pool, 10, RelaysCM_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_RelaysCM;
osMessageQDef(MsgBox_RelaysCM, 2, &RelaysCM_MEAS);            // ��Ϣ���ж��壬����ģ���߳�������ͨѶ�߳�
osMessageQId  MsgBox_MTRelaysCM;
osMessageQDef(MsgBox_MTRelaysCM, 2, &RelaysCM_MEAS);          // ��Ϣ���ж���,��������ͨѶ�߳���ģ���߳�
osMessageQId  MsgBox_DPRelaysCM;
osMessageQDef(MsgBox_DPRelaysCM, 2, &RelaysCM_MEAS);          // ��Ϣ���ж��壬����ģ���߳�����ʾģ���߳�

void RelaysCM_Init(void){

	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	PAout(0) = PAout(1) = 0;
}

void RelaysCM_Thread(const void *argument){

	osEvent  evt;
    osStatus status;	
	
	u8 UPLD_cnt;
	const u8 UPLD_period = 5;

	RelaysCM_MEAS actuatorData;	//����������
	static RelaysCM_MEAS  Data_temp   = {0};	//��������������ͬ���ԱȻ���
	static RelaysCM_MEAS  Data_tempDP = {0};	//������������ʾ���ݶԱȻ���
	
	RelaysCM_MEAS *mptr = NULL;
	RelaysCM_MEAS *rptr = NULL;
	
	for(;;){
	
		evt = osMessageGet( MsgBox_MTRelaysCM, 100);
		if (evt.status == osEventMessage){		//�ȴ���Ϣָ��
			
			rptr = evt.value.p;
			/*�Զ��屾���߳̽������ݴ��������������������������*/
			
			actuatorData.relay_con = rptr->relay_con;

			do{status = osPoolFree(RelaysCM_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
			rptr = NULL;
		}
		
		if(Data_temp.relay_con != actuatorData.relay_con){
		
			Data_temp.relay_con = actuatorData.relay_con;
			
			PAout(0) = (Data_temp.relay_con >> 0) & 0x01;
			PAout(1) = (Data_temp.relay_con >> 1) & 0x01;
		}
	
		if(Data_tempDP.relay_con != actuatorData.relay_con){
		
			Data_tempDP.relay_con = actuatorData.relay_con;
			
			do{mptr = (RelaysCM_MEAS *)osPoolCAlloc(RelaysCM_pool);}while(mptr == NULL);	//1.44��Һ����ʾ��Ϣ����
			mptr->relay_con = actuatorData.relay_con;
			osMessagePut(MsgBox_DPRelaysCM, (uint32_t)mptr, 100);
			osDelay(10);
		}

		if(UPLD_cnt < UPLD_period)UPLD_cnt ++;	//���ݶ�ʱ�ϴ�
		else{
		
			UPLD_cnt = 0;
			
			do{mptr = (RelaysCM_MEAS *)osPoolCAlloc(RelaysCM_pool);}while(mptr == NULL);
			mptr->relay_con = actuatorData.relay_con;
			osMessagePut(MsgBox_RelaysCM, (uint32_t)mptr, 100);
			osDelay(10);
		}
	}
}

void RelaysCMThread_Active(void){

	RelaysCM_Init();
	RelaysCM_pool   = osPoolCreate(osPool(RelaysCM_pool));	//�����ڴ��
	MsgBox_RelaysCM 	= osMessageCreate(osMessageQ(MsgBox_RelaysCM), NULL);	//������Ϣ����
	MsgBox_MTRelaysCM = osMessageCreate(osMessageQ(MsgBox_MTRelaysCM), NULL);//������Ϣ����
	MsgBox_DPRelaysCM = osMessageCreate(osMessageQ(MsgBox_DPRelaysCM), NULL);//������Ϣ����
	tid_RelaysCM_Thread = osThreadCreate(osThread(RelaysCM_Thread),NULL);
}
