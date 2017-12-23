#include "gasMS.h"

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_gasMS_Thread;
osThreadDef(gasMS_Thread,osPriorityNormal,1,512);
			 
osPoolId  gasMS_pool;								 
osPoolDef(gasMS_pool, 10, gasMS_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_gasMS;
osMessageQDef(MsgBox_gasMS, 2, &gasMS_MEAS);            // ��Ϣ���ж��壬����ģ���߳�������ͨѶ�߳�
osMessageQId  MsgBox_MTgasMS;
osMessageQDef(MsgBox_MTgasMS, 2, &gasMS_MEAS);          // ��Ϣ���ж���,��������ͨѶ�߳���ģ���߳�
osMessageQId  MsgBox_DPgasMS;
osMessageQDef(MsgBox_DPgasMS, 2, &gasMS_MEAS);          // ��Ϣ���ж��壬����ģ���߳�����ʾģ���߳�

void gasMS_Init(void){

	;
}

void gasMS_Thread(const void *argument){

	;
}

void gasMSThread_Active(void){

	gasMS_Init();
	gasMS_pool   = osPoolCreate(osPool(gasMS_pool));	//�����ڴ��
	MsgBox_gasMS 	= osMessageCreate(osMessageQ(MsgBox_gasMS), NULL);	//������Ϣ����
	MsgBox_MTgasMS = osMessageCreate(osMessageQ(MsgBox_MTgasMS), NULL);//������Ϣ����
	MsgBox_DPgasMS = osMessageCreate(osMessageQ(MsgBox_DPgasMS), NULL);//������Ϣ����
	tid_gasMS_Thread = osThreadCreate(osThread(gasMS_Thread),NULL);
}
