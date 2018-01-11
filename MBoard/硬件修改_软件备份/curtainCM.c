#include "curtainCM.h"

static curtainCM_MEAS curtainATTR;

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_curtainCM_Thread;
osThreadDef(curtainCM_Thread,osPriorityNormal,1,512);
			 
osPoolId  curtainCM_pool;								 
osPoolDef(curtainCM_pool, 10, curtainCM_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_curtainCM;
osMessageQDef(MsgBox_curtainCM, 2, &curtainCM_MEAS);            // ��Ϣ���ж��壬����ģ���߳�������ͨѶ�߳�
osMessageQId  MsgBox_MTcurtainCM;
osMessageQDef(MsgBox_MTcurtainCM, 2, &curtainCM_MEAS);          // ��Ϣ���ж���,��������ͨѶ�߳���ģ���߳�
osMessageQId  MsgBox_DPcurtainCM;
osMessageQDef(MsgBox_DPcurtainCM, 2, &curtainCM_MEAS);          // ��Ϣ���ж��壬����ģ���߳�����ʾģ���߳�

void curtainCM_ioInit(void){

	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE );	                

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;	//����
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;		
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init(GPIOC, &GPIO_InitStructure);	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;	//���
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);	
	
	PBout(6) = PBout(7) = 0;
}

void curtainCM_ADCInit(void){

	ADC_InitTypeDef ADC_InitStructure; 
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE );	  //ʹ��ADC1ͨ��ʱ��

	RCC_ADCCLKConfig(RCC_PCLK2_Div6);   //����ADC��Ƶ����6 72M/6=12,ADC���ʱ�䲻�ܳ���14M                     

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
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

u16 curtGet_Adc(u8 ch)   
{
  	//����ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ239.5����	  			    
  
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//ʹ��ָ����ADC1�����ת����������	
	 
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));//�ȴ�ת������

	return ADC_GetConversionValue(ADC1);	//�������һ��ADC1�������ת�����
}

u16 curtGet_Adc_Average(u8 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val += curtGet_Adc(ch);
		delay_ms(5);
	}
	return temp_val/times;
} 

void curtainCM_Init(void){

	curtainCM_ioInit();
	curtainCM_ADCInit();
}

void curtain_logInitCallback(uint32_t event){

	;
}

void curtain_logInit(void){

	/*Initialize the USART driver */
	Driver_USART1.Initialize(curtain_logInitCallback);
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
}

void curtainCM_Thread(const void *argument){
	
	const uint8_t dpSize = 50;
	const uint8_t dpPeriod = 40;
	char  disp[dpSize];
	uint8_t Pcnt;
	
	static bool CurtainUpEN = 0;
	static bool CurtainDnEN = 0;
	static bool CurtainEN = 0;
	static uint16_t Curtain_valElec = 0;
	u8 Kcnt;
//	static curtainCM_MEAS curtainATTR_temp;
	
	for(;;){
	
		curtIOCHG_Kin();
		
		if(!curtKeySW){	//������ʹ�ܡ�����ת
			
			Kcnt = 100;
			while(!curtKeySW && Kcnt){osDelay(20);Kcnt --;}
			CurtainEN = !CurtainEN;
		}if(!CurtainEN){CurtainUpEN = CurtainDnEN = 0; curtMTUP = curtMTDN = 0;}		//���а���ʧ��״ִ̬��	
		
		if(CurtainEN && !curtKeyUP){	//����ʹ�ܡ�����ת
			
			Kcnt = 100;
			while(!curtKeyUP && Kcnt){osDelay(20);Kcnt --;}
			CurtainUpEN = !CurtainUpEN; 
			CurtainDnEN = 0;
		}	
		if(CurtainEN && !curtKeyDN){	//�ش�ʹ�ܡ�����ת
		
			Kcnt = 100;
			while(!curtKeyDN && Kcnt){osDelay(20);Kcnt --;}
			CurtainDnEN = !CurtainDnEN; 
			CurtainUpEN = 0;
		}	
		
		if(CurtainUpEN)curtMTUP = 1;else curtMTUP = 0;	//����״ִ̬��
		if(CurtainDnEN)curtMTDN = 1;else curtMTDN = 0;	//�ش�״ִ̬��
		
		Curtain_valElec = curtGet_Adc_Average(0,5);
		
		curtainATTR.CurtainENs   = CurtainEN;
		curtainATTR.CurtainUpENs = CurtainUpEN;
		curtainATTR.CurtainDnENs = CurtainDnEN;
		curtainATTR.valElec      = Curtain_valElec;
		
		curtIOCHG_DB();
		
		if(Pcnt < dpPeriod){
			
			osDelay(20);
			Pcnt ++;
		}else{
			
			Pcnt = 0;
			memset(disp,0,dpSize * sizeof(char));
			sprintf(disp,"��--------------��\n CurtainEN : %d\n CurtainUpEN : %d\n CurtainDnEN : %d\n valElec : %d\n",curtainATTR.CurtainENs,curtainATTR.CurtainUpENs,curtainATTR.CurtainDnENs,curtainATTR.valElec);
			Driver_USART1.Send(disp,strlen(disp));
			osDelay(20);
		}
	}
}

void curtainCMThread_Active(void){

	static bool memInit_flg = false;
	
	if(!memInit_flg){
	
		curtainCM_pool   = osPoolCreate(osPool(curtainCM_pool));	//�����ڴ��
		MsgBox_curtainCM 	= osMessageCreate(osMessageQ(MsgBox_curtainCM), NULL);   //������Ϣ����
		MsgBox_MTcurtainCM = osMessageCreate(osMessageQ(MsgBox_MTcurtainCM), NULL);//������Ϣ����
		MsgBox_DPcurtainCM = osMessageCreate(osMessageQ(MsgBox_DPcurtainCM), NULL);//������Ϣ����
		
		memInit_flg = true;
	}

	curtainCM_Init();
	tid_curtainCM_Thread = osThreadCreate(osThread(curtainCM_Thread),NULL);
}
