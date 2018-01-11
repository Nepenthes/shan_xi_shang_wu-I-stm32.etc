#include "templeMS.h"

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_templeMS_Thread;
osThreadDef(templeMS_Thread,osPriorityNormal,1,512);
			 
osPoolId  templeMS_pool;								 
osPoolDef(templeMS_pool, 10, templeMS_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_templeMS;
osMessageQDef(MsgBox_templeMS, 2, &templeMS_MEAS);            // ��Ϣ���ж��壬����ģ���߳�������ͨѶ�߳�
osMessageQId  MsgBox_MTtempleMS;
osMessageQDef(MsgBox_MTtempleMS, 2, &templeMS_MEAS);          // ��Ϣ���ж���,��������ͨѶ�߳���ģ���߳�
osMessageQId  MsgBox_DPtempleMS;
osMessageQDef(MsgBox_DPtempleMS, 2, &templeMS_MEAS);          // ��Ϣ���ж��壬����ģ���߳�����ʾģ���߳�

void templeMS_Init(void){

	;
}

void templeMS_Thread(const void *argument){

	;
}

void templeMSThread_Active(void){

	templeMS_Init();
	templeMS_pool   = osPoolCreate(osPool(templeMS_pool));	//�����ڴ��
	MsgBox_templeMS 	= osMessageCreate(osMessageQ(MsgBox_templeMS), NULL);	//������Ϣ����
	MsgBox_MTtempleMS = osMessageCreate(osMessageQ(MsgBox_MTtempleMS), NULL);//������Ϣ����
	MsgBox_DPtempleMS = osMessageCreate(osMessageQ(MsgBox_DPtempleMS), NULL);//������Ϣ����
	tid_templeMS_Thread = osThreadCreate(osThread(templeMS_Thread),NULL);
}
