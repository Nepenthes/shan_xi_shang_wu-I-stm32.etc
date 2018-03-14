#include "Moudle_DEC.h"
#include "Tips.h"

Moudle_attr Moudle_GTA;

osThreadId tid_MBDEC_Thread;
osThreadDef(MBDEC_Thread,osPriorityNormal,1,512);

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

void stdDeInit(void){

	TIM_BDTRInitTypeDef TIM_BDTRStruct;
	
	TIM_BDTRStructInit(&TIM_BDTRStruct);

	EXTI_DeInit();

	ADC_DeInit(ADC1);
}

void MoudleDEC_ioInit(void){		//ģ����ų�ʼ��

	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD	| RCC_APB2Periph_GPIOE, ENABLE );	  //ʹ��ADC1ͨ��ʱ��

	GPIO_InitStructure.GPIO_Pin |= 0xfff8;				//Ӳ��ID������ų�ʼ��
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_7 | GPIO_Pin_4;		//Moudle_Check���ų�ʼ��
	GPIO_Init(GPIOE, &GPIO_InitStructure);
}

void MBDEC_Thread(const void *argument){	//ѭ�����
	
	const u16 signel_waitTime = 1500;
	
	u16 ID_temp,alive_temp;
	static u8 Alive_local = 0x00;	
	const char disp_size = 50;
	char disp[disp_size];
	
	bool M_CHG;
	
	osSignalWait(EVTSIG_MDEC_EN,osWaitForever);			//�װ��ʼ�����صȴ�
	osSignalClear(tid_MBDEC_Thread,EVTSIG_MDEC_EN);		
	
	for(;;){
	
		if(extension_IF){
			
			Moudle_GTA.Alive |= extension_FLG;
		}else Moudle_GTA.Alive &= ~extension_FLG;	//��չģ����  
		if(wireless_IF){
		
			Moudle_GTA.Alive |= wireless_FLG;
		}else Moudle_GTA.Alive &= ~wireless_FLG;	//����ͨ��ģ����
		if(LCD4_3_IF){
		
			Moudle_GTA.Alive |= LCD4_3_FLG;
		}else Moudle_GTA.Alive &= ~LCD4_3_FLG; 		//4.3��LCDģ����
		
		osDelay(200);
		
		if(Alive_local != Moudle_GTA.Alive){	//�Ƿ���ģ���Σ�����
		
			osDelay(500);
			if(Alive_local != Moudle_GTA.Alive){
			
				M_CHG = true;
			}
		}
		
		if(M_CHG){
			
			M_CHG = false;
		
			alive_temp = Alive_local ^ Moudle_GTA.Alive;	//��ȡ��α䶯ģ��
			Alive_local = Moudle_GTA.Alive;
			
			if(alive_temp & wireless_FLG){
			
				if(wireless_IF){
					
					ID_temp = (GPIO_ReadInputData(GPIOD) >> 3) & 0x001f;		//����ͨ��ģ��ID��ȡ
					Moudle_GTA.Wirless_ID = (u8)ID_temp;
					
					osSignalSet (tid_tips, EVTSIG_MSG_A);
					osDelay(signel_waitTime);
					
					osDelay(100);
					memset(disp,0,disp_size * sizeof(char));
					sprintf(disp,"����ͨѶģ�飺0x%02X(ID)�ѱ���װ������\r\n",Moudle_GTA.Wirless_ID);
					Driver_USART1.Send(disp,strlen(disp));
					
					switch(Moudle_GTA.Wirless_ID){				//ͨѶģ����������
					
						case MID_TRANS_Zigbee	:	break;
							
						case MID_TRANS_Wifi		:	break;
							
						default:break;
					}
					
					//wirelessThread_Active();	//���ߴ�������������ȴ�ʹ���źż��
					
				}else{
					
					osSignalSet (tid_tips, EVTSIG_MSG_B);
					osDelay(signel_waitTime);
																	
					memset(disp,0,disp_size * sizeof(char));
					sprintf(disp,"����ͨѶģ�飺0x%02X(ID)�ѱ��γ�\r\n",Moudle_GTA.Wirless_ID);
					Driver_USART1.Send(disp,strlen(disp));
					
					switch(Moudle_GTA.Wirless_ID){				//ͨѶģ��������ֹ
					
						case MID_TRANS_Zigbee	:	break;
							
						case MID_TRANS_Wifi		:	break;
							
						default:break;
					}
				}
			}
			
			if(alive_temp & extension_FLG){
			
				if(extension_IF){
					
					ID_temp = (GPIO_ReadInputData(GPIOD) >> 8) & 0x00ff;		//��չģ��ID��ȡ
					Moudle_GTA.Extension_ID = (u8)ID_temp;	
					
					osSignalSet (tid_tips, EVTSIG_EXT_A);
					osDelay(signel_waitTime);
					
					memset(disp,0,disp_size * sizeof(char));
					sprintf(disp,"��չģ�飺0x%02X(ID)�ѱ���װ������\r\n",Moudle_GTA.Extension_ID);
					Driver_USART1.Send(disp,strlen(disp));
					osDelay(20);
					
					switch(Moudle_GTA.Extension_ID){	//����һ����չģ����������
					
						case MID_SENSOR_FIRE :	
							
								fireMSThread_Active();
								break;
						
						case MID_SENSOR_PYRO :	
							
								pyroMSThread_Active();
								break;
						
						case MID_SENSOR_SMOKE :	
							
								smokeMSThread_Active();
								break;
						
						case MID_SENSOR_GAS  :	
							
								gasMSThread_Active();
								break;
						
						case MID_SENSOR_TEMP :	
							
								tempMSThread_Active();
								break;
						
						case MID_SENSOR_LIGHT:	
							
								lightMSThread_Active();
								break;
						
						case MID_SENSOR_ANALOG :	
							
								analogMSThread_Active();
								break;
						
						case MID_EGUARD :	
							
								Eguard_Active();
								break;
						
						case MID_EXEC_DEVIFR:	
							
								keyIFRActive();
								osSignalSet (tid_USARTDebug_Thread, USARTDEBUG_THREAD_EN);
								break;
						
						case MID_EXEC_SOURCE:  
							
								sourceCMThread_Active();
								break;
						
						case MID_EXEC_RELAYS:
								
								RelaysCMThread_Active();
								break;
						
						case MID_EXEC_CURTAIN:
							
								curtainCMThread_Active();
								break;
						
						case MID_EXEC_DEVPWM:	
							
								pwmCMThread_Active();
								break;
						
						case MID_EXEC_SPEAK:
							
								speakCMThread_Active();
								break;
							
						default:break;
					}
					
					osSignalSet (tid_USARTWireless_Thread, WIRLESS_THREAD_EN); //�������ߴ������
					
				}else{
					
					osSignalSet (tid_tips, EVTSIG_EXT_B);
					osDelay(signel_waitTime);
					
					stdDeInit();	//�ײ�����ȫ��ʧ��
					
					switch(Moudle_GTA.Extension_ID){	//����һ����չģ��������ֹ
					
						case MID_SENSOR_FIRE :	
							
								osThreadTerminate(tid_fireMS_Thread);
								break;
						
						case MID_SENSOR_PYRO :	
							
								osThreadTerminate(tid_pyroMS_Thread);
								break;
						
						case MID_SENSOR_SMOKE :	
							
								osThreadTerminate(tid_smokeMS_Thread);
								break;
						
						case MID_SENSOR_GAS  :	
							
								osThreadTerminate(tid_gasMS_Thread);
								break;
						
						case MID_SENSOR_TEMP :	
							
								osThreadTerminate(tid_tempMS_Thread);
								break;
						
						case MID_SENSOR_LIGHT:	
							
								osThreadTerminate(tid_lightMS_Thread);
								break;
						
						case MID_SENSOR_ANALOG :	
							
								osThreadTerminate(tid_analogMS_Thread);
								break;
						
						case MID_EGUARD :	
							
								Eguard_Terminate();
								break;
						
						case MID_EXEC_DEVIFR	 :	
							
								keyIFR_Terminate();
								break;
						
						case MID_EXEC_SOURCE :  
								
								osThreadTerminate(tid_sourceCM_Thread); 
								break;
						
						case MID_EXEC_RELAYS:
								
								osThreadTerminate(tid_RelaysCM_Thread); 
								break;
						
						case MID_EXEC_CURTAIN:
							
								osThreadTerminate(tid_curtainCM_Thread);
								break;
						
						case MID_EXEC_DEVPWM:	
							
								pwmCM_Terminate();
								break;
						
						case MID_EXEC_SPEAK:
							
								osThreadTerminate(tid_speakCM_Thread); 
								break;
						
						default:break;
					}
					
					memset(disp,0,disp_size * sizeof(char));
					sprintf(disp,"��չģ�飺0x%02X(ID)�ѱ��γ�\r\n",Moudle_GTA.Extension_ID);
					Driver_USART1.Send(disp,strlen(disp));
					osDelay(20);
					
					Moudle_GTA.Extension_ID = 0;
				}
			}
			
			if(alive_temp & LCD4_3_FLG){	//��ʾģ����ID��ֱ�Ӽ���Ƿ�����
			
				if(LCD4_3_IF){

					osDelay(100);
					memset(disp,0,disp_size * sizeof(char));
					sprintf(disp,"��ʾģ���ѱ���װ������\r\n");
					Driver_USART1.Send(disp,strlen(disp));
				}else{
					
					memset(disp,0,disp_size * sizeof(char));
					sprintf(disp,"��ʾģ���ѱ��γ�\r\n");
					Driver_USART1.Send(disp,strlen(disp));
				}
			}
		}
	}
}

void MoudleDEC_Init(void){	//ģ������̼���
	
	MoudleDEC_ioInit();
	tid_MBDEC_Thread = osThreadCreate(osThread(MBDEC_Thread),NULL);
}
