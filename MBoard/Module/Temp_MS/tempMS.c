#include "tempMS.h"

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_tempMS_Thread;
osThreadDef(tempMS_Thread,osPriorityNormal,1,512);
			 
osPoolId  tempMS_pool;								 
osPoolDef(tempMS_pool, 10, tempMS_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_tempMS;
osMessageQDef(MsgBox_tempMS, 2, &tempMS_MEAS);            // ��Ϣ���ж��壬����ģ���߳�������ͨѶ�߳�
osMessageQId  MsgBox_MTtempMS;
osMessageQDef(MsgBox_MTtempMS, 2, &tempMS_MEAS);          // ��Ϣ���ж���,��������ͨѶ�߳���ģ���߳�
osMessageQId  MsgBox_DPtempMS;
osMessageQDef(MsgBox_DPtempMS, 2, &tempMS_MEAS);          // ��Ϣ���ж��壬����ģ���߳�����ʾģ���߳�

void tempMS_Init(void){

	;
}

void tempMS_Thread(const void *argument){

	;
}

void tempMSThread_Active(void){

	tempMS_Init();
	tempMS_pool   = osPoolCreate(osPool(tempMS_pool));	//�����ڴ��
	MsgBox_tempMS 	= osMessageCreate(osMessageQ(MsgBox_tempMS), NULL);	//������Ϣ����
	MsgBox_MTtempMS = osMessageCreate(osMessageQ(MsgBox_MTtempMS), NULL);//������Ϣ����
	MsgBox_DPtempMS = osMessageCreate(osMessageQ(MsgBox_DPtempMS), NULL);//������Ϣ����
	tid_tempMS_Thread = osThreadCreate(osThread(tempMS_Thread),NULL);
}
