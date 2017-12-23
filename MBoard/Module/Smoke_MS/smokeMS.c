#include "smokeMS.h"

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_smokeMS_Thread;
osThreadDef(smokeMS_Thread,osPriorityNormal,1,512);
			 
osPoolId  smokeMS_pool;								 
osPoolDef(smokeMS_pool, 10, smokeMS_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_smokeMS;
osMessageQDef(MsgBox_smokeMS, 2, &smokeMS_MEAS);            // ��Ϣ���ж��壬����ģ���߳�������ͨѶ�߳�
osMessageQId  MsgBox_MTsmokeMS;
osMessageQDef(MsgBox_MTsmokeMS, 2, &smokeMS_MEAS);          // ��Ϣ���ж���,��������ͨѶ�߳���ģ���߳�
osMessageQId  MsgBox_DPsmokeMS;
osMessageQDef(MsgBox_DPsmokeMS, 2, &smokeMS_MEAS);          // ��Ϣ���ж��壬����ģ���߳�����ʾģ���߳�

void smokeMS_Init(void){

	;
}

void smokeMS_Thread(const void *argument){

	;
}

void smokeMSThread_Active(void){

	smokeMS_Init();
	smokeMS_pool   = osPoolCreate(osPool(smokeMS_pool));	//�����ڴ��
	MsgBox_smokeMS 	= osMessageCreate(osMessageQ(MsgBox_smokeMS), NULL);	//������Ϣ����
	MsgBox_MTsmokeMS = osMessageCreate(osMessageQ(MsgBox_MTsmokeMS), NULL);//������Ϣ����
	MsgBox_DPsmokeMS = osMessageCreate(osMessageQ(MsgBox_DPsmokeMS), NULL);//������Ϣ����
	tid_smokeMS_Thread = osThreadCreate(osThread(smokeMS_Thread),NULL);
}
