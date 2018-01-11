#include "Eguard.h"
#include "debugUart.h"

extern ARM_DRIVER_USART Driver_USART2;

extern osThreadId tid_fingerID_Thread;
extern osThreadId tid_rfID_Thread;
extern osThreadId tid_kBoard_Thread;
					 
osPoolId  EGUD_pool;								 
osPoolDef(EGUD_pool, 10, EGUARD_MEAS);                    // �ڴ�ض���
osMessageQId  MsgBox_EGUD_FID;		
osMessageQDef(MsgBox_EGUD_FID, 3, &EGUARD_MEAS);             // ��Ϣ���ж���
osMessageQId  MsgBox_MTEGUD;		
osMessageQDef(MsgBox_MTEGUD, 3, &EGUARD_MEAS);             // ��Ϣ���ж���
osMessageQId  MsgBox_DPEGUD;		
osMessageQDef(MsgBox_DPEGUD, 3, &EGUARD_MEAS);             // ��Ϣ���ж���

void Eguard_Active(void){
	
	static bool memInit_flg = false;
	
	if(!memInit_flg){

		EGUD_pool = osPoolCreate(osPool(EGUD_pool));	//�����ڴ��
		MsgBox_EGUD_FID = osMessageCreate(osMessageQ(MsgBox_EGUD_FID), NULL);	//������Ϣ����
		MsgBox_MTEGUD = osMessageCreate(osMessageQ(MsgBox_MTEGUD), NULL);	//������Ϣ����
		MsgBox_DPEGUD = osMessageCreate(osMessageQ(MsgBox_DPEGUD), NULL);	//������Ϣ����
		memInit_flg = true;
	}
	
	//fingerIDThread_Active();
	//rfIDThread_Active();
	kBoardThread_Active();
}

void Eguard_Terminate(void){

	osThreadTerminate(tid_fingerID_Thread);
	osThreadTerminate(tid_rfID_Thread);
	osThreadTerminate(tid_kBoard_Thread);
}

