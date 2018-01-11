#include "gasMS.h"

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_gasMS_Thread;
osThreadDef(gasMS_Thread,osPriorityNormal,1,512);
			 
osPoolId  gasMS_pool;								 
osPoolDef(gasMS_pool, 10, gasMS_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_gasMS;
osMessageQDef(MsgBox_gasMS, 2, &gasMS_MEAS);            // ��Ϣ���ж��壬����ģ���߳�������ͨѶ�߳�
osMessageQId  MsgBox_MTgasMS;
osMessageQDef(MsgBox_MTgasMS, 2, &gasMS_MEAS);          // ��Ϣ���ж���,��������ͨѶ�߳���ģ���߳�
osMessageQId  MsgBox_DPgasMS;
osMessageQDef(MsgBox_DPgasMS, 2, &gasMS_MEAS);          // ��Ϣ���ж��壬����ģ���߳�����ʾģ���߳�

void gasMS_DIOinit(void){

	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	 //ʹ��PB�˿�ʱ��

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);					 
}

void gasMS_AIOinit(void){

	ADC_InitTypeDef ADC_InitStructure; 
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);	  //ʹ��ADC1ͨ��ʱ��
 

	RCC_ADCCLKConfig(RCC_PCLK2_Div6);   //����ADC��Ƶ����6 72M/6=12,ADC���ʱ�䲻�ܳ���14M


	//PC0 1 ��Ϊģ��ͨ����������  ADC12_IN8                       

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		//ģ����������
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
	
	ADC_DeInit(ADC1);  //��λADC1,������ ADC1 ��ȫ���Ĵ�������Ϊȱʡֵ

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	//ADC����ģʽ:ADC1��ADC2�����ڶ���ģʽ
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;	//ģ��ת�������ڵ�ͨ��ģʽ
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;	//ģ��ת�������ڵ���ת��ģʽ
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//ת��������������ⲿ��������
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	//ADC�����Ҷ���
	ADC_InitStructure.ADC_NbrOfChannel = 1;	//˳����й���ת����ADCͨ������Ŀ
	ADC_Init(ADC1, &ADC_InitStructure);	//����ADC_InitStruct��ָ���Ĳ�����ʼ������ADCx�ļĴ���   

  
	ADC_Cmd(ADC1, ENABLE);	//ʹ��ָ����ADC1
	
	ADC_ResetCalibration(ADC1);	//ʹ�ܸ�λУ׼  
	 
	while(ADC_GetResetCalibrationStatus(ADC1));	//�ȴ���λУ׼����
	
	ADC_StartCalibration(ADC1);	 //����ADУ׼
 
	while(ADC_GetCalibrationStatus(ADC1));	 //�ȴ�У׼����
 
//	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//ʹ��ָ����ADC1�����ת����������
}

void gasMS_Init(void){

	gasMS_DIOinit();
	gasMS_AIOinit();
}

//���ADCֵ
//ch:ͨ��ֵ 0~3
uint16_t gasGet_Adc(uint8_t ch)   
{
  	//����ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ239.5����	  			    
  
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//ʹ��ָ����ADC1�����ת����������	
	 
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));//�ȴ�ת������

	return ADC_GetConversionValue(ADC1);	//�������һ��ADC1�������ת�����
}

uint16_t gasGet_Adc_Average(uint8_t ch,uint8_t times)
{
	u32 temp_val=0;
	uint8_t t;
	
	for(t=0;t<times;t++)
	{
		temp_val += gasGet_Adc(ch);
		delay_ms(5);
	}
	return temp_val / times;
} 	

void gasMS_Thread(const void *argument){

	osEvent  evt;
    osStatus status;	
	
	const uint8_t dpSize = 30;
	const uint8_t dpPeriod = 20;
	
	static uint8_t Pcnt = 0;
	char disp[dpSize];
	
	gasMS_MEAS	sensorData;
	static gasMS_MEAS Data_temp = {1};
	
	gasMS_MEAS *mptr = NULL;
	gasMS_MEAS *rptr = NULL;
	
	for(;;){
		
		evt = osMessageGet(MsgBox_MTgasMS, 100);
		if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
			
			rptr = evt.value.p;
			/*�Զ��屾���߳̽������ݴ��������������������������*/
			

			do{status = osPoolFree(gasMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
			rptr = NULL;
		}
		
		sensorData.anaDAT	= (uint8_t)(gasGet_Adc_Average(1,8) / 41);		//���ݲɼ�
		sensorData.VAL		= GAS_DATA;
		
		if(Data_temp.anaDAT != sensorData.anaDAT || 	//�������ͣ����ݸ���ʱ�Ŵ�����
		   Data_temp.VAL != sensorData.VAL){	
		
			Data_temp.anaDAT = sensorData.anaDAT;
			Data_temp.VAL 	 = sensorData.VAL;
			
			do{mptr = (gasMS_MEAS *)osPoolCAlloc(gasMS_pool);}while(mptr == NULL);
			mptr->anaDAT = sensorData.anaDAT;
			mptr->VAL = sensorData.VAL;
			osMessagePut(MsgBox_gasMS, (uint32_t)mptr, 100);
			
			do{mptr = (gasMS_MEAS *)osPoolCAlloc(gasMS_pool);}while(mptr == NULL);
			mptr->anaDAT = sensorData.anaDAT;
			mptr->VAL = sensorData.VAL;
			osMessagePut(MsgBox_DPgasMS, (uint32_t)mptr, 100);
			osDelay(10);
		}
		
		if(Pcnt < dpPeriod){osDelay(10);Pcnt ++;}
		else{
		
			Pcnt = 0;
			memset(disp,0,dpSize * sizeof(char));
			sprintf(disp,"\n\rvalAnalog : %d,valDigital : %d\n\r", sensorData.anaDAT,sensorData.VAL);			
			Driver_USART1.Send(disp,strlen(disp));	
			osDelay(20);
		}
		
		osDelay(10);
	}
}

void gasMSThread_Active(void){

	static bool memInit_flg = false;
	
	if(!memInit_flg){
	
		gasMS_pool   = osPoolCreate(osPool(gasMS_pool));	//�����ڴ��
		MsgBox_gasMS 	= osMessageCreate(osMessageQ(MsgBox_gasMS), NULL);   //������Ϣ����
		MsgBox_MTgasMS = osMessageCreate(osMessageQ(MsgBox_MTgasMS), NULL);//������Ϣ����
		MsgBox_DPgasMS = osMessageCreate(osMessageQ(MsgBox_DPgasMS), NULL);//������Ϣ����
		
		memInit_flg = true;
	}

	gasMS_Init();
	tid_gasMS_Thread = osThreadCreate(osThread(gasMS_Thread),NULL);
}
