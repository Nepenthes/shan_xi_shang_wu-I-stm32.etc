#include "infraTrans.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static IFR_MEAS ifrCM_Attr;
static uint8_t  ifrDevMOUDLE_ID;

osThreadId tid_keyIFR_Thread;
osThreadId tid_keyIFR_Thread_umdScan;
osThreadDef(keyIFR_Thread,osPriorityAboveNormal,1,3072);
osThreadDef(keyIFR_Thread_umdScan,osPriorityAboveNormal,1,512);

osPoolId  IFR_pool;								 
osPoolDef(IFR_pool, 10, IFR_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_IFR;
osMessageQDef(MsgBox_IFR, 2, &IFR_MEAS);            // ��Ϣ���ж��壬����ģ���߳�������ͨѶ�߳�
osMessageQId  MsgBox_MTIFR;
osMessageQDef(MsgBox_MTIFR, 2, &IFR_MEAS);          // ��Ϣ���ж���,��������ͨѶ�߳���ģ���߳�
osMessageQId  MsgBox_DPIFR;
osMessageQDef(MsgBox_DPIFR, 2, &IFR_MEAS);          // ��Ϣ���ж��壬����ģ���߳�����ʾģ���߳�

osPoolId  memID_IFRsigK_pool;								 
osPoolDef(IFRsigK_pool, 4, IFR_kMSG);              // ������Ϣ�ڴ�ض���
osMessageQId  MsgBox_IFRsigK;
osMessageQDef(MsgBox_IFRsigK, 2, &IFR_kMSG);        // ��Ϣ���ж��壬���ڵװ尴���źŶ�ȡ

const uint8_t IFR_PER  = 2;		//��ƽ�����ֱ��� 2us
const uint8_t KEY_NUM = 11;
bool measure_en = true;

uint8_t tabHp,tabLp;		
uint16_t HTtab[Tab_size];
uint16_t LTtab[Tab_size];	//����ifr�ز���Ϣ����

uint16_t ifrKB_HTtab[KEY_NUM][Tab_size];	
uint16_t ifrKB_LTtab[KEY_NUM][Tab_size];
uint8_t  ifrKB_tabHp[KEY_NUM],ifrKB_tabLp[KEY_NUM];	//����ifr�ز���Ϣ������

typedef void (* funkeyThread)(funKeyInit key_Init,Obj_keyStatus *orgKeyStatus,funKeyScan key_Scan,Obj_eventKey keyEvent,const char *Tips_head);

extern ARM_DRIVER_USART Driver_USART1;								//�豸�����⴮��һ�豸����

void IFR_leyInit_X(void){
	
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE );	  //ʹ��ADC1ͨ��ʱ��

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;		//ģ����������
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 			
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	PAout(0) = PAout(1) = PAout(4) = PAout(5) = 0;
	PBin(8)  = PBin(9)  = PBin(10) = 1; 
}

void IFR_leyInit_Y(void){
	
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE );	  //ʹ��ADC1ͨ��ʱ��

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;		//ģ����������
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 			
	GPIO_Init(GPIOB, &GPIO_InitStructure);	

	PAin(0)  = PAin(1)  = PAin(4)   = PAin(5) = 1;
	PBout(8) = PBout(9) = PBout(10) = 0; 
}

uint8_t IFR_kScan_A(void){

	uint8_t key = 0;
	
	IFR_leyInit_X();
	
	if(!PBin(8)) key = 1;
	if(!PBin(9)) key = 2;
	if(!PBin(10))key = 3;
	
	IFR_leyInit_Y();
	
	if(!PAin(0))key += 0;
	if(!PAin(1))key += 3;
	if(!PAin(4))key += 6;
	if(!PAin(5))key += 9;
	
	switch(key){		//ǿ����
	
		case 1 :	key = 1;break;
			
		case 2 :	key = 3;break;
			
		case 3 :	key = 2;break;
			
		case 4 :	key = 4;break;
			
		case 5 :	key = 5;break;
			
		case 6 :	key = 6;break;
			
		case 8 :	key = 7;break;
			
		case 10:	key = 8;break;
			
		case 11:	key = 9;break;
			
		case 12:	key =10;break;
		
		default:	key = ifrvalK_NULL;
					break;
	}
	
	return key;
}

uint8_t IFR_kScan_B(void){

	IFR_leyInit_X();
	
	delay_us(10);
	
	if((!PBin(8)) | (!PBin(9)) | (!PBin(10))){
		
		return IFR_kScan_A();	
	}else return ifrvalK_NULL;
}

void Remote_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO,ENABLE);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource6);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_SetBits(GPIOC,GPIO_Pin_6);
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	PBout(15) = 0;
	PBout(13) = 1;

	EXTI_ClearITPendingBit(EXTI_Line6);
	
    EXTI_InitStructure.EXTI_Line = EXTI_Line6;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

	NVIC_PriorityGroupConfig(2);
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //��ռ���ȼ�0��
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //�����ȼ�3��
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
    NVIC_Init(&NVIC_InitStructure);
	
	PCin(6) = 1;
}

u16 HW_ReceiveTime(void)
{
    u16 t=0;
	const uint16_t MAX = 60000 / IFR_PER;

    while(PCin(6))
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
	const uint16_t MAX = 60000 / IFR_PER;

    while(!PCin(6))
    {
        t++;
        delay_us(IFR_PER);
		
        if(t >= MAX) return t;
    }

    return t;
}


void IFR_Send(uint16_t HTab[],uint8_t Hp,uint16_t LTab[],uint8_t Lp){

	const uint16_t MAX = 6000 / IFR_PER; //��ʱ�䱣���źŲ�����׼
	uint8_t loop;
	uint16_t temp;
	
//	char disp[20];							//��Ӧ����λ�źų����������
//	
//	memset(disp,0,sizeof(char) * 20);
//	sprintf(disp,"%d",HTab[14]);
//	Driver_USART1.Send(disp,strlen(disp));
	
	PBout(15) = 0;
	for(loop = 0;loop < Lp;loop ++){
	
		if(LTab[loop] < MAX){
		
			temp = (LTab[loop] + LTab[loop] / 9) / 10;	//�ز����� 14Ϊʱ�䲹����12(����ֵ) = 13-1(13 Ϊ 26 / IFR_PER) �� 26Ϊ38k�ز�������
		}	
		else{
		
			temp = (LTab[loop] + LTab[loop] / 9) / 10 + 20UL; //��ʱ�䱣���źŲ���
		}
		while(temp --){			//38k�ز������ڵ��� 
		
			PBout(15) = 1;		
			delay_us(8);
			PBout(15) = 0;
			delay_us(16);			
		}
		
		PBout(15) = 0;		    //�͵�ƽ�źŻ���
		temp = HTab[loop] + HTab[loop] / 20;
		while(temp --)delay_us(IFR_PER);
	}
	
	PBout(15) = 0;
}

uint8_t keyIFR_Scan(void) {

	return IFR_kScan_B();
}

void keyAnalog_Test(void) {

    char disp[30];
    uint16_t keyAnalog;

    for(;;) {

        keyAnalog = keyIFR_Scan();

        sprintf(disp,"valAnalog : %d\n\r", keyAnalog);
        Driver_USART1.Send(disp,strlen(disp));
        osDelay(500);
    }
}

void KB_ifrDats_Save(u8 num){

//	memset(ifrKB_HTtab[num], 0, Tab_size * sizeof(uint16_t));
//	memset(ifrKB_LTtab[num], 0, Tab_size * sizeof(uint16_t));
//	
//	memcpy(ifrKB_HTtab[num],(uint16_t *)HTtab,tabHp * 2);	//��������Ϊ u16,��memcpy���ֽ�Ϊ��λ������2
//	memcpy(ifrKB_LTtab[num],(uint16_t *)LTtab,tabLp * 2);
//	
//	ifrKB_tabHp[num] = tabHp;
//	ifrKB_tabLp[num] = tabLp;
	
	STMFLASH_Write(MODULE_IFRdatsHpn_DATADDR + (num * 2),(uint16_t *)&tabHp,1);	//��ַ����Ϊ2�ı�����������2���ܿ�����
	STMFLASH_Write(MODULE_IFRdatsLpn_DATADDR + (num * 2),(uint16_t *)&tabLp,1);
	STMFLASH_Write(MODULE_IFRdatsHp_DATADDR + (num * 256),(uint16_t *)HTtab,tabHp);	//����ַ�洢�������飬���账������
	STMFLASH_Write(MODULE_IFRdatsLp_DATADDR + (num * 256),(uint16_t *)LTtab,tabLp);
}

void usr_sigin(void){

	uint16_t Ktemp;
	uint8_t	 cnt;
	
	ifrCM_Attr.VAL_KEY = ifrvalK_NULL;
	
	beeps(4);

lrn_start:
	
	cnt = 5;
	while(cnt --){	//tips ��ʼѧϰ
	
		IFRLRN_STATUS = !IFRLRN_STATUS;
		osDelay(150);
	}IFRLRN_STATUS = 0;
	
	ifrCM_Attr.VAL_KEY = ifrvalK_NULL;
	ifrCM_Attr.STATUS = kifrSTATUS_WAITK;
	
	for(;;){	//tips �ȴ��洢Ŀ�갴��
		
		if(ifrCM_Attr.VAL_KEY == 1){IFRLRN_STATUS = 1;beeps(3);return;}
		
		Ktemp = ifrCM_Attr.VAL_KEY;
		
		if(ifrCM_Attr.VAL_KEY == ifrvalK_NULL){
		
			IFRLRN_STATUS = 0;
			osDelay(20);
		}else break;
	}
	
	osDelay(100);
	measure_en = true;
	
	beeps(1);
	
	ifrCM_Attr.STATUS = kifrSTATUS_WAITSG;
	
	while(measure_en){	//�ȴ�ң���ź�
		
		if(ifrCM_Attr.VAL_KEY == 1){IFRLRN_STATUS = 1;beeps(3);return;}
		
		if(ifrCM_Attr.VAL_KEY != ifrvalK_NULL)Ktemp = ifrCM_Attr.VAL_KEY;
	
		IFRLRN_STATUS = !IFRLRN_STATUS;
		osDelay(150);
	}IFRLRN_STATUS = 1;
	
	KB_ifrDats_Save(Ktemp);	//Ŀ�갴���洢ifr�ز���Ϣ
	
	cnt = 5;
	while(cnt --){	//tips	�洢ѧϰ���
	
		IFRLRN_STATUS = !IFRLRN_STATUS;
		osDelay(150);
	}IFRLRN_STATUS = 1;
	
	beeps(0);
	
	ifrCM_Attr.STATUS = kifrSTATUS_LRNOVR;
	osDelay(500);
	
goto lrn_start;
}

void keyIFR_Thread_umdScan(const void *argument){

	osEvent  evt;
    osStatus status;	
	
	IFR_MEAS *mptr = NULL;
	IFR_MEAS *rptr = NULL;
	
	uint8_t ifrKey_temp;
	
	static IFR_MEAS ifrCM_Attr_tempDP;
	
	for(;;){
	
		evt = osMessageGet(MsgBox_MTIFR, 100);
		if (evt.status == osEventMessage){		//�ȴ���Ϣָ��
			
			rptr = evt.value.p;
			/*�Զ��屾���߳̽������ݴ��������������������������*/
			
			if(rptr->Mod_addr == ifrDevMOUDLE_ID)
				ifrCM_Attr.VAL_KEY = rptr->VAL_KEY;

			do{status = osPoolFree(IFR_pool, rptr);}while(status != osOK);	//�ڴ��ͷ�
			rptr = NULL;
		}
		
		ifrKey_temp = keyIFR_Scan();
		if(ifrvalK_NULL != ifrKey_temp){ifrCM_Attr.VAL_KEY = ifrKey_temp;}
		
		if(ifrCM_Attr_tempDP.VAL_KEY != ifrCM_Attr.VAL_KEY||
		   ifrCM_Attr_tempDP.STATUS  != ifrCM_Attr.STATUS){
			   
			    ifrCM_Attr_tempDP.VAL_KEY = ifrCM_Attr.VAL_KEY;
			    ifrCM_Attr_tempDP.STATUS  = ifrCM_Attr.STATUS;
		   
				do{mptr = (IFR_MEAS *)osPoolCAlloc(IFR_pool);}while(mptr == NULL);	//1.44��Һ����ʾ��Ϣ����
				mptr->speDPCMD = SPECMD_ifrDevDATS_CHG;
				mptr->VAL_KEY  = ifrCM_Attr.VAL_KEY;
				mptr->STATUS   = ifrCM_Attr.STATUS;
				osMessagePut(MsgBox_DPIFR, (uint32_t)mptr, 100);
				osDelay(500);
		}
		   
		osDelay(10);
	}
}

void keyIFR_Thread(const void *argument){
	
	osEvent  evt;
    osStatus status;
	
//	char disp[30];
	pwmCM_kMSG *rptr_sigK = NULL;
	IFR_MEAS   *mptr 	  = NULL;
	
	ifrCM_Attr.VAL_KEY = ifrvalK_NULL;
	
	STMFLASH_Read(MODULE_IFRid_DATADDR,(uint16_t *)&ifrDevMOUDLE_ID,1);
	if((ifrDevMOUDLE_ID != ifrDevMID_video)&&\
	   (ifrDevMOUDLE_ID != ifrDevMID_audio))ifrDevMOUDLE_ID = ifrDevMID_audio;
	
	osDelay(500);	//������ʱ
	
	do{mptr = (IFR_MEAS *)osPoolCAlloc(IFR_pool);}while(mptr == NULL);	
	mptr->speDPCMD = SPECMD_ifrDevModADDR_CHG;
	mptr->Mod_addr = ifrDevMOUDLE_ID;
	osMessagePut(MsgBox_DPIFR, (uint32_t)mptr, 100);
	
	for(;;){
		
	/********************************�װ尴���߳����ݽ���******************************************/
		evt = osMessageGet(MsgBox_IFRsigK, 100);
		if (evt.status == osEventMessage){
			
			IFR_MEAS *mptr = NULL;
		
			rptr_sigK = evt.value.p;
			/*�װ尴����Ϣ���������������������������*/
			
			ifrDevMOUDLE_ID = rptr_sigK->mADDR;
			STMFLASH_Write(MODULE_IFRid_DATADDR,(uint16_t *)&ifrDevMOUDLE_ID,1);
			
			do{mptr = (IFR_MEAS *)osPoolCAlloc(IFR_pool);}while(mptr == NULL);	//�װ尴���¼�����
			mptr->speDPCMD = SPECMD_ifrDevModADDR_CHG;
			mptr->Mod_addr = ifrDevMOUDLE_ID;
			osMessagePut(MsgBox_DPIFR, (uint32_t)mptr, 100);
			beeps(9);
			
			do{status = osPoolFree(memID_IFRsigK_pool, rptr_sigK);}while(status != osOK);	//�ڴ��ͷ�
			rptr_sigK = NULL;
		}
		
	/************************************���߳�***************************************************/
		ifrCM_Attr.STATUS = kifrSTATUS_NONLRN;
		
		if(ifrCM_Attr.VAL_KEY != ifrvalK_NULL){
		
			if(ifrCM_Attr.VAL_KEY == 1){usr_sigin(); ifrCM_Attr.VAL_KEY = ifrvalK_NULL;}
			else{
					
				ifrCM_Attr.STATUS = kifrSTATUS_SGOUT;
				osDelay(300);
			
				memset(HTtab, 0, Tab_size * sizeof(uint16_t));
				memset(LTtab, 0, Tab_size * sizeof(uint16_t));
				
				STMFLASH_Read(MODULE_IFRdatsHpn_DATADDR + (ifrCM_Attr.VAL_KEY * 2),(uint16_t *)&tabHp,1);	//��ַ����Ϊ2�ı�����������2���ܿ�����
				STMFLASH_Read(MODULE_IFRdatsLpn_DATADDR + (ifrCM_Attr.VAL_KEY * 2),(uint16_t *)&tabLp,1);
				
				STMFLASH_Read(MODULE_IFRdatsHp_DATADDR + (ifrCM_Attr.VAL_KEY * 256),(uint16_t *)HTtab,tabHp);	//����ַ�洢�������飬���账������
				STMFLASH_Read(MODULE_IFRdatsLp_DATADDR + (ifrCM_Attr.VAL_KEY * 256),(uint16_t *)LTtab,tabLp);
				
				if((HTtab[0] + HTtab[1]+ LTtab[0] + LTtab[1]) > 65000){ifrCM_Attr.VAL_KEY = ifrvalK_NULL; continue;} //ȡ���������Ƿ����ֵ��δ���洢����ֵ�����޷�ʹ��

				IFR_Send(HTtab,tabHp,LTtab,tabLp);
				osDelay(20);
//				IFR_Send(HTtab,tabHp,LTtab,tabLp);
				
//				IFR_Send((uint16_t *)ifrKB_HTtab[ifrCM_Attr.VAL_KEY],ifrKB_tabHp[ifrCM_Attr.VAL_KEY],(uint16_t *)ifrKB_LTtab[ifrCM_Attr.VAL_KEY],ifrKB_tabLp[ifrCM_Attr.VAL_KEY]);
//				osDelay(20);
//				IFR_Send((uint16_t *)ifrKB_HTtab[ifrCM_Attr.VAL_KEY],ifrKB_tabHp[ifrCM_Attr.VAL_KEY],(uint16_t *)ifrKB_LTtab[ifrCM_Attr.VAL_KEY],ifrKB_tabLp[ifrCM_Attr.VAL_KEY]);				
				
				tips_beep(6,100,1);
				ifrCM_Attr.VAL_KEY = ifrvalK_NULL;
			}
		}
	
//		sprintf(disp,"%d\n",ifrCM_Attr.VAL_KEY);	/*�������_����ֵ*/
//		Driver_USART1.Send(disp,strlen(disp));
//		if(ifrCM_Attr.VAL_KEY != ifrvalK_NULL)ifrCM_Attr.VAL_KEY = ifrvalK_NULL;
		
		osDelay(50);
	}
	
//	for(;;){					/*�������_����ֵ*/
//	
//		keyAnalog_Test();
//	}
}

void keyIFR_Terminate(void){

	osThreadTerminate(tid_keyIFR_Thread); 
	osThreadTerminate(tid_keyIFR_Thread_umdScan); 
}

void keyIFRActive(void){

	static bool memInit_flg = false;
	
	Remote_Init();
	
	tid_keyIFR_Thread = osThreadCreate(osThread(keyIFR_Thread),NULL);
	tid_keyIFR_Thread_umdScan = osThreadCreate(osThread(keyIFR_Thread_umdScan),NULL);
	
	if(!memInit_flg){

		IFR_pool     = osPoolCreate(osPool(IFR_pool));	//�����ڴ��
		MsgBox_IFR 	 = osMessageCreate(osMessageQ(MsgBox_IFR), NULL);	//������Ϣ����
		MsgBox_MTIFR = osMessageCreate(osMessageQ(MsgBox_MTIFR), NULL);	//������Ϣ����
		MsgBox_DPIFR = osMessageCreate(osMessageQ(MsgBox_DPIFR), NULL);	//������Ϣ����
		
		memID_IFRsigK_pool = osPoolCreate(osPool(IFRsigK_pool));
		MsgBox_IFRsigK = osMessageCreate(osMessageQ(MsgBox_IFRsigK), NULL);
		
		memInit_flg = true;
	}
}

