#include "fireMS.h"

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_fireMS_Thread;
osThreadDef(fireMS_Thread,osPriorityNormal,1,512);
			 
osPoolId  fireMS_pool;								 
osPoolDef(fireMS_pool, 10, fireMS_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_fireMS;
osMessageQDef(MsgBox_fireMS, 2, &fireMS_MEAS);            // ��Ϣ���ж��壬����ģ���߳�������ͨѶ�߳�
osMessageQId  MsgBox_MTfireMS;
osMessageQDef(MsgBox_MTfireMS, 2, &fireMS_MEAS);          // ��Ϣ���ж���,��������ͨѶ�߳���ģ���߳�
osMessageQId  MsgBox_DPfireMS;
osMessageQDef(MsgBox_DPfireMS, 2, &fireMS_MEAS);          // ��Ϣ���ж��壬����ģ���߳�����ʾģ���߳�

void fireMS_Init(void){

	;
}

void fireMS_Thread(const void *argument){

	;
}

void fireMSThread_Active(void){

	fireMS_Init();
	fireMS_pool   = osPoolCreate(osPool(fireMS_pool));	//�����ڴ��
	MsgBox_fireMS 	= osMessageCreate(osMessageQ(MsgBox_fireMS), NULL);	//������Ϣ����
	MsgBox_MTfireMS = osMessageCreate(osMessageQ(MsgBox_MTfireMS), NULL);//������Ϣ����
	MsgBox_DPfireMS = osMessageCreate(osMessageQ(MsgBox_DPfireMS), NULL);//������Ϣ����
	tid_fireMS_Thread = osThreadCreate(osThread(fireMS_Thread),NULL);
}
