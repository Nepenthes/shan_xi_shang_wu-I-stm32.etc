#include <Key&Tips.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern ARM_DRIVER_USART Driver_USART1;								//�豸�����⴮��һ�豸����

osThreadId tid_keyMboard_Thread;										//����������߳�ID
osThreadDef(keyMboard_Thread,osPriorityAboveNormal,1,1536);	//����������̶߳���

typedef void (* funkeyThread)(funKeyInit key_Init,Obj_keyStatus *orgKeyStatus,funKeyScan key_Scan,Obj_eventKey keyEvent,const char *Tips_head);

/***���������ʼ��***/
void keyInit(void){	

	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE,ENABLE);		//ʱ�ӷ���
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;	//�˿����Է���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
}

/***������ֵ��ȡ***/
static uint16_t keyScan(void){

	if(!K1)return KEY_VALUE_1;		//��ֵ1
	if(!K2)return KEY_VALUE_2;		//��ֵ2
	return KEY_NULL;					//�ް���
}

/***�������״̬��***/
static uint16_t getKey(Obj_keyStatus *orgKeyStatus,funKeyScan keyScan){

	static	uint16_t s_u16KeyState 		= KEY_STATE_INIT;		//״̬�����״̬����ʼ��״̬
	static	uint16_t	s_u16LastKey		= KEY_NULL;				//������ʷ������ֵ	
	static	uint8_t	KeyKeepComfirm		= 0;						//������ȷ�ϱ��� ȷ������ʱ����ʱ
	static	uint16_t	s_u16KeyTimeCount	= 0;						//����ʱ�����壨������KEY_TICK���м����������������ֵ��ȷ���Ƿ����ڳ�����
				uint16_t keyTemp				= KEY_NULL;				/*ʮ�����Ƶ�һλ������״̬��	�ڶ�λ�����ּ���ֵ��		����λ����ֵ��		����λ����������ֵ*/
	
	static	uint32_t osTick_last			= 0xffff0000;			//��osTick���м�¼����������һ��osTick���жԱȻ�ȡ������˼�������жϰ����Ƿ������������£�
	
	keyTemp = keyScan();		//��ȡ��ֵ
	
	switch(s_u16KeyState){	//��ȡ״̬��״̬
	
		case KEY_STATE_INIT:	//��ʼ��״̬
			
				if(orgKeyStatus->keyCTflg){	//�����һ���Ƿ�Ϊ����
				
					if((osKernelSysTick() - osTick_last) > KEY_CONTINUE_PERIOD){	//��һ�����������Ȿ���Ƿ��������
					
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
		
				if(KEY_NULL != keyTemp){		//�����Ƿ���
				
					s_u16LastKey 	= keyTemp;	//�洢������ֵ
					keyTemp 		  |= KEY_DOWN;	//����״̬ȷ��Ϊ����
					s_u16KeyState	= KEY_STATE_LONG;	//������Ȼδ�����л�״̬���������
				}else{
				
					s_u16KeyState	= KEY_STATE_INIT;	//���Ϊ�������������״̬�����س�ʼ��״̬
				}
				break;
				
		case KEY_STATE_LONG:		//����״̬���
				
				if(KEY_NULL != keyTemp){	//�����Ƿ���
					
					if(++s_u16KeyTimeCount > KEY_LONG_PERIOD){	//��������Ȼδ��������ݳ���ʱ�����м�������ȷ���Ƿ�Ϊ����
					
						s_u16KeyTimeCount	= 0;			//ȷ�ϳ�������������ֵ����
						orgKeyStatus->sKeyKeep				= 0;			//���״̬���л�״̬ǰ���Ա���ȷ��״̬�������ֵ��ǰ���㣬������ֵΪ�����󱣳ּ�������
						KeyKeepComfirm		= 0;			//���״̬���л�״̬ǰ���Ա���ȷ��״̬�������ֵ��ǰ���㣬����ֵ���ڶ��峤�����ý��б��ּ���
						keyTemp			  |= KEY_LONG;	//����״̬ȷ��Ϊ����
						s_u16KeyState		= KEY_STATE_KEEP;	//������Ȼδ�����л�״̬���������ּ��
						
						orgKeyStatus->keyOverFlg			= KEY_OVER_LONG;	//����
					}else	orgKeyStatus->keyOverFlg	= KEY_OVER_SHORT;		//�̰�
				}else{
				
					s_u16KeyState	= KEY_STATE_RELEASE;	//���ȷ��Ϊ�����󰴼������л�״̬��������
				}
				break;
				
		case KEY_STATE_KEEP:		//�����󱣳�״̬���
			
				if(KEY_NULL != keyTemp){		//�����Ƿ���
				
					if(++s_u16KeyTimeCount > KEY_KEEP_PERIOD){	//��������Ȼδ��������ݳ���ʱ�����м�������ȷ���Ƿ�Ϊ�������������
						
						s_u16KeyTimeCount	= 0;			//ȷ�ϳ�����������֣���������ֵ����
						if(KeyKeepComfirm < (KEY_COMFIRM + 3))KeyKeepComfirm++;			//����Ƿ��������ʱ��
						if(orgKeyStatus->sKeyKeep < 15 && KeyKeepComfirm > KEY_COMFIRM)orgKeyStatus->sKeyKeep++; 	//��⵽���������ִ�г����󱣳ּ���
						if(orgKeyStatus->sKeyKeep){		//��⵽�����󱣳ּ���ֵ��Ϊ�㣬��ȷ�ϰ���״̬Ϊ������������֣��Է���ֵ������Ӧȷ�ϴ���
						
							orgKeyStatus->keyOverFlg	 = KEY_OVER_KEEP;	//״̬ȷ��Ϊ������Ϊ����
							keyTemp	|= orgKeyStatus->sKeyKeep << 8;		//���ּ�����������8λ����ʮ������keyTemp�ڶ�λ
							keyTemp	|= KEY_KEEP;			//����״̬ȷ��Ϊ�������������
						}		
					}
				}else{
				
					s_u16KeyState	= KEY_STATE_RELEASE;	//����״̬ȷ��Ϊ�������������֮�����л�״̬��������
				}
				break;
				
		case KEY_STATE_RELEASE:	//����״̬���
				
				s_u16LastKey |= KEY_UP;	//�洢����״̬
				keyTemp		  = s_u16LastKey;	//����״̬ȷ��Ϊ����
				s_u16KeyState = KEY_STATE_RECORD;	//�л�״̬������������¼
				break;
		
		case KEY_STATE_RECORD:	//����������¼״̬���

				if((osKernelSysTick() - osTick_last) < KEY_CONTINUE_PERIOD){	//�����ΰ�������ʱ����С�ڹ涨ֵ�����ж�Ϊ����
					
					orgKeyStatus->sKeyCount++;	//��������		
				}else{
					
					orgKeyStatus->sKeyCount = 0;	//�����Ͽ�����������
				}
				
				if(orgKeyStatus->sKeyCount){		//������������Ϊ�㣬��ȷ��Ϊ�����������Է���ֵ������Ӧ����
													
					orgKeyStatus->keyCTflg	= 1;	//��������־
					keyTemp	= s_u16LastKey & 0x00f0;	//��ȡ��ֵ
					keyTemp	|=	KEY_CONTINUE;				//ȷ��Ϊ��������
					if(orgKeyStatus->sKeyCount < 15)keyTemp += orgKeyStatus->sKeyCount;	//�����������ݷ���ʮ������keyTemp����λ�����λ��
				}
				
				s_u16KeyState	= KEY_STATE_INIT;		//���״̬�����س�ʼ״̬
				osTick_last	 	= osKernelSysTick();	//��¼osTick�������´��������Ա�
				break;
		
		default:break;
	}
	return keyTemp;	//���ذ���״̬�ͼ�ֵ
}

/*������ʼ������������״̬����ṹ�壬����ɨ�躯�������������¼�������������ʾ��Ϣͷ*/
void key_Thread(funKeyInit key_Init,
					 Obj_keyStatus *orgKeyStatus,
					 funKeyScan key_Scan,Obj_eventKey keyEvent,
					 const char *Tips_head){
	
/***�������ԣ�����1����������Ϣ��****/
	static uint16_t keyVal;						//����״̬�¼�
	static uint8_t	key_temp;					//������ֵ����
	static uint8_t	kCount;						//��������ֵ�������������ּ�������������ʹ��ͬһ����������Ϊ����״̬����ͬʱ����
	static uint8_t	kCount_rec;			//��ʷ����ֵ����
	
	static osThreadId ID_Temp;					//��ǰ�߳�ID����
	static osEvent evt;
	static uint8_t KEY_DEBUG_FLG = 0;

	const	 uint8_t	tipsLen = 80;		//Tips��ӡ�ַ�������
	static char	key_tempDisp;
	static char	kCountDisp;
	static char	kCount_recDisp;
	static char	tips[tipsLen];					//Tips�ַ���
	
	key_Init();

	for(;;){
		
		keyVal = getKey(orgKeyStatus,key_Scan);    //��ȡ��ֵ
		
		ID_Temp = osThreadGetId();
		evt = osSignalWait (KEY_DEBUG_OFF, 1);		 //��ȡDebug_log���Ȩ���ź�
		if (evt.value.signals == KEY_DEBUG_OFF){
		
			KEY_DEBUG_FLG = 0;
			osSignalClear(ID_Temp ,KEY_DEBUG_OFF);
		}else{
		
			evt = osSignalWait (KEY_DEBUG_ON, 1);
			if (evt.value.signals == KEY_DEBUG_ON){
			
				KEY_DEBUG_FLG = 1;
				osSignalClear(ID_Temp ,KEY_DEBUG_ON);
			}		
		}  
		
if(KEY_DEBUG_FLG){
	
		memset(tips,0,tipsLen*sizeof(char));	//ÿ��Tips��ӡ�����
		strcat(tips,"Tips:");						//Tips��ʶ
		strcat(tips,Tips_head);
		strcat(tips,"-");
}
/*------------------------------------------------------------------------------------------------------------------------------*/		
		switch(keyVal & 0xf000){
		
			case KEY_LONG		:	
				
					key_temp = (uint8_t)((keyVal & 0x00f0) >> 4);
if(KEY_DEBUG_FLG){/*Debug_log���ʹ��*/
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
if(KEY_DEBUG_FLG){/*Debug_log���ʹ��*/					
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
if(KEY_DEBUG_FLG){/*Debug_log���ʹ��*/				
					strcat(tips,"����");	
					key_tempDisp = key_temp + '0';
					strcat(tips,(const char*)&key_tempDisp);	
					strcat(tips,"����\r\n");	
					Driver_USART1.Send(tips,strlen(tips));
}/***/			
					break;
					
			case KEY_UP			:
if(KEY_DEBUG_FLG){/*Debug_log���ʹ��*/			
					strcat(tips,"����");	
					key_tempDisp = key_temp + '0';
					strcat(tips,(const char*)&key_tempDisp);	
}/***/						
					switch(orgKeyStatus->keyOverFlg){

							case KEY_OVER_SHORT		:	
								
								   if(keyEvent.funKeySHORT[key_temp])keyEvent.funKeySHORT[key_temp]();		//�����¼��������ȼ�ⴥ���¼��Ƿ񴴽���û�����򲻽��д���
if(KEY_DEBUG_FLG){/*Debug_log���ʹ��*/								
									strcat(tips,"�̰�����\r\n");	
									Driver_USART1.Send(tips,strlen(tips));
									orgKeyStatus->keyOverFlg = 0;
}/***/							
									break;

							case KEY_OVER_LONG		:
								
									if(keyEvent.funKeyLONG[key_temp])keyEvent.funKeyLONG[key_temp]();
if(KEY_DEBUG_FLG){/*Debug_log���ʹ��*/								
									strcat(tips,"��������\r\n");	
									Driver_USART1.Send(tips,strlen(tips));
									orgKeyStatus->keyOverFlg = 0;
}/***/							
									break;

							case KEY_OVER_KEEP		:	
								
									if(keyEvent.funKeyKEEP[key_temp][kCount_rec])keyEvent.funKeyKEEP[key_temp][kCount_rec]();
if(KEY_DEBUG_FLG){/*Debug_log���ʹ��*/									
									strcat(tips,"�����󱣳�");
									kCount_recDisp = kCount_rec + '0';
									strcat(tips,(const char*)&kCount_recDisp);
									strcat(tips,"�μ��������\r\n");
									Driver_USART1.Send(tips,strlen(tips));
									kCount_rec = 0;
}/***/							
									break;			
							default:break;
						}
						break;
					
			case KEY_CONTINUE	:
				
					kCount 		= (uint8_t)((keyVal & 0x000f) >> 0);	//��ȡ����ֵ
					kCount_rec	= kCount + 1;
if(KEY_DEBUG_FLG){/*Debug_log���ʹ��*/	
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
if(KEY_DEBUG_FLG){/*Debug_log���ʹ��*/					
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
					
			default:break;
		}
		osDelay(KEY_TICK);
	}
}

void abc(void){

	Driver_USART1.Send("abcd",4);
	osDelay(20);
}

/***����������߳�***/
void keyMboard_Thread(const void *argument){
	
	const char *Tips_Head = "�װ尴��";
	static Obj_eventKey myKeyEvent = {0};					//���������¼����Ƚ����ձ���Ҫ���ִ����¼���ֱ�Ӵ�����Ӧ�������ɣ��հ״��Զ��жϲ��ᴥ��
	static Obj_keyStatus myKeyStatus = {0};		//�����ж������־��ʼ��
	static funkeyThread key_ThreadMB = key_Thread;
	
	myKeyEvent.funKeyCONTINUE[2][6] = abc;			//�趨����������6�δ����¼�

	key_ThreadMB(keyInit,&myKeyStatus,keyScan,myKeyEvent,Tips_Head);	
}
	
void keyMboardActive(void){
	
	tid_keyMboard_Thread = osThreadCreate(osThread(keyMboard_Thread),NULL);
}



