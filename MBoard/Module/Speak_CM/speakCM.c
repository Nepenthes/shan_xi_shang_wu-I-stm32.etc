#include "speakCM.h"

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_speakCM_Thread;
osThreadDef(speakCM_Thread,osPriorityNormal,1,512);
			 
osPoolId  speakCM_pool;								 
osPoolDef(speakCM_pool, 10, speakCM_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_speakCM;
osMessageQDef(MsgBox_speakCM, 2, &speakCM_MEAS);            // ��Ϣ���ж��壬����ģ���߳�������ͨѶ�߳�
osMessageQId  MsgBox_MTspeakCM;
osMessageQDef(MsgBox_MTspeakCM, 2, &speakCM_MEAS);          // ��Ϣ���ж���,��������ͨѶ�߳���ģ���߳�
osMessageQId  MsgBox_DPspeakCM;
osMessageQDef(MsgBox_DPspeakCM, 2, &speakCM_MEAS);          // ��Ϣ���ж��壬����ģ���߳�����ʾģ���߳�

void speakCM_Init(void){

	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);	 //ʹ��PB,PE�˿�ʱ��

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_12;	 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);					 //�����趨������ʼ��GPIOB.5	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;	 
	GPIO_Init(GPIOB, &GPIO_InitStructure);					 //�����趨������ʼ��GPIOB.5	
}

void NVCdat_in(uint8_t dat){

	uint8_t i;
	
	PA1 = 0;
	PA0 = 1;
	
	osDelay(5); /*5ms */
	for(i=0;i<8;i++){
		
		PA1 = 1;
		if(dat & 1){
			
			delay_us(1200); /* 1200us */
			PA1 = 0;
			delay_us(400); /* 400us */
		}else{
			
			delay_us(400);
			PA1 = 0;
			delay_us(1200);
		}
		dat >>= 1;
	}
	PA1 = 1;
	osDelay(50);
}

void SPK_Select(uint8_t num,uint8_t vol){

	if(num > 14)return;
	
	NVCdat_in(0xe0 + vol);
	//NVCdat_in(0xf2);	//ѭ������ʹ��
	NVCdat_in(num);
}

void speakCM_Thread(const void *argument){

	osEvent  evt;
    osStatus status;	

	speakCM_MEAS actuatorData;	//����������
	static speakCM_MEAS  Data_temp   = {1};	//��������������ͬ���ԱȻ���
	static speakCM_MEAS  Data_tempDP = {1};	//������������ʾ���ݶԱȻ���
	
	speakCM_MEAS *mptr = NULL;
	speakCM_MEAS *rptr = NULL;
	
	SPK_EN = 1;
	for(;;){
	
		
		evt = osMessageGet(MsgBox_MTspeakCM, 100);
		if (evt.status == osEventMessage){		//�ȴ���Ϣָ��
			
			rptr = evt.value.p;
			/*�Զ��屾���߳̽������ݴ��������������������������*/
			
			actuatorData.spk_num = rptr->spk_num;

			do{status = osPoolFree(speakCM_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
			rptr = NULL;
		}
		
		if(Data_temp.spk_num != actuatorData.spk_num){
		
			Data_temp.spk_num = actuatorData.spk_num;
			SPK_Select(actuatorData.spk_num,6);
		}
	
		if(Data_tempDP.spk_num != actuatorData.spk_num){
		
			Data_tempDP.spk_num = actuatorData.spk_num;
			
			do{mptr = (speakCM_MEAS *)osPoolCAlloc(speakCM_pool);}while(mptr == NULL);	//1.44��Һ����ʾ��Ϣ����
			mptr->spk_num = actuatorData.spk_num;
			osMessagePut(MsgBox_DPspeakCM, (uint32_t)mptr, 100);
			osDelay(10);
		}
			
//		for(temp = 0;temp < 14;temp ++){		/**�������**/
//		
//			SPK_Select(temp,6);
//			osDelay(3000);
//			SPK_STP;
//		}
	}
}

void speakCMThread_Active(void){

	static bool memInit_flg = false;
	
	if(!memInit_flg){
	
		speakCM_pool   = osPoolCreate(osPool(speakCM_pool));	//�����ڴ��
		MsgBox_speakCM 	= osMessageCreate(osMessageQ(MsgBox_speakCM), NULL);   //������Ϣ����
		MsgBox_MTspeakCM = osMessageCreate(osMessageQ(MsgBox_MTspeakCM), NULL);//������Ϣ����
		MsgBox_DPspeakCM = osMessageCreate(osMessageQ(MsgBox_DPspeakCM), NULL);//������Ϣ����
		
		memInit_flg = true;
	}

	speakCM_Init();
	tid_speakCM_Thread = osThreadCreate(osThread(speakCM_Thread),NULL);
}
