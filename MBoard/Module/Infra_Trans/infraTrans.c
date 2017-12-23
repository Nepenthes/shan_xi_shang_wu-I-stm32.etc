#include "infraTrans.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

osThreadId tid_keyIFR_Thread;
osThreadDef(keyIFR_Thread,osPriorityAboveNormal,1,4096);

osPoolId  IFR_pool;								 
osPoolDef(IFR_pool, 10, IFR_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_IFR;
osMessageQDef(MsgBox_IFR, 2, &IFR_MEAS);            // ��Ϣ���ж��壬����ģ���߳�������ͨѶ�߳�
osMessageQId  MsgBox_MTIFR;
osMessageQDef(MsgBox_MTIFR, 2, &IFR_MEAS);          // ��Ϣ���ж���,��������ͨѶ�߳���ģ���߳�
osMessageQId  MsgBox_DPIFR;
osMessageQDef(MsgBox_DPIFR, 2, &IFR_MEAS);          // ��Ϣ���ж��壬����ģ���߳�����ʾģ���߳�

const uint8_t Tab_size = 255;	//�źŲ�����
const uint8_t IFR_PER  = 2;		//��ƽ�����ֱ��� 2us
bool measure_en = true;
uint8_t tabHp,tabLp;
volatile uint16_t HTtab[Tab_size];
volatile uint16_t LTtab[Tab_size];

typedef void (* funkeyThread)(funKeyInit key_Init,Obj_keyStatus *orgKeyStatus,funKeyScan key_Scan,Obj_eventKey keyEvent,const char *Tips_head);

extern ARM_DRIVER_USART Driver_USART1;								//�豸�����⴮��һ�豸����

void keyIFR_ADCInit(void)
{
    ADC_InitTypeDef ADC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1	, ENABLE );	  //ʹ��ADC1ͨ��ʱ��

    RCC_ADCCLKConfig(RCC_PCLK2_Div6);   //����ADC��Ƶ����6 72M/6=12,ADC���ʱ�䲻�ܳ���14M

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;		//ģ����������
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

void Remote_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,ENABLE);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource14);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB,GPIO_Pin_14);
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	PBout(15) = 0;

	EXTI_ClearITPendingBit(EXTI_Line14);
	
    EXTI_InitStructure.EXTI_Line = EXTI_Line14;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

	NVIC_PriorityGroupConfig(2);
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //��ռ���ȼ�0��
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //�����ȼ�3��
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
    NVIC_Init(&NVIC_InitStructure);
}

u16 HW_ReceiveTime(void)
{
    u16 t=0;
	const uint16_t MAX = 12000 / IFR_PER;

    while(PBin(14))
    {
        t++;
        delay_us(IFR_PER);
		
        if(t >= MAX) return t;
    }

    return t;
}

u16 LW_ReceiveTime(void)
{
    u16 t=0;
	const uint16_t MAX = 12000 / IFR_PER;

    while(!PBin(14))
    {
        t++;
        delay_us(IFR_PER);
		
        if(t >= MAX) return t;
    }

    return t;
}

void EXTI15_10_IRQHandler(void) {

	const uint16_t SIGLEN_MAX = 10000 / IFR_PER;  //һ����ƽ�ź�ά���ʱ������
	const uint16_t SIGLEN_MIN = 4000  / IFR_PER;  //һ����ƽ�ź�ά�����ʱ������
	
	static uint8_t Tab_Hp,Tab_Lp;
	static uint16_t HT_Tab[Tab_size] = {0};
	static uint16_t LT_Tab[Tab_size] = {0};
	uint16_t time = 0;
	
	Tab_Hp = Tab_Lp = 0;

    if(measure_en && EXTI_GetITStatus(EXTI_Line14) == SET)
    {
		
		Tab_Hp = Tab_Lp = 0;
		memset(HT_Tab, 0, Tab_size * sizeof(uint16_t));
		memset(LT_Tab, 0, Tab_size * sizeof(uint16_t));
		while(1){
		
			if(!PBin(14)){
			
				time = LW_ReceiveTime();
				
				if(time > SIGLEN_MIN && Tab_Lp > 155){  //���治�������ڽز�����
				
					//Driver_USART1.Send(&Tab_Lp,1);   //�����ڸߵ�ƽ�ź��ܳ��������
					memcpy((uint16_t *)LTtab,LT_Tab,Tab_Lp * 2);	//��������Ϊ u16,��memcpy���ֽ�Ϊ��λ������2
					memcpy((uint16_t *)HTtab,HT_Tab,Tab_Hp * 2);
					tabLp = Tab_Lp;
					tabHp = Tab_Hp;
					measure_en = false;
					break;
				}else{
					
					LT_Tab[Tab_Lp ++] = time;
				}
			}
			if(PBin(14)){
			
				time = HW_ReceiveTime();
					
				if((time > SIGLEN_MAX) || (time > SIGLEN_MIN && Tab_Hp > 155)){
					
					//Driver_USART1.Send(&Tab_Hp,1);	//�����ڵ͵�ƽ�ź��ܳ��������
					memcpy((uint16_t *)LTtab,LT_Tab,Tab_Lp * 2);	//��������Ϊ u16,��memcpy���ֽ�Ϊ��λ������2
					memcpy((uint16_t *)HTtab,HT_Tab,Tab_Hp * 2);
					tabLp = Tab_Lp;
					tabHp = Tab_Hp;
					measure_en = false;
					break;
				}else{

					HT_Tab[Tab_Hp ++] = time;
				}
			}
		}
    }
	
	EXTI_ClearITPendingBit(EXTI_Line14);
	EXTI_ClearFlag(EXTI_Line14);
}

void IFR_Send(uint16_t HTab[],uint8_t Hp,uint16_t LTab[],uint8_t Lp){

	const uint16_t MAX = 6000 / IFR_PER; //��ʱ�䱣���źŲ�����׼
	uint8_t loop;
	uint16_t temp;
	
//	char disp[20];							//��Ӧ����λ�źų����������
//	sprintf(disp,"%d",LTab[69]);
//	Driver_USART1.Send(disp,strlen(disp));
	
	PBout(15) = 0;
	for(loop = 0;loop < Lp;loop ++){
	
		if(LTab[loop] < MAX){
		
			temp = (LTab[loop] + LTab[loop] / 14) / 12;	//�ز����� 14Ϊʱ�䲹����12(����ֵ) = 13-1(13 Ϊ 26 / IFR_PER) �� 26Ϊ38k�ز�������
		}	
		else{
		
			temp = (LTab[loop] + LTab[loop] / 14) / 12 + 60UL; //��ʱ�䱣���źŲ���
		}
		while(temp --){			//38k�ز������ڵ���
		
			PBout(15) = 1;		
			delay_us(11);
			PBout(15) = 0;
			delay_us(12);			
		}
		
		PBout(15) = 0;		    //�͵�ƽ�źŻ���
		temp = HTab[loop] + HTab[loop] / 15;
		while(temp --)delay_us(IFR_PER);
	}
	
	PBout(15) = 0;
}

//���ADCֵ
//ch:ͨ��ֵ 0~3
uint16_t keyIFRGet_Adc(uint8_t ch)
{
    //����ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5);	//ADC1,ADCͨ��,����ʱ��Ϊ239.5����

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//ʹ��ָ����ADC1�����ת����������

    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));//�ȴ�ת������

    return ADC_GetConversionValue(ADC1);	//�������һ��ADC1�������ת�����
}

uint16_t keyIFRGet_Adc_Average(uint8_t ch,uint8_t times)
{
    u32 temp_val = 0;
    uint8_t t;

    for(t=0; t<times; t++)
    {
        temp_val += keyIFRGet_Adc(ch);
        delay_ms(5);
    }
    return temp_val / times;
}

/***�������״̬��***/
static uint16_t getKey2(Obj_keyStatus *orgKeyStatus,funKeyScan keyScan) {

    static	uint16_t s_u16KeyState 		= KEY_STATE_INIT;		//״̬�����״̬����ʼ��״̬
    static	uint16_t	s_u16LastKey		= KEY_NULL;			//������ʷ������ֵ
    static	uint8_t	KeyKeepComfirm		= 0;					//������ȷ�ϱ��� ȷ������ʱ����ʱ
    static	uint16_t	s_u16KeyTimeCount	= 0;						//����ʱ�����壨������KEY_TICK���м����������������ֵ��ȷ���Ƿ����ڳ�����
    uint16_t keyTemp				= KEY_NULL;					/*ʮ�����Ƶ�һλ������״̬��	�ڶ�λ�����ּ���ֵ��		����λ����ֵ��		����λ����������ֵ*/

    static	uint32_t osTick_last			= 0xffff0000;		//��osTick���м�¼����������һ��osTick���жԱȻ�ȡ������˼�������жϰ����Ƿ������������£�

    keyTemp = keyScan();		//��ȡ��ֵ

    switch(s_u16KeyState) {		//��ȡ״̬��״̬

    case KEY_STATE_INIT:		//��ʼ��״̬

        if(orgKeyStatus->keyCTflg) {	//�����һ���Ƿ�Ϊ����

            if((osKernelSysTick() - osTick_last) > KEY_CONTINUE_PERIOD) {	//��һ�����������Ȿ���Ƿ��������

                keyTemp	= s_u16LastKey & 0x00f0;	//���β���������������ֵΪ��������״̬��ͬʱ������־λ����
                keyTemp |= KEY_CTOVER;
                orgKeyStatus->keyCTflg = 0;
            }
        }

        if(KEY_NULL != keyTemp)s_u16KeyState = KEY_STATE_WOBBLE;	//��⵽�а������л�״̬���������
        break;

    case KEY_STATE_WOBBLE:	//����״̬���

        s_u16KeyState = KEY_STATE_PRESS;	//ȷ�ϰ������л�״̬���̰����
        break;

    case KEY_STATE_PRESS:	//�̰�״̬���

        if(KEY_NULL != keyTemp) {		//�����Ƿ���

            s_u16LastKey 	= keyTemp;	//�洢������ֵ
            keyTemp 		  |= KEY_DOWN;	//����״̬ȷ��Ϊ����
            s_u16KeyState	= KEY_STATE_LONG;	//������Ȼδ�����л�״̬���������
        } else {

            s_u16KeyState	= KEY_STATE_INIT;	//���Ϊ�������������״̬�����س�ʼ��״̬
        }
        break;

    case KEY_STATE_LONG:		//����״̬���

        if(KEY_NULL != keyTemp) {	//�����Ƿ���

            if(++s_u16KeyTimeCount > KEY_LONG_PERIOD) {	//��������Ȼδ��������ݳ���ʱ�����м�������ȷ���Ƿ�Ϊ����

                s_u16KeyTimeCount	= 0;			//ȷ�ϳ�������������ֵ����
                orgKeyStatus->sKeyKeep				= 0;			//���״̬���л�״̬ǰ���Ա���ȷ��״̬�������ֵ��ǰ���㣬������ֵΪ�����󱣳ּ�������
                KeyKeepComfirm		= 0;			//���״̬���л�״̬ǰ���Ա���ȷ��״̬�������ֵ��ǰ���㣬����ֵ���ڶ��峤�����ý��б��ּ���
                keyTemp			  |= KEY_LONG;	//����״̬ȷ��Ϊ����
                s_u16KeyState		= KEY_STATE_KEEP;	//������Ȼδ�����л�״̬���������ּ��

                orgKeyStatus->keyOverFlg			= KEY_OVER_LONG;	//����
            } else	orgKeyStatus->keyOverFlg	= KEY_OVER_SHORT;		//�̰�
        } else {

            s_u16KeyState	= KEY_STATE_RELEASE;	//���ȷ��Ϊ�����󰴼������л�״̬��������
        }
        break;

    case KEY_STATE_KEEP:		//�����󱣳�״̬���

        if(KEY_NULL != keyTemp) {		//�����Ƿ���

            if(++s_u16KeyTimeCount > KEY_KEEP_PERIOD) {	//��������Ȼδ��������ݳ���ʱ�����м�������ȷ���Ƿ�Ϊ�������������

                s_u16KeyTimeCount	= 0;			//ȷ�ϳ�����������֣���������ֵ����
                if(KeyKeepComfirm < (KEY_COMFIRM + 3))KeyKeepComfirm++;			//����Ƿ��������ʱ��
                if(orgKeyStatus->sKeyKeep < 15 && KeyKeepComfirm > KEY_COMFIRM)orgKeyStatus->sKeyKeep++; 	//��⵽���������ִ�г����󱣳ּ���
                if(orgKeyStatus->sKeyKeep) {		//��⵽�����󱣳ּ���ֵ��Ϊ�㣬��ȷ�ϰ���״̬Ϊ������������֣��Է���ֵ������Ӧȷ�ϴ���

                    orgKeyStatus->keyOverFlg	 = KEY_OVER_KEEP;	//״̬ȷ��Ϊ������Ϊ����
                    keyTemp	|= orgKeyStatus->sKeyKeep << 8;		//���ּ�����������8λ����ʮ������keyTemp�ڶ�λ
                    keyTemp	|= KEY_KEEP;			//����״̬ȷ��Ϊ�������������
                }
            }
        } else {

            s_u16KeyState	= KEY_STATE_RELEASE;	//����״̬ȷ��Ϊ�������������֮�����л�״̬��������
        }
        break;

    case KEY_STATE_RELEASE:	//����״̬���

        s_u16LastKey |= KEY_UP;	//�洢����״̬
        keyTemp		  = s_u16LastKey;	//����״̬ȷ��Ϊ����
        s_u16KeyState = KEY_STATE_RECORD;	//�л�״̬������������¼
        break;

    case KEY_STATE_RECORD:	//����������¼״̬���

        if((osKernelSysTick() - osTick_last) < KEY_CONTINUE_PERIOD) {	//�����ΰ�������ʱ����С�ڹ涨ֵ�����ж�Ϊ����

            orgKeyStatus->sKeyCount++;	//��������
        } else {

            orgKeyStatus->sKeyCount = 0;	//�����Ͽ�����������
        }

        if(orgKeyStatus->sKeyCount) {		//������������Ϊ�㣬��ȷ��Ϊ�����������Է���ֵ������Ӧ����

            orgKeyStatus->keyCTflg	= 1;	//��������־
            keyTemp	= s_u16LastKey & 0x00f0;	//��ȡ��ֵ
            keyTemp	|=	KEY_CONTINUE;				//ȷ��Ϊ��������
            if(orgKeyStatus->sKeyCount < 15)keyTemp += orgKeyStatus->sKeyCount;	//�����������ݷ���ʮ������keyTemp����λ�����λ��
        }

        s_u16KeyState	= KEY_STATE_INIT;		//���״̬�����س�ʼ״̬
        osTick_last	 	= osKernelSysTick();	//��¼osTick�������´��������Ա�
        break;

    default:
        break;
    }
    return keyTemp;	//���ذ���״̬�ͼ�ֵ
}

/*������ʼ������������״̬����ṹ�壬����ɨ�躯�������������¼�������������ʾ��Ϣͷ*/
void key_Thread2(funKeyInit key_Init,
                 Obj_keyStatus *orgKeyStatus,
                 funKeyScan key_Scan,Obj_eventKey keyEvent,
                 const char *Tips_head) {

    /***�������ԣ�����1����������Ϣ��****/
    static uint16_t keyVal;						//����״̬�¼�
    static uint8_t	key_temp;					//������ֵ����
    static uint8_t	kCount;						//��������ֵ�������������ּ�������������ʹ��ͬһ����������Ϊ����״̬����ͬʱ����
    static uint8_t	kCount_rec;			//��ʷ����ֵ����

    static osThreadId ID_Temp;					//��ǰ�߳�ID����
    static osEvent evt;
    static uint8_t KEY_DEBUG_FLG = 0;

    const	 uint8_t	tipsLen = 100;		//Tips��ӡ�ַ�������
    static char	key_tempDisp;
    static char	kCountDisp;
    static char	kCount_recDisp;
    static char	tips[tipsLen];					//Tips�ַ���

    key_Init();

    for(;;) {

        keyVal = getKey2(orgKeyStatus,key_Scan);    //��ȡ��ֵ

        ID_Temp = osThreadGetId();
        evt = osSignalWait (KEY_DEBUG_OFF, 1);		 //��ȡDebug_log���Ȩ���ź�
        if (evt.value.signals == KEY_DEBUG_OFF) {

            KEY_DEBUG_FLG = 0;
            osSignalClear(ID_Temp,KEY_DEBUG_OFF);
        } else {

            evt = osSignalWait (KEY_DEBUG_ON, 1);
            if (evt.value.signals == KEY_DEBUG_ON) {

                KEY_DEBUG_FLG = 1;
                osSignalClear(ID_Temp,KEY_DEBUG_ON);
            }
        }

        if(KEY_DEBUG_FLG) {

            memset(tips,0,tipsLen*sizeof(char));	//ÿ��Tips��ӡ�����
            strcat(tips,"Tips:");						//Tips��ʶ
            strcat(tips,Tips_head);
            strcat(tips,"-");
        }
        /*------------------------------------------------------------------------------------------------------------------------------*/
        switch(keyVal & 0xf000) {

        case KEY_LONG		:

            key_temp = (uint8_t)((keyVal & 0x00f0) >> 4);
            if(KEY_DEBUG_FLG) { /*Debug_log���ʹ��*/
                strcat(tips,"����");
                key_tempDisp = key_temp + '0';
                strcat(tips,(const char*)&key_tempDisp);
                strcat(tips,"����\r\n");
                Driver_USART1.Send(tips,strlen(tips));
            }/***/
            break;

        case KEY_KEEP		:

            kCount		= (uint8_t)((keyVal & 0x0f00) >> 8);  //��ȡ����ֵ
            kCount_rec	= kCount;
            if(KEY_DEBUG_FLG) { /*Debug_log���ʹ��*/
                strcat(tips,"����");
                key_tempDisp = key_temp + '0';
                strcat(tips,(const char*)&key_tempDisp);
                strcat(tips,"�������֣����ּ�����");
                kCountDisp = kCount + '0';
                strcat(tips,(const char*)&kCountDisp);
                strcat(tips,"\r\n");
                Driver_USART1.Send(tips,strlen(tips));
            }/***/
            break;

        case KEY_DOWN		:

            key_temp = (uint8_t)((keyVal & 0x00f0) >> 4);
            if(KEY_DEBUG_FLG) { /*Debug_log���ʹ��*/
                strcat(tips,"����");
                key_tempDisp = key_temp + '0';
                strcat(tips,(const char*)&key_tempDisp);
                strcat(tips,"����\r\n");
                Driver_USART1.Send(tips,strlen(tips));
            }/***/
            break;

        case KEY_UP			:
            if(KEY_DEBUG_FLG) { /*Debug_log���ʹ��*/
                strcat(tips,"����");
                key_tempDisp = key_temp + '0';
                strcat(tips,(const char*)&key_tempDisp);
            }/***/
            switch(orgKeyStatus->keyOverFlg) {

            case KEY_OVER_SHORT		:

                if(keyEvent.funKeySHORT[key_temp])keyEvent.funKeySHORT[key_temp]();		//�����¼��������ȼ�ⴥ���¼��Ƿ񴴽���û�����򲻽��д���
                if(KEY_DEBUG_FLG) { /*Debug_log���ʹ��*/
                    strcat(tips,"�̰�����\r\n");
                    Driver_USART1.Send(tips,strlen(tips));
                    orgKeyStatus->keyOverFlg = 0;
                }/***/
                break;

            case KEY_OVER_LONG		:

                if(keyEvent.funKeyLONG[key_temp])keyEvent.funKeyLONG[key_temp]();
                if(KEY_DEBUG_FLG) { /*Debug_log���ʹ��*/
                    strcat(tips,"��������\r\n");
                    Driver_USART1.Send(tips,strlen(tips));
                    orgKeyStatus->keyOverFlg = 0;
                }/***/
                break;

            case KEY_OVER_KEEP		:

                if(keyEvent.funKeyKEEP[key_temp][kCount_rec])keyEvent.funKeyKEEP[key_temp][kCount_rec]();
                if(KEY_DEBUG_FLG) { /*Debug_log���ʹ��*/
                    strcat(tips,"�����󱣳�");
                    kCount_recDisp = kCount_rec + '0';
                    strcat(tips,(const char*)&kCount_recDisp);
                    strcat(tips,"�μ��������\r\n");
                    Driver_USART1.Send(tips,strlen(tips));
                    kCount_rec = 0;
                }/***/
                break;
            default:
                break;
            }
            break;

        case KEY_CONTINUE	:

            kCount 		= (uint8_t)((keyVal & 0x000f) >> 0);	//��ȡ����ֵ
            kCount_rec	= kCount + 1;
            if(KEY_DEBUG_FLG) { /*Debug_log���ʹ��*/
                strcat(tips,"����");
                key_tempDisp = key_temp + '0';
                strcat(tips,(const char*)&key_tempDisp);
                strcat(tips,"����������������");
                kCountDisp = kCount + '0';
                strcat(tips,(const char*)&kCountDisp);
                strcat(tips,"\r\n");
                Driver_USART1.Send(tips,strlen(tips));
            }/***/
            break;

        case KEY_CTOVER	:

            if(keyEvent.funKeyCONTINUE[key_temp][kCount_rec])keyEvent.funKeyCONTINUE[key_temp][kCount_rec]();
            if(KEY_DEBUG_FLG) { /*Debug_log���ʹ��*/
                strcat(tips,"����");
                key_tempDisp = key_temp + '0';
                strcat(tips,(const char*)&key_tempDisp);
                strcat(tips,"����");
                kCount_recDisp = kCount_rec + '0';
                strcat(tips,(const char*)&kCount_recDisp);
                strcat(tips,"�κ����\r\n");
                Driver_USART1.Send(tips,strlen(tips));
            }/***/
            kCount_rec = 0;
            break;

        default:
            break;
        }
        osDelay(20);
    }
}

void keyAnalog_Test(void) {

    char disp[30];
    uint16_t keyAnalog;

    keyIFR_ADCInit();

    for(;;) {

        keyAnalog = keyIFRGet_Adc(4);

        sprintf(disp,"valAnalog : %d\n\r", keyAnalog);
        Driver_USART1.Send(disp,strlen(disp));
        osDelay(500);
    }
}

uint16_t keyIFR_Scan(void) {

    uint16_t keyAnalog = keyIFRGet_Adc(4);
	
	if(keyAnalog < 150)return KEY_NULL;
	else return (10 - (keyAnalog / 400)) << 4;
}

void test_s10(void) {

    Driver_USART1.Send("abcd",4);
    osDelay(20);
}

void usr_sigin(void){

	measure_en = true;
}

void usr_sigout(void){

	IFR_Send((uint16_t *)HTtab,tabHp,(uint16_t *)LTtab,tabLp);
}

void keyIFR_Thread(const void *argument) {

    const char *Tips_Head = "����ת����չ�尴��";
    static Obj_eventKey myKeyIFREvent = {0};	//���������¼����Ƚ����ձ���Ҫ���ִ����¼���ֱ�Ӵ�����Ӧ�������ɣ��հ״��Զ��жϲ��ᴥ��
    static Obj_keyStatus myKeyStatus = {0};		//�����ж������־��ʼ��
    static funkeyThread key_ThreadIFR = key_Thread2;

    myKeyIFREvent.funKeySHORT[10] = test_s10;			//k10�̰�����
	
	myKeyIFREvent.funKeySHORT[7]  = usr_sigin;			//k7�̰�����
	myKeyIFREvent.funKeySHORT[9]  = usr_sigout;			//k9�̰�����

    key_ThreadIFR(keyIFR_ADCInit,&myKeyStatus,keyIFR_Scan,myKeyIFREvent,Tips_Head);
}

void keyIFRActive(void) {

	Remote_Init();
	keyIFR_ADCInit();
    tid_keyIFR_Thread = osThreadCreate(osThread(keyIFR_Thread),NULL);
}

