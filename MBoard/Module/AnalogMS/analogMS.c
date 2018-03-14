#include "analogMS.h"

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_analogMS_Thread;
osThreadDef(analogMS_Thread,osPriorityNormal,1,512);
			 
osPoolId  analogMS_pool;								 
osPoolDef(analogMS_pool, 10, analogMS_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_analogMS;
osMessageQDef(MsgBox_analogMS, 2, &analogMS_MEAS);            // ��Ϣ���ж��壬����ģ�����������ͨѶ����
osMessageQId  MsgBox_MTanalogMS;
osMessageQDef(MsgBox_MTanalogMS, 2, &analogMS_MEAS);          // ��Ϣ���ж���,��������ͨѶ������ģ�����
osMessageQId  MsgBox_DPanalogMS;
osMessageQDef(MsgBox_DPanalogMS, 2, &analogMS_MEAS);          // ��Ϣ���ж��壬����ģ���������ʾģ�����

void analogMS_ADCInit(void){

	ADC_InitTypeDef ADC_InitStructure; 
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE );	  //ʹ��ADC1ͨ��ʱ��

	RCC_ADCCLKConfig(RCC_PCLK2_Div6);   //����ADC��Ƶ����6 72M/6=12,ADC���ʱ�䲻�ܳ���14M                     

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_4|GPIO_Pin_5;
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

u16 analogGet_Adc(u8 ch)   
{
  	//����ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ239.5����	  			    
  
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//ʹ��ָ����ADC1�����ת����������	
	 
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));//�ȴ�ת������

	return ADC_GetConversionValue(ADC1);	//�������һ��ADC1�������ת�����
}

u16 analogGet_Adc_Average(u8 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val += analogGet_Adc(ch);
		delay_ms(5);
	}
	return temp_val/times;
} 

void analogMS_Init(void){

	analogMS_ADCInit();
}

void analogMS_Thread(const void *argument){

	osEvent  evt;
    osStatus status;
	
	const bool UPLOAD_MODE = false;	//1�����ݱ仯ʱ���ϴ� 0�����ڶ�ʱ�ϴ�
	
	const uint8_t upldPeriod = 5;	//�����ϴ�����������UPLOAD_MODE = false ʱ��Ч��
	
	uint8_t UPLDcnt = 0;
	bool UPLD_EN = false;
	
	const uint8_t dpSize = 30;
	const uint8_t dpPeriod = 10;
	
	static uint8_t Pcnt = 0;
	char disp[dpSize];
	
	analogMS_MEAS sensorData;
	static analogMS_MEAS Data_temp = {1};
	static analogMS_MEAS Data_tempDP = {1};
	
	analogMS_MEAS *mptr = NULL;
	analogMS_MEAS *rptr = NULL;
	
	for(;;){
		
		evt = osMessageGet(MsgBox_MTanalogMS, 100);
		if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
			
			rptr = evt.value.p;
			/*�Զ��屾�ؽ��̽������ݴ��������������������������*/
			

			do{status = osPoolFree(analogMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
			rptr = NULL;
		}

		sensorData.Ich1 = analogGet_Adc_Average(0,100);
		sensorData.Ich2 = analogGet_Adc_Average(4,100);
		
		if(!UPLOAD_MODE){	//ѡ���ϴ�����ģʽ
		
			if(UPLDcnt < upldPeriod)UPLDcnt ++;
			else{
			
				UPLDcnt = 0;
				UPLD_EN = true;
			}
		}else{
			
			if(Data_temp.Ich1 != sensorData.Ich1 ||
			   Data_temp.Ich2 != sensorData.Ich2){
			
				Data_temp.Ich1 = sensorData.Ich1;
				Data_temp.Ich2 = sensorData.Ich2;
				UPLD_EN = true;
			}
		}
		
		if(UPLD_EN){
			
			UPLD_EN = false;
			   
			do{mptr = (analogMS_MEAS *)osPoolCAlloc(analogMS_pool);}while(mptr == NULL);
			mptr->Ich1 = sensorData.Ich1;
			mptr->Ich2 = sensorData.Ich2;
			osMessagePut(MsgBox_analogMS, (uint32_t)mptr, 100);
			osDelay(500);
		}
		
		if(Data_tempDP.Ich1 != sensorData.Ich1 ||
		   Data_tempDP.Ich2 != sensorData.Ich2){
		
			Data_tempDP.Ich1 = sensorData.Ich1;
			Data_tempDP.Ich2 = sensorData.Ich2;
			
			do{mptr = (analogMS_MEAS *)osPoolCAlloc(analogMS_pool);}while(mptr == NULL);
			mptr->Ich1 = sensorData.Ich1;
			mptr->Ich2 = sensorData.Ich2;
			osMessagePut(MsgBox_DPanalogMS, (uint32_t)mptr, 100);
			osDelay(10);
		}
		
		if(Pcnt < dpPeriod){osDelay(10);Pcnt ++;}
		else{
		
			Pcnt = 0;
			memset(disp,0,dpSize * sizeof(char));
			sprintf(disp,"��--------��\n Ich1:%d\n Ich2:%d\n Vch1:%d\n Vch2:%d\n\n",sensorData.Ich1,sensorData.Ich2,sensorData.Vch1,sensorData.Vch2);
			Driver_USART1.Send(disp,strlen(disp));
			osDelay(20);
		}
		
		osDelay(10);
	}
}

void analogMSThread_Active(void){

	static bool memInit_flg = false;
	
	if(!memInit_flg){
	
		analogMS_pool   = osPoolCreate(osPool(analogMS_pool));	//�����ڴ��
		MsgBox_analogMS 	= osMessageCreate(osMessageQ(MsgBox_analogMS), NULL);   //������Ϣ����
		MsgBox_MTanalogMS = osMessageCreate(osMessageQ(MsgBox_MTanalogMS), NULL);//������Ϣ����
		MsgBox_DPanalogMS = osMessageCreate(osMessageQ(MsgBox_DPanalogMS), NULL);//������Ϣ����
		
		memInit_flg = true;
	}

	analogMS_Init();
	tid_analogMS_Thread = osThreadCreate(osThread(analogMS_Thread),NULL);
}
