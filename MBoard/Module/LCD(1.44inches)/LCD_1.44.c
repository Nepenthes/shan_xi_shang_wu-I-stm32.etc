#include "LCD_1.44.h"
  
//����LCD��Ҫ����
//Ĭ��Ϊ����
_lcd144_dev lcd144dev;

osThreadId tid_LCD144Test_Thread;

osThreadDef(LCD144Disp_Thread,osPriorityNormal,1,2048);

//������ɫ,������ɫ
uint16_t LCD144POINT_COLOR = 0x0000,LCD144BACK_COLOR = 0xFFFF;  
uint16_t DeviceCode;	 
/****************************************************************************
* ��    �ƣ�void  SPIv_WriteData(uint8_t Data)
* ��    �ܣ�STM32_ģ��SPIдһ���ֽ����ݵײ㺯��
* ��ڲ�����Data
* ���ڲ�������
* ˵    ����STM32_ģ��SPI��дһ���ֽ����ݵײ㺯��
****************************************************************************/
void  SPIv_WriteData(uint8_t Data)
{
	unsigned char i=0;
	for(i=8;i>0;i--)
	{
		if(Data&0x80)	
	  LCD_1_44_SDA_SET; //�������
      else LCD_1_44_SDA_CLR;
	   
      LCD_1_44_SCL_CLR;       
      LCD_1_44_SCL_SET;
      Data<<=1; 
	}
}

/****************************************************************************
* ��    �ƣ�uint8_t SPI_WriteByte(SPI_TypeDef* SPIx,uint8_t Byte)
* ��    �ܣ�STM32_Ӳ��SPI��дһ���ֽ����ݵײ㺯��
* ��ڲ�����SPIx,Byte
* ���ڲ��������������յ�������
* ˵    ����STM32_Ӳ��SPI��дһ���ֽ����ݵײ㺯��
****************************************************************************/
uint8_t SPI_WriteByte(SPI_TypeDef* SPIx,uint8_t Byte)
{
	while((SPIx->SR&SPI_I2S_FLAG_TXE)==RESET);		//�ȴ���������	  
	SPIx->DR=Byte;	 	//����һ��byte   
	while((SPIx->SR&SPI_I2S_FLAG_RXNE)==RESET);//�ȴ�������һ��byte  
	return SPIx->DR;          	     //�����յ�������			
} 

/****************************************************************************
* ��    �ƣ�void SPI_SetSpeed(SPI_TypeDef* SPIx,uint8_t SpeedSet)
* ��    �ܣ�����SPI���ٶ�
* ��ڲ�����SPIx,SpeedSet
* ���ڲ�������
* ˵    ����SpeedSet:1,����;0,����;
****************************************************************************/
void SPI_SetSpeed(SPI_TypeDef* SPIx,uint8_t SpeedSet)
{
	SPIx->CR1&=0XFFC7;
	if(SpeedSet==1)//����
	{
		SPIx->CR1|=SPI_BaudRatePrescaler_2;//Fsck=Fpclk/2	
	}
	else//����
	{
		SPIx->CR1|=SPI_BaudRatePrescaler_32; //Fsck=Fpclk/32
	}
	SPIx->CR1|=1<<6; //SPI�豸ʹ��
} 

/****************************************************************************
* ��    �ƣ�SPI3_Init(void)
* ��    �ܣ�STM32_SPI3Ӳ�����ó�ʼ��
* ��ڲ�������
* ���ڲ�������
* ˵    ����STM32_SPI3Ӳ�����ó�ʼ��
****************************************************************************/
void SPI3_Init(void)	
{
	SPI_InitTypeDef  SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	 
	//����SPI3�ܽ�
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO|RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4| GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
	
	//SPI3����ѡ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3,ENABLE);
	   
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI3, &SPI_InitStructure);
	
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE) ;

	//ʹ��SPI3
	SPI_Cmd(SPI3, ENABLE);  
}

//******************************************************************
//��������  LCD_1_44_WR_REG
//���ߣ�    xiao��@ȫ������
//���ڣ�    2013-02-22
//���ܣ�    ��Һ��������д��д16λָ��
//���������Reg:��д���ָ��ֵ
//����ֵ��  ��
//�޸ļ�¼����
//******************************************************************
void LCD_1_44_WR_REG(uint16_t data)
{ 
   LCD_1_44_CS_CLR;
   LCD_1_44_RS_CLR;
#if USE_HARDWARE_SPI   
   SPI_WriteByte(SPI3,data);
#else
   SPIv_WriteData(data);
#endif 
   LCD_1_44_CS_SET;
}

//******************************************************************
//��������  LCD_1_44_WR_DATA
//���ߣ�    xiao��@ȫ������
//���ڣ�    2013-02-22
//���ܣ�    ��Һ��������д��д8λ����
//���������Data:��д�������
//����ֵ��  ��
//�޸ļ�¼����
//******************************************************************
void LCD_1_44_WR_DATA(uint8_t data)
{
	
   LCD_1_44_CS_CLR;
   LCD_1_44_RS_SET;
#if USE_HARDWARE_SPI   
   SPI_WriteByte(SPI3,data);
#else
   SPIv_WriteData(data);
#endif 
   LCD_1_44_CS_SET;

}
//******************************************************************
//��������  LCD_1_44_DrawPoint_16Bit
//���ߣ�    xiao��@ȫ������
//���ڣ�    2013-02-22
//���ܣ�    8λ���������д��һ��16λ����
//���������(x,y):�������
//����ֵ��  ��
//�޸ļ�¼����
//******************************************************************
void LCD_1_44_WR_DATA_16Bit(uint16_t data)
{	
   LCD_1_44_CS_CLR;
   LCD_1_44_RS_SET;
#if USE_HARDWARE_SPI   
   SPI_WriteByte(SPI3,data>>8);
   SPI_WriteByte(SPI3,data);
#else
   SPIv_WriteData(data>>8);
   SPIv_WriteData(data);
#endif 
   LCD_1_44_CS_SET;
}

//******************************************************************
//��������  LCD_1_44_WriteReg
//���ߣ�    xiao��@ȫ������
//���ڣ�    2013-02-22
//���ܣ�    д�Ĵ�������
//���������LCD_1_44_Reg:�Ĵ�����ַ
//			LCD_1_44_RegValue:Ҫд�������
//����ֵ��  ��
//�޸ļ�¼����
//******************************************************************
void LCD_1_44_WriteReg(uint16_t LCD_1_44_Reg, uint16_t LCD_1_44_RegValue)
{	
	LCD_1_44_WR_REG(LCD_1_44_Reg);  
	LCD_1_44_WR_DATA(LCD_1_44_RegValue);	    		 
}	   
	 
//******************************************************************
//��������  LCD_1_44_WriteRAM_Prepare
//���ߣ�    xiao��@ȫ������
//���ڣ�    2013-02-22
//���ܣ�    ��ʼдGRAM
//			�ڸ�Һ��������RGB����ǰ��Ӧ�÷���дGRAMָ��
//�����������
//����ֵ��  ��
//�޸ļ�¼����
//******************************************************************
void LCD_1_44_WriteRAM_Prepare(void)
{
	LCD_1_44_WR_REG(lcd144dev.wramcmd);
}	 

//******************************************************************
//��������  LCD_1_44_DrawPoint
//���ߣ�    xiao��@ȫ������
//���ڣ�    2013-02-22
//���ܣ�    ��ָ��λ��д��һ�����ص�����
//���������(x,y):�������
//����ֵ��  ��
//�޸ļ�¼����
//******************************************************************
void LCD_1_44_DrawPoint(uint16_t x,uint16_t y)
{
	LCD_1_44_SetCursor(x,y);//���ù��λ�� 
	LCD_1_44_WR_DATA_16Bit(LCD144POINT_COLOR);
}

//******************************************************************
//��������  LCD_1_44_GPIOInit
//���ߣ�    xiao��@ȫ������
//���ڣ�    2013-02-22
//���ܣ�    Һ����IO��ʼ����Һ����ʼ��ǰҪ���ô˺���
//�����������
//����ֵ��  ��
//�޸ļ�¼����
//******************************************************************
void LCD_1_44_GPIOInit(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	      
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4| GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	  	
}

//******************************************************************
//��������  LCD_1_44_Reset
//���ߣ�    xiao��@ȫ������
//���ڣ�    2013-02-22
//���ܣ�    LCD��λ������Һ����ʼ��ǰҪ���ô˺���
//�����������
//����ֵ��  ��
//�޸ļ�¼����
//******************************************************************
void LCD_1_44_RESET(void)
{
	LCD_1_44_RST_CLR;
	osDelay(100);	
	LCD_1_44_RST_SET;
	osDelay(50);
}
 	 
//******************************************************************
//��������  LCD_1_44_Init
//���ߣ�    xiao��@ȫ������
//���ڣ�    2013-02-22
//���ܣ�    LCD��ʼ��
//�����������
//����ֵ��  ��
//�޸ļ�¼����
//******************************************************************
void LCD144_Init(void)
{  
#if USE_HARDWARE_SPI //ʹ��Ӳ��SPI
	SPI3_Init();
#else	
	LCD_1_44_GPIOInit();//ʹ��ģ��SPI
#endif  										 

 	LCD_1_44_RESET(); //Һ������λ

	//************* Start Initial Sequence **********//		
  LCD_1_44_WR_REG(0x11); //Exit Sleep
	osDelay(20);
	LCD_1_44_WR_REG(0x26); //Set Default Gamma
	LCD_1_44_WR_DATA(0x04);
	LCD_1_44_WR_REG(0xB1);//Set Frame Rate
	LCD_1_44_WR_DATA(0x0e);
	LCD_1_44_WR_DATA(0x10);
	LCD_1_44_WR_REG(0xC0); //Set VRH1[4:0] & VC[2:0] for VCI1 & GVDD
	LCD_1_44_WR_DATA(0x08);
	LCD_1_44_WR_DATA(0x00);
	LCD_1_44_WR_REG(0xC1); //Set BT[2:0] for AVDD & VCL & VGH & VGL
	LCD_1_44_WR_DATA(0x05);
	LCD_1_44_WR_REG(0xC5); //Set VMH[6:0] & VML[6:0] for VOMH & VCOML
	LCD_1_44_WR_DATA(0x38);
	LCD_1_44_WR_DATA(0x40);
	
	LCD_1_44_WR_REG(0x3a); //Set Color Format
	LCD_1_44_WR_DATA(0x05);
	LCD_1_44_WR_REG(0x36); //RGB
	LCD_1_44_WR_DATA(0x1C);   //1C//C8
	
	LCD_1_44_WR_REG(0x2A); //Set Column Address
	LCD_1_44_WR_DATA(0x00);
	LCD_1_44_WR_DATA(0x00);
	LCD_1_44_WR_DATA(0x00);
	LCD_1_44_WR_DATA(0x7F);
	LCD_1_44_WR_REG(0x2B); //Set Page Address
	LCD_1_44_WR_DATA(0x00);
	LCD_1_44_WR_DATA(32);
	LCD_1_44_WR_DATA(0x00);
	LCD_1_44_WR_DATA(127+32);
	
	LCD_1_44_WR_REG(0xB4); 
	LCD_1_44_WR_DATA(0x00);
	
	LCD_1_44_WR_REG(0xf2); //Enable Gamma bit
	LCD_1_44_WR_DATA(0x01);
	LCD_1_44_WR_REG(0xE0); 
	LCD_1_44_WR_DATA(0x3f);//p1
	LCD_1_44_WR_DATA(0x22);//p2
	LCD_1_44_WR_DATA(0x20);//p3
	LCD_1_44_WR_DATA(0x30);//p4
	LCD_1_44_WR_DATA(0x29);//p5
	LCD_1_44_WR_DATA(0x0c);//p6
	LCD_1_44_WR_DATA(0x4e);//p7
	LCD_1_44_WR_DATA(0xb7);//p8
	LCD_1_44_WR_DATA(0x3c);//p9
	LCD_1_44_WR_DATA(0x19);//p10
	LCD_1_44_WR_DATA(0x22);//p11
	LCD_1_44_WR_DATA(0x1e);//p12
	LCD_1_44_WR_DATA(0x02);//p13
	LCD_1_44_WR_DATA(0x01);//p14
	LCD_1_44_WR_DATA(0x00);//p15
	LCD_1_44_WR_REG(0xE1); 
	LCD_1_44_WR_DATA(0x00);//p1
	LCD_1_44_WR_DATA(0x1b);//p2
	LCD_1_44_WR_DATA(0x1f);//p3
	LCD_1_44_WR_DATA(0x0f);//p4
	LCD_1_44_WR_DATA(0x16);//p5
	LCD_1_44_WR_DATA(0x13);//p6
	LCD_1_44_WR_DATA(0x31);//p7
	LCD_1_44_WR_DATA(0x84);//p8
	LCD_1_44_WR_DATA(0x43);//p9
	LCD_1_44_WR_DATA(0x06);//p10
	LCD_1_44_WR_DATA(0x1d);//p11
	LCD_1_44_WR_DATA(0x21);//p12
	LCD_1_44_WR_DATA(0x3d);//p13
	LCD_1_44_WR_DATA(0x3e);//p14
	LCD_1_44_WR_DATA(0x3f);//p15
	
	LCD_1_44_WR_REG(0x29); // Display On
	LCD_1_44_WR_REG(0x2C);

	LCD_1_44_SetParam();//����LCD����	 
	///LCD_1_44_LED_SET;//��������	 
	//LCD_1_44_Clear(WHITE);
}
//******************************************************************
//��������  LCD_1_44_Clear
//���ߣ�    xiao��@ȫ������
//���ڣ�    2013-02-22
//���ܣ�    LCDȫ�������������
//���������Color:Ҫ���������ɫ
//����ֵ��  ��
//�޸ļ�¼����
//******************************************************************
void LCD_1_44_Clear(uint16_t Color)
{
	uint16_t i,j;      
	LCD_1_44_SetWindows(0,0,lcd144dev.width-1,lcd144dev.height-1);	  
	for(i=0;i<lcd144dev.width;i++)
	{
		for(j=0;j<lcd144dev.height;j++)
		LCD_1_44_WR_DATA_16Bit(Color);	//д������ 	 
	}
} 

void LCD_1_44_ClearS(uint16_t Color,uint16_t x,uint16_t y,uint16_t xx,uint16_t yy)
{
	uint16_t i,j;      
	LCD_1_44_SetWindows(x,y,xx-1,yy-1);	  
	for(i=x;i<xx;i++)
	{
		for(j=y;j<yy;j++)
		LCD_1_44_WR_DATA_16Bit(Color);	//д������ 	 
	}
}   	
/*************************************************
��������LCD_1_44_SetWindows
���ܣ�����lcd��ʾ���ڣ��ڴ�����д�������Զ�����
��ڲ�����xy�����յ�
����ֵ����
*************************************************/
void LCD_1_44_SetWindows(uint16_t xStar, uint16_t yStar,uint16_t xEnd,uint16_t yEnd)
{
#if USE_HORIZONTAL==1	//ʹ�ú���

	LCD_1_44_WR_REG(lcd144dev.setxcmd);	
	LCD_1_44_WR_DATA(xStar>>8);
	LCD_1_44_WR_DATA(0x00FF&xStar);		
	LCD_1_44_WR_DATA(xEnd>>8);
	LCD_1_44_WR_DATA(0x00FF&xEnd);

	LCD_1_44_WR_REG(lcd144dev.setycmd);	
	LCD_1_44_WR_DATA(yStar>>8);
	LCD_1_44_WR_DATA(0x00FF&yStar);		
	LCD_1_44_WR_DATA(yEnd>>8);
	LCD_1_44_WR_DATA(0x00FF&yEnd);		
#else
	
	LCD_1_44_WR_REG(lcd144dev.setxcmd);	
	LCD_1_44_WR_DATA(xStar>>8);
	LCD_1_44_WR_DATA(0x00FF&xStar);		
	LCD_1_44_WR_DATA(xEnd>>8);
	LCD_1_44_WR_DATA(0x00FF&xEnd);

	LCD_1_44_WR_REG(lcd144dev.setycmd);	
	LCD_1_44_WR_DATA(yStar>>8);
	LCD_1_44_WR_DATA(0x00FF&yStar+0);		
	LCD_1_44_WR_DATA(yEnd>>8);
	LCD_1_44_WR_DATA(0x00FF&yEnd+0);	
#endif

	LCD_1_44_WriteRAM_Prepare();	//��ʼд��GRAM				
}   

/*************************************************
��������LCD_1_44_SetCursor
���ܣ����ù��λ��
��ڲ�����xy����
����ֵ����
*************************************************/
void LCD_1_44_SetCursor(uint16_t Xpos, uint16_t Ypos)
{	  	    			
	LCD_1_44_SetWindows(Xpos,Ypos,Xpos,Ypos);
} 

//����LCD����
//������к�����ģʽ�л�
void LCD_1_44_SetParam(void)
{ 	
	lcd144dev.wramcmd=0x2C;
#if USE_HORIZONTAL==1	//ʹ�ú���	  
	lcd144dev.dir=1;//����
	lcd144dev.width=128+3;
	lcd144dev.height=128+2;
	lcd144dev.setxcmd=0x2A;
	lcd144dev.setycmd=0x2B;			
	LCD_1_44_WriteReg(0x36,0xA8);

#else//����
	lcd144dev.dir=0;//����				 	 		
	lcd144dev.width=128+2;
	lcd144dev.height=128+3;
	lcd144dev.setxcmd=0x2A;
	lcd144dev.setycmd=0x2B;	
	LCD_1_44_WriteReg(0x36,0xC8);
	//LCD_1_44_WriteReg(0x36,0x1C);//����ֵʹ��0x1C��LCD_1_44_SetWindows�����С�+32��ƫ����Ӧȡ0
#endif
}	

void LCD144Test_Thread(const void *argument){

	while(1)
	{
		LCD_1_44_Clear(BLACK); //����

		LCD144POINT_COLOR=GRAY; 

		Show_Str(32,5,BLUE,WHITE,"ϵͳ���",16,0);

		Show_Str(5,25,RED,YELLOW,"�¶�     ��",24,1);
		LCD_1_44_ShowNum2412(5+48,25,RED,YELLOW,":24",24,1);

		Show_Str(5,50,YELLOW,YELLOW,"ʪ��     ��",24,1);
		LCD_1_44_ShowNum2412(5+48,50,YELLOW,YELLOW,":32",24,1);

		Show_Str(5,75,WHITE,YELLOW,"��ѹ      ��",24,1);
		LCD_1_44_ShowNum2412(5+48,75,WHITE,YELLOW,":3.2",24,1);
			
		Show_Str(5,100,GREEN,YELLOW,"����      ��",24,1);
		LCD_1_44_ShowNum2412(5+48,100,GREEN,YELLOW,":0.2",24,1);
		
		delay_ms(1500);
	}
}

void LCD144Disp_Thread(const void *argument){

	static char EXT_ID,MSG_ID,ZW_ADDR;
	u8 EXTID_Disp[5],MSGID_Disp[5],WZADDR_Disp[10];
	osStatus status;
	
	LCD_1_44_Clear(BLACK); //����
	
	Show_Str(70,3,LGRAYBLUE,BLACK,"MSG_type:",12,1);
	Show_Str(123,4,YELLOW,BLACK,"x",12,1);
	
	Show_Str(3,3,LGRAYBLUE,BLACK,"EXT_ID:",12,1);
	Show_Str(44,3,YELLOW,BLACK,"xxxx",12,1);
	
	Show_Str(3,13,LGRAYBLUE,BLACK,"W/Z_ADDR:",12,1);
	Show_Str(56,14,YELLOW,BLACK,"unknown",12,1);
	
	Show_Str(5,50,YELLOW,BLACK,"δ��⵽",24,1);
	Show_Str(25,75,YELLOW,BLACK,"��չģ��",24,1);
	
	for(;;){
		
		if(MSG_ID != Moudle_GTA.Wirless_ID){
		
			MSG_ID = Moudle_GTA.Wirless_ID;
			LCD_1_44_ClearS(BLACK,122,0,130,13);
			
			switch(Moudle_GTA.Wirless_ID){
			
				case MID_TRANS_Zigbee:	
					
						sprintf((char *)MSGID_Disp,"Z");
						Show_Str(123,4,BRED,BLACK,MSGID_Disp,12,1);
						break;
						
				case MID_TRANS_Wifi:	
					
						sprintf((char *)MSGID_Disp,"W");
						Show_Str(123,4,BRED,BLACK,MSGID_Disp,12,1);
						break;
				
				default:sprintf((char *)MSGID_Disp,"-");
						Show_Str(123,4,LGRAYBLUE,BLACK,MSGID_Disp,12,1);
						break;
			}
		}
		
		if(EXT_ID != Moudle_GTA.Extension_ID){		//��⵽ģ����ģ�����������
		
			EXT_ID = Moudle_GTA.Extension_ID;
			if(!EXT_ID){
			
				sprintf((char *)EXTID_Disp,"NONE");
				LCD_1_44_ClearS(BLACK,43,0,70,13);
				Show_Str(44,3,BRRED,BLACK,EXTID_Disp,12,1);
			}else{
				
				sprintf((char *)EXTID_Disp,"0x%02X",Moudle_GTA.Extension_ID);
				LCD_1_44_ClearS(BLACK,43,0,70,13);
				Show_Str(44,3,BRRED,BLACK,EXTID_Disp,12,1);
			}
		
			LCD_1_44_ClearS(BLACK,5,25,127,127);
			
			osDelay(500);
			
			switch(Moudle_GTA.Extension_ID){		

				case MID_SENSOR_FIRE:	

						Show_Str(5,50,WHITE,BLACK,"�������n",24,1);
						Show_Str(50,75,WHITE,BLACK,"X X",24,1);
						break;
				
				case MID_SENSOR_PYRO:		
					
						Show_Str(5,50,WHITE,BLACK,"��������n",24,1);
						Show_Str(50,75,WHITE,BLACK,"X X",24,1);
						break;
				
				case MID_SENSOR_SMOKE:

						Show_Str(5,50,WHITE,BLACK,"�������n",24,1);
						Show_Str(50,75,WHITE,BLACK,"X X",24,1);
						break;
						
				case MID_SENSOR_GAS:	
							
						Show_Str(5,50,WHITE,BLACK,"ȼ������n",24,1);
						Show_Str(50,75,WHITE,BLACK,"X X",24,1);
						break;	
				
				case MID_SENSOR_TEMP:	

						Show_Str(5,25,WHITE,BLACK,"�¶��n",24,1);
						Show_Str(50,50,WHITE,BLACK,"X X",24,1);
						Show_Str(5,75,WHITE,BLACK,"ʪ���n",24,1);
						Show_Str(50,100,WHITE,BLACK,"X X",24,1);	
						break;
				
				case MID_SENSOR_LIGHT:	
					
						Show_Str(5,50,WHITE,BLACK,"���ȼ���n",24,1);
						Show_Str(50,75,WHITE,BLACK,"X X",24,1);
						break;	
				
				case MID_SENSOR_ANALOG:
				
						Show_Str(5,25,WHITE,BLACK,"anaIch1:",24,1);
						Show_Str(5,45,GREEN,BLACK,"anaIch2:",24,1);
						Show_Str(5,65,WHITE,BLACK,"anaVch1:",24,1);
						Show_Str(5,85,GREEN,BLACK,"anaVch2:",24,1);
						break;
				
				case MID_EGUARD:
					
						Show_Str(5,25,WHITE,BLACK,"figID is:",24,1);
						Show_Str(5,55,WHITE,BLACK,"rfID is:",24,1);
						Show_Str(5,85,WHITE,BLACK,"Password is:",24,1);
						break;
				
				case MID_EXEC_DEVIFR:		
					
						Show_Str(5,25,WHITE,BLACK,"��ֵ�n",24,1);
						Show_Str(50,50,GREEN,BLACK,"X X",24,1);
						Show_Str(5,75,WHITE,BLACK,"����״̬�n",24,1);
						Show_Str(50,100,GREEN,BLACK,"X X",24,1);
						break;
				
				case MID_EXEC_DEVPWM:
					
						Show_Str(5,25,WHITE,BLACK,"����״̬�n",24,1);
						Show_Str(50,50,GREEN,BLACK,"X X",24,1);
						Show_Str(5,75,WHITE,BLACK,"�����n",24,1);
						Show_Str(50,100,GREEN,BLACK,"X X",24,1);
						break;
				
				case MID_EXEC_CURTAIN:
					
						Show_Str(5,50,WHITE,BLACK,"����״̬�n",24,1);
						Show_Str(50,75,WHITE,BLACK,"X X",24,1);
						break;	
				
				case MID_EXEC_SOURCE:
					
						Show_Str(5,25,WHITE,BLACK,"��Դ״̬�n",24,1);
						Show_Str(50,50,GREEN,BLACK,"X X",24,1);
						Show_Str(5,75,WHITE,BLACK,"ģ��ֵ�n",24,1);
						Show_Str(50,100,GREEN,BLACK,"X X",24,1);	
						break;
				
				case MID_EXEC_SPEAK:
					
						Show_Str(5,50,WHITE,BLACK,"����״̬�n",24,1);
						Show_Str(50,75,WHITE,BLACK,"X X",24,1);
						break;	

				default:
						
						Show_Str(5,50,YELLOW,BLACK,"δ��⵽",24,1);
						Show_Str(25,75,YELLOW,BLACK,"��չģ��",24,1);
						break;
			}
		}
		
		switch(Moudle_GTA.Extension_ID){		

			case MID_SENSOR_FIRE:{
				
						fireMS_MEAS *rptr;
						osEvent  evt;
						char disp[30];
				
						evt = osMessageGet(MsgBox_DPfireMS, 10);
						if (evt.status == osEventMessage){
							
							rptr = evt.value.p;
							/*��ʾ���ֳ����������������*/
							
							LCD_1_44_ClearS(BLACK,0,75,127,127);
							if(rptr->VAL){
							
								sprintf(disp,"�л�");
								Show_Str(40,85,GREEN,BLACK,(uint8_t *)disp,24,1);
							}else{
							
								sprintf(disp,"û��");
								Show_Str(40,85,GREEN,BLACK,(uint8_t *)disp,24,1);
							}

							do{status = osPoolFree(fireMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_SENSOR_PYRO:{
					
						pyroMS_MEAS *rptr;
						osEvent  evt;
						char disp[30];
				
						evt = osMessageGet(MsgBox_DPpyroMS, 10);
						if (evt.status == osEventMessage){
							
							rptr = evt.value.p;
							/*��ʾ���ֳ����������������*/
							
							LCD_1_44_ClearS(BLACK,0,75,127,127);
							if(rptr->VAL){
							
								sprintf(disp,"����");
								Show_Str(40,85,GREEN,BLACK,(uint8_t *)disp,24,1);
							}else{
							
								sprintf(disp,"����");
								Show_Str(40,85,GREEN,BLACK,(uint8_t *)disp,24,1);
							}
							
							do{status = osPoolFree(pyroMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_SENSOR_SMOKE:{
					
						smokeMS_MEAS *rptr;
						osEvent  evt;
						char disp[30];
				
						evt = osMessageGet(MsgBox_DPsmokeMS, 10);
						if (evt.status == osEventMessage){
							
							rptr = evt.value.p;
							/*��ʾ���ֳ����������������*/
							
							LCD_1_44_ClearS(BLACK,0,75,127,127);
							if(rptr->VAL){
							
								sprintf(disp,"����");
								Show_Str(40,85,GREEN,BLACK,(uint8_t *)disp,24,1);
							}else{
							
								sprintf(disp,"����");
								Show_Str(40,85,GREEN,BLACK,(uint8_t *)disp,24,1);
							}
							
							sprintf(disp,"anaVAL:%d",rptr->anaDAT);
							Show_Str(10,105,BLUE,BLACK,(uint8_t *)disp,24,1);
							
							do{status = osPoolFree(smokeMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
					
			case MID_SENSOR_GAS:{
					
						gasMS_MEAS *rptr;
						osEvent  evt;
						char disp[30];
				
						evt = osMessageGet(MsgBox_DPgasMS, 10);
						if (evt.status == osEventMessage){
							
							rptr = evt.value.p;
							/*��ʾ���ֳ����������������*/
							
							LCD_1_44_ClearS(BLACK,0,75,127,127);
							if(rptr->VAL){
							
								sprintf(disp,"����");
								Show_Str(40,85,GREEN,BLACK,(uint8_t *)disp,24,1);
							}else{
							
								sprintf(disp,"����");
								Show_Str(40,85,GREEN,BLACK,(uint8_t *)disp,24,1);
							}
							
							sprintf(disp,"anaVAL:%d",rptr->anaDAT);
							Show_Str(10,105,BLUE,BLACK,(uint8_t *)disp,24,1);
							
							do{status = osPoolFree(gasMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_SENSOR_TEMP:{
					
						tempMS_MEAS *rptr;
						osEvent  evt;
						char disp[30];
				
						evt = osMessageGet(MsgBox_DPtempMS, 10);
						if (evt.status == osEventMessage){
							
							rptr = evt.value.p;
							/*��ʾ���ֳ����������������*/
							
							LCD_1_44_ClearS(BLACK,0,50,127,75);
							LCD_1_44_ClearS(BLACK,0,100,127,125);
							
							sprintf(disp,"%.2f",rptr->temp);
							LCD_1_44_ShowNum2412(20,50,BLUE,BLACK,(uint8_t *)disp,24,1);
							Show_Str(20 + strlen(disp) * 24,50,GREEN,BLACK,"��",24,1);
							sprintf(disp,"%.2f",rptr->hum);
							LCD_1_44_ShowNum2412(20,100,BLUE,BLACK,(uint8_t *)disp,24,1);
							Show_Str(20 + strlen(disp) * 24,100,GREEN,BLACK,"��",24,1);
							
							do{status = osPoolFree(tempMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_SENSOR_LIGHT:{
				
						lightMS_MEAS *rptr;
						osEvent  evt;
						char disp[30];
						
						evt = osMessageGet(MsgBox_DPlightMS, 10);
						if (evt.status == osEventMessage){
							
							rptr = evt.value.p;
							/*��ʾ���ֳ����������������*/
							
							LCD_1_44_ClearS(BLACK,0,75,127,127);
							sprintf(disp,"%d Lux",rptr->illumination);
							Show_Str(20 + 8*(8 - strlen(disp)),85,GREEN,BLACK,(uint8_t *)disp,24,1);
							
							do{status = osPoolFree(lightMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_SENSOR_ANALOG:{
					
						analogMS_MEAS *rptr;
						osEvent  evt;
						char disp[30];
				
						evt = osMessageGet(MsgBox_DPanalogMS, 10);
						if (evt.status == osEventMessage){
							
							rptr = evt.value.p;
							/*��ʾ���ֳ����������������*/
							
							LCD_1_44_ClearS(BLACK,0,25,127,45);
							sprintf(disp,"anaIch1:%d",rptr->Ich1);
							Show_Str(50,25,GREEN,BLACK,(uint8_t *)disp,24,1);
							
							LCD_1_44_ClearS(BLACK,0,25,127,45);
							sprintf(disp,"anaIch2:%d",rptr->Ich2);
							Show_Str(50,45,GREEN,BLACK,(uint8_t *)disp,24,1);
							
							LCD_1_44_ClearS(BLACK,0,45,127,65);
							sprintf(disp,"anaVch1:%d",rptr->Vch1);
							Show_Str(50,65,GREEN,BLACK,(uint8_t *)disp,24,1);
							
							LCD_1_44_ClearS(BLACK,0,65,127,85);
							sprintf(disp,"anaVch2:%d",rptr->Vch2);
							Show_Str(50,85,GREEN,BLACK,(uint8_t *)disp,24,1);
							
							do{status = osPoolFree(analogMS_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_EGUARD:{
					
						EGUARD_MEAS *rptr;
						osEvent  evt;
						char disp[30];
				
						static uint8_t DP_cnt;
						static bool DP_FLG = true;
				
						if(DP_FLG){
						
							if(DP_cnt)DP_cnt --;
							else{
							
								DP_FLG = false;
								LCD_1_44_ClearS(BLACK,0,40,127,55);
								LCD_1_44_ClearS(BLACK,0,70,127,85);
								LCD_1_44_ClearS(BLACK,0,100,127,115);
								
								Show_Str(40,40,GREEN,BLACK,"NULL",24,1);
								Show_Str(40,70,GREEN,BLACK,"NULL",24,1);
								Show_Str(40,100,GREEN,BLACK,"NULL",24,1);
							}
						}
				
						evt = osMessageGet(MsgBox_DPEGUD, 10);
						if (evt.status == osEventMessage){
							
							rptr = evt.value.p;
							/*��ʾ���ֳ����������������*/
							
							DP_FLG = true;
							DP_cnt = 100;
							
							switch(rptr->CMD){
								
								case FID_EXERES_TTIT:
									
										LCD_1_44_ClearS(BLACK,0,40,127,85);
										memset(disp,0,30 * sizeof(char));
										sprintf(disp,"%d",rptr->DAT);			
										Show_Str(25,40,GREEN,BLACK,(uint8_t *)disp,24,1);
										break;
							
								case RFID_EXERES_TTIT:
										
										LCD_1_44_ClearS(BLACK,0,70,127,85);
										memset(disp,0,30 * sizeof(char));
										sprintf(disp,"%02X%02X%02X%02X",rptr->rfidDAT[0],rptr->rfidDAT[1],rptr->rfidDAT[2],rptr->rfidDAT[3]);			
										Show_Str(25,70,GREEN,BLACK,(uint8_t *)disp,24,1);
										break;
								
								case PSD_EXERES_TTIT:
									
										LCD_1_44_ClearS(BLACK,0,70,127,85);
										
										Show_Str(25,70,GREEN,BLACK,(uint8_t *)disp,24,1);
										break;
								
								default:break;
							}
							
							do{status = osPoolFree(EGUD_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_EXEC_DEVIFR:{
					
						IFR_MEAS *rptr;
						osEvent  evt;
						char disp[30];
				
						evt = osMessageGet(MsgBox_DPIFR, 10);
						if (evt.status == osEventMessage){
							
							rptr = evt.value.p;
							/*��ʾ���ֳ����������������*/
							
							switch(rptr->speDPCMD){
							
								case SPECMD_pwmDevModADDR_CHG:{
								
									;
								}break;
								
								case SPECMD_pwmDevDATS_CHG:{
								
									LCD_1_44_ClearS(BLACK,0,50,127,75);
									sprintf(disp,"%d",rptr->VAL_KEY);
									LCD_1_44_ShowNum2412(50,50,GREEN,BLACK,(uint8_t *)disp,24,1);
									
									LCD_1_44_ClearS(BLACK,0,100,127,125);
									switch(rptr->STATUS){
									
										case kifrSTATUS_NONLRN:sprintf(disp,"NULL");break;
											
										case kifrSTATUS_WAITK:sprintf(disp,"�����ȴ�");break;
											
										case kifrSTATUS_WAITSG:sprintf(disp,"�ȴ�ң��");break;
											
										case kifrSTATUS_LRNOVR:sprintf(disp,"ѧϰ���");break;
											
										case kifrSTATUS_SGOUT:sprintf(disp,"�ź����");break;
										
										default:sprintf(disp,"NULL");break;
									}
									Show_Str(10,100,GREEN,BLACK,(uint8_t *)disp,24,1);									
								}break;
								
								default:break;
							}
							
							do{status = osPoolFree(IFR_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_EXEC_DEVPWM:{
			
						pwmCM_MEAS *rptr;
						osEvent  evt;
						char disp[30];
				
						evt = osMessageGet(MsgBox_DPpwmCM, 10);
						if (evt.status == osEventMessage){
							
							rptr = evt.value.p;
							/*��ʾ���ֳ����������������*/
							
							switch(rptr->speDPCMD){
							
								case SPECMD_ifrDevModADDR_CHG:{
								
									;
								}break;
								
								case SPECMD_ifrDevDATS_CHG:{
								
									LCD_1_44_ClearS(BLACK,0,50,127,75);
									switch(rptr->Switch){
									
										case true:	sprintf(disp,"����"); break;
												
										case false:	sprintf(disp,"�ر�"); break;
									}
									Show_Str(25,50,GREEN,BLACK,(uint8_t *)disp,24,1);
									
									LCD_1_44_ClearS(BLACK,0,100,127,125);
									sprintf(disp,"%d",rptr->pwmVAL);
									Show_Str(25,100,GREEN,BLACK,(uint8_t *)disp,24,1);
								}break;
							}
							
							do{status = osPoolFree(pwmCM_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_EXEC_CURTAIN:{
					
						curtainCM_MEAS *rptr;
						osEvent  evt;
						char disp[30];
				
						evt = osMessageGet(MsgBox_DPcurtainCM, 10);
						if (evt.status == osEventMessage){
							
							rptr = evt.value.p;
							/*��ʾ���ֳ����������������*/
							
							LCD_1_44_ClearS(BLACK,0,75,127,100);
							switch(rptr->valACT){
							
								case CMD_CURTUP:	sprintf(disp,"����");break;
								
								case CMD_CURTSTP:	sprintf(disp,"�ر�");break;
								
								case CMD_CURTDN:	sprintf(disp,"ֹͣ");break;
							}
							Show_Str(25,75,GREEN,BLACK,(uint8_t *)disp,24,1);
							
							do{status = osPoolFree(curtainCM_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_EXEC_SOURCE:{
					
						sourceCM_MEAS *rptr;
						osEvent  evt;
						char disp[30];
				
						evt = osMessageGet(MsgBox_DPsourceCM, 10);
						if (evt.status == osEventMessage){
							
							rptr = evt.value.p;
							/*��ʾ���ֳ����������������*/
							
							LCD_1_44_ClearS(BLACK,0,75,127,100);
							switch(rptr->Switch){
							
								case true:	sprintf(disp,"����"); break;
										
								case false:	sprintf(disp,"�ر�"); break;
							}
							Show_Str(25,50,GREEN,BLACK,(uint8_t *)disp,24,1);
							
							LCD_1_44_ClearS(BLACK,0,100,127,125);
							sprintf(disp,"%d",rptr->anaVal);
							Show_Str(25,100,GREEN,BLACK,(uint8_t *)disp,24,1);
							
							do{status = osPoolFree(sourceCM_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;
			
			case MID_EXEC_SPEAK:{
			
						speakCM_MEAS *rptr;
						osEvent  evt;
						char disp[30];
				
						evt = osMessageGet(MsgBox_DPspeakCM, 10);
						if (evt.status == osEventMessage){
							
							rptr = evt.value.p;
							/*��ʾ���ֳ����������������*/
							
							LCD_1_44_ClearS(BLACK,0,75,127,100);
							switch(rptr->spk_num){
							
								case 1:	sprintf(disp,"������");break;
									
								case 2:	sprintf(disp,"ȼ������");break;
									
								case 3:	sprintf(disp,"���汨��");break;
									
								case 4:	sprintf(disp,"��������");break;
									
								case 5:	sprintf(disp,"������");break;
								
								case 6:	sprintf(disp,"������");break;
								
								case 7:	sprintf(disp,"�ƹ�");break;
								
								case 8:	sprintf(disp,"�ƹ��");break;
								
								case 9:	sprintf(disp,"�ƹ����");break;
							
								case 10:sprintf(disp,"�ƹ����");break;
								
								case 11:sprintf(disp,"�ƹ�����");break;
								
								case 12:sprintf(disp,"�ƹ��");break;
								
								case 13:sprintf(disp,"���ӿ�");break;
								
								case 14:sprintf(disp,"���ӹ�");break;
								
								default:sprintf(disp,"һ������");break;
							}
							
							Show_Str(10,75,GREEN,BLACK,(uint8_t *)disp,24,1);
							
							do{status = osPoolFree(speakCM_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
							rptr = NULL;
						}
					}break;

			default:break;
		}
		osDelay(20);
	}
}

void LCD144Disp_Active(void){
	
	LCD144_Init();
	tid_LCD144Test_Thread = osThreadCreate(osThread(LCD144Disp_Thread),NULL);
}

