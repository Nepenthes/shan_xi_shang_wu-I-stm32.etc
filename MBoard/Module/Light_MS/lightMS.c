#include "lightMS.h"

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_lightMS_Thread;
osThreadDef(lightMS_Thread,osPriorityNormal,1,512);
			 
osPoolId  lightMS_pool;								 
osPoolDef(lightMS_pool, 10, lightMS_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_lightMS;
osMessageQDef(MsgBox_lightMS, 2, &lightMS_MEAS);            // ��Ϣ���ж��壬����ģ���߳�������ͨѶ�߳�
osMessageQId  MsgBox_MTlightMS;
osMessageQDef(MsgBox_MTlightMS, 2, &lightMS_MEAS);          // ��Ϣ���ж���,��������ͨѶ�߳���ģ���߳�
osMessageQId  MsgBox_DPlightMS;
osMessageQDef(MsgBox_DPlightMS, 2, &lightMS_MEAS);          // ��Ϣ���ж��壬����ģ���߳�����ʾģ���߳�

void lightMS_Init(void){

	;
}

void lightMS_Thread(const void *argument){

	;
}

void lightMSThread_Active(void){

	lightMS_Init();
	lightMS_pool   = osPoolCreate(osPool(lightMS_pool));	//�����ڴ��
	MsgBox_lightMS 	= osMessageCreate(osMessageQ(MsgBox_lightMS), NULL);	//������Ϣ����
	MsgBox_MTlightMS = osMessageCreate(osMessageQ(MsgBox_MTlightMS), NULL);//������Ϣ����
	MsgBox_DPlightMS = osMessageCreate(osMessageQ(MsgBox_DPlightMS), NULL);//������Ϣ����
	tid_lightMS_Thread = osThreadCreate(osThread(lightMS_Thread),NULL);
}
