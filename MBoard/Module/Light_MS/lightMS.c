#include "lightMS.h"

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_lightMS_Thread;
osThreadDef(lightMS_Thread,osPriorityNormal,1,512);
			 
osPoolId  lightMS_pool;								 
osPoolDef(lightMS_pool, 10, lightMS_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_lightMS;
osMessageQDef(MsgBox_lightMS, 2, &lightMS_MEAS);            // ��Ϣ���ж��壬����ģ���߳�������ͨѶ�߳�
osMessageQId  MsgBox_MTlightMS;
osMessageQDef(MsgBox_MTlightMS, 2, &lightMS_MEAS);          // ��Ϣ���ж���,��������ͨѶ�߳���ģ���߳�
osMessageQId  MsgBox_DPlightMS;
osMessageQDef(MsgBox_DPlightMS, 2, &lightMS_MEAS);          // ��Ϣ���ж��壬����ģ���߳�����ʾģ���߳�

cdsADC_Init(void){

//	ADC_InitTypeDef ADC_InitStructure; 
//	GPIO_InitTypeDef GPIO_InitStructure;

//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE );	  //ʹ��ADC1ͨ��ʱ��

//	RCC_ADCCLKConfig(RCC_PCLK2_Div6);   //����ADC��Ƶ����6 72M/6=12,ADC���ʱ�䲻�ܳ���14M                     

//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		//ģ����������
//	GPIO_Init(GPIOA, &GPIO_InitStructure);	

//	ADC_DeInit(ADC1);  //��λADC1,������ ADC1 ��ȫ���Ĵ�������Ϊȱʡֵ

//	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	//ADC����ģʽ:ADC1��ADC2�����ڶ���ģʽ
//	ADC_InitStructure.ADC_ScanConvMode = DISABLE;	//ģ��ת�������ڵ�ͨ��ģʽ
//	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;	//ģ��ת�������ڵ���ת��ģʽ
//	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//ת��������������ⲿ��������
//	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	//ADC�����Ҷ���
//	ADC_InitStructure.ADC_NbrOfChannel = 1;	//˳����й���ת����ADCͨ������Ŀ
//	ADC_Init(ADC1, &ADC_InitStructure);	//����ADC_InitStruct��ָ���Ĳ�����ʼ������ADCx�ļĴ���   


//	ADC_Cmd(ADC1, ENABLE);	//ʹ��ָ����ADC1

//	ADC_ResetCalibration(ADC1);	//ʹ�ܸ�λУ׼  
//	 
//	while(ADC_GetResetCalibrationStatus(ADC1));	//�ȴ���λУ׼����

//	ADC_StartCalibration(ADC1);	 //����ADУ׼

//	while(ADC_GetCalibrationStatus(ADC1));	 //�ȴ�У׼����

////	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//ʹ��ָ����ADC1�����ת����������
}

u16 cdsGet_Adc(u8 ch)   
{
  	//����ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ239.5����	  			    
  
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//ʹ��ָ����ADC1�����ת����������	
	 
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));//�ȴ�ת������

	return ADC_GetConversionValue(ADC1);	//�������һ��ADC1�������ת�����
}

u16 cdsGet_Adc_Average(u8 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val += cdsGet_Adc(ch);
		delay_ms(5);
	}
	return temp_val/times;
} 

//IO��ʼ��
void tsl2561IO_Init(void)
{
 
 GPIO_InitTypeDef  GPIO_InitStructure;
 	
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //ʹ��PB,PE�˿�ʱ��
	
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7;	 
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //�������
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
 GPIO_Init(GPIOB, &GPIO_InitStructure);					 //�����趨������ʼ��GPIOB.5

 GPIO_SetBits(GPIOB,GPIO_Pin_6|GPIO_Pin_7); 	
}

void TSL2561_Init(void)
{
	tsl2561IO_Init();
	TSLSDA_OUT();
	TSLIIC_SCL=1;
	TSLIIC_SDA=1;
	TSL2561_Write(CONTROL,0x03);
	delay_ms(100);
	TSL2561_Write(TIMING,0x02);
}

void tsl2561_start(void)
{
	TSLSDA_OUT();     //sda
	TSLIIC_SDA=1;	  	  
	TSLIIC_SCL=1;
	delay_us(4);
 	TSLIIC_SDA=0;//START:when CLK is high,DATA change form high to low 
	delay_us(4);
	TSLIIC_SCL=0;//
}

void stop(void)
{
	TSLSDA_OUT();//sda
	TSLIIC_SCL=0;
	TSLIIC_SDA=0;//STOP:when CLK is high DATA change form low to high
 	delay_us(4);
	TSLIIC_SCL=1; 
	TSLIIC_SDA=1;//
	delay_us(4);							   	
}

void respons(void)
{
	TSLIIC_SCL=0;
	TSLSDA_OUT();
	TSLIIC_SDA=0;
	delay_us(2);
	TSLIIC_SCL=1;
	delay_us(2);
	TSLIIC_SCL=0;
}

void write_byte(uint8 value)
{
    uint8_t t;   
	TSLSDA_OUT(); 	    
    TSLIIC_SCL=0;
    for(t=0;t<8;t++)
    {              
        //TSLIIC_SDA=(txd&0x80)>>7;
		if((value&0x80)>>7)
			TSLIIC_SDA=1;
		else
			TSLIIC_SDA=0;
		value<<=1; 	  
		delay_us(2);   
		TSLIIC_SCL=1;
		delay_us(2); 
		TSLIIC_SCL=0;	
		delay_us(2);
    }	 
}

uint8 read_byte(void)
{
	unsigned char i,receive=0;
	TSLSDA_IN();//SDA
    for(i=0;i<8;i++ )
	{
        TSLIIC_SCL=0; 
        delay_us(2);
		TSLIIC_SCL=1;
        receive<<=1;
        if(TSLREAD_SDA)receive++;   
		delay_us(1); 
    }					 

	TSLSDA_OUT();
	TSLIIC_SDA=1;//release DATA-line
	return receive;
}


void TSL2561_Write(uint8 command,uint8 data)
{
	tsl2561_start();
	write_byte(SLAVE_ADDR_WR);
	respons();
	write_byte(command);
	respons();
	write_byte(data);
	respons();
	stop();
}


uint8 TSL2561_Read(uint8 command)
{
	uint8 data;
	tsl2561_start();
	write_byte(SLAVE_ADDR_WR);
	respons();
	write_byte(command);
	respons();
	
	tsl2561_start();
	write_byte(SLAVE_ADDR_RD);
	respons();
	data=read_byte();
	stop();
	return data;
}

uint32 Read_Light(void)
{
//	uint16 Channel0,Channel1;
//	uint8 Data0_L,Data0_H,Data1_L,Data1_H;
//	
//	Data0_L=TSL2561_Read(DATA0LOW);
//	Data0_H=TSL2561_Read(DATA0HIGH);
//	Channel0=(256*Data0_H + Data0_L);
//	
//	Data1_L=TSL2561_Read(DATA1LOW);
//	Data1_H=TSL2561_Read(DATA1HIGH);
//	Channel1=(256*Data1_H + Data1_L);
//	
//	return calculateLux(Channel0,Channel1);
	
	return (uint32)cdsGet_Adc_Average(0,5);
}

uint32_t calculateLux(uint16_t ch0, uint16_t ch1)
{
		uint32_t chScale;
		uint32_t channel1;
		uint32_t channel0;
		uint32_t temp;
		uint32_t ratio1 = 0;
		uint32_t ratio	;
		uint32_t lux_temp;
		uint16_t b, m;
		chScale=(1 <<TSL2561_LUX_CHSCALE);           //����ʱ��Ĵ���Ϊ0x02��
		chScale = chScale << 4;                      //��������Ϊ1�ģ�����Ϊ16����д��һ��
		// scale the channel values
		channel0 = (ch0 * chScale) >> TSL2561_LUX_CHSCALE;
		channel1 = (ch1 * chScale) >> TSL2561_LUX_CHSCALE;
		// find the ratio of the channel values (Channel1/Channel0)
		if (channel0 != 0)
		ratio1 = (channel1 << (TSL2561_LUX_RATIOSCALE+1)) / channel0;
		ratio = (ratio1 + 1) >> 1;	  									 // round the ratio value
		if ((ratio > 0) && (ratio <= TSL2561_LUX_K1T))
			{
				b=TSL2561_LUX_B1T;
				m=TSL2561_LUX_M1T;
			}
		else if (ratio <= TSL2561_LUX_K2T)
			{
				b=TSL2561_LUX_B2T;
				m=TSL2561_LUX_M2T;
			}
		else if (ratio <= TSL2561_LUX_K3T)
			{
				b=TSL2561_LUX_B3T;
				m=TSL2561_LUX_M3T;
			}
		else if (ratio <= TSL2561_LUX_K4T)
			{
				b=TSL2561_LUX_B4T;
				m=TSL2561_LUX_M4T;
			}
		else if (ratio <= TSL2561_LUX_K5T)
			{
				b=TSL2561_LUX_B5T;
				m=TSL2561_LUX_M5T;
			}
		else if (ratio <= TSL2561_LUX_K6T)
			{
				b=TSL2561_LUX_B6T;
				m=TSL2561_LUX_M6T;
			}
		else if (ratio <= TSL2561_LUX_K7T)
			{
				b=TSL2561_LUX_B7T;
				m=TSL2561_LUX_M7T;
			}
		else if (ratio > TSL2561_LUX_K8T)
			{
				b=TSL2561_LUX_B8T;
				m=TSL2561_LUX_M8T;
			}
		temp = ((channel0 * b) - (channel1 * m));
		if (temp < 1)  temp = 0;							// do not allow negative lux value
		temp += (1 << (TSL2561_LUX_LUXSCALE-1));			// round lsb (2^(LUX_SCALE-1))
		lux_temp = temp >> TSL2561_LUX_LUXSCALE;			// strip off fractional portion
		return lux_temp;		  							// Signal I2C had no errors
}

void lightMS_Init(void){

//	TSL2561_Init();
	cdsADC_Init();
}

void lightMS_Thread(const void *argument){

	osEvent  evt;
    osStatus status;	
	
	const uint8_t dpSize = 30;
	const uint8_t dpPeriod = 40;
	
	static uint8_t Pcnt = 0;
	char disp[dpSize];
	
	lightMS_MEAS sensorData;
	static lightMS_MEAS Data_temp = {1};
	
	lightMS_MEAS *mptr = NULL;
	lightMS_MEAS *rptr = NULL;
	
	for(;;){
		
	/***********************�����߳����ݽ���***************************************************/
	//�������������ݽ����ϴ����������ݹ��ܱ�������ʱ����
		evt = osMessageGet(MsgBox_MTlightMS, 100);
		if (evt.status == osEventMessage) {		//�ȴ���Ϣָ��
			
			rptr = evt.value.p;
			/*�Զ��屾���߳̽������ݴ��������������������������*/
			

			do{status = osPoolFree(lightMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
			rptr = NULL;
		}

		sensorData.illumination = Read_Light();
		
		if(Data_temp.illumination != sensorData.illumination){	//�������ͣ����ݸ���ʱ�Ŵ�����
		
			Data_temp.illumination = sensorData.illumination;
			
			do{mptr = (lightMS_MEAS *)osPoolCAlloc(lightMS_pool);}while(mptr == NULL);	//�������ݴ�����Ϣ����
			mptr->illumination = sensorData.illumination;
			osMessagePut(MsgBox_lightMS, (uint32_t)mptr, 100);
			
			do{mptr = (lightMS_MEAS *)osPoolCAlloc(lightMS_pool);}while(mptr == NULL);	//1.44��Һ����ʾ��Ϣ����
			mptr->illumination = sensorData.illumination;
			osMessagePut(MsgBox_DPlightMS, (uint32_t)mptr, 100);
			osDelay(10);
		}
		
		if(Pcnt < dpPeriod){osDelay(10);Pcnt ++;}
		else{
		
			Pcnt = 0;
			memset(disp,0,dpSize * sizeof(char));
			sprintf(disp,"��ǰ����ǿ��Ϊ��%d Lux \r\n",sensorData.illumination);
			Driver_USART1.Send(disp,strlen(disp));
			osDelay(20);
		}	
		osDelay(10);
	}
}

void lightMSThread_Active(void){
	
	static bool memInit_flg = false;
	
	if(!memInit_flg){
	
		lightMS_pool   = osPoolCreate(osPool(lightMS_pool));	//�����ڴ��
		MsgBox_lightMS 	= osMessageCreate(osMessageQ(MsgBox_lightMS), NULL);   //������Ϣ����
		MsgBox_MTlightMS = osMessageCreate(osMessageQ(MsgBox_MTlightMS), NULL);//������Ϣ����
		MsgBox_DPlightMS = osMessageCreate(osMessageQ(MsgBox_DPlightMS), NULL);//������Ϣ����
		
		memInit_flg = true;
	}

	lightMS_Init();
	tid_lightMS_Thread = osThreadCreate(osThread(lightMS_Thread),NULL);
}
