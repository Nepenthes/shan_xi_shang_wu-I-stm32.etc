#include "simuMS.h"

extern ARM_DRIVER_USART Driver_USART1;		//�豸�����⴮��һ�豸����

osThreadId tid_simuMS_Thread;
osThreadDef(simuMS_Thread,osPriorityNormal,1,512);
			 
osPoolId  simuMS_pool;								 
osPoolDef(simuMS_pool, 10, simuMS_MEAS);                  // �ڴ�ض���
osMessageQId  MsgBox_simuMS;
osMessageQDef(MsgBox_simuMS, 2, &simuMS_MEAS);            // ��Ϣ���ж��壬����ģ���߳�������ͨѶ�߳�
osMessageQId  MsgBox_MTsimuMS;
osMessageQDef(MsgBox_MTsimuMS, 2, &simuMS_MEAS);          // ��Ϣ���ж���,��������ͨѶ�߳���ģ���߳�
osMessageQId  MsgBox_DPsimuMS;
osMessageQDef(MsgBox_DPsimuMS, 2, &simuMS_MEAS);          // ��Ϣ���ж��壬����ģ���߳�����ʾģ���߳�

void simuMS_Init(void){

	;
}

void simuMS_Thread(const void *argument){

	;
}

void simuMSThread_Active(void){

	simuMS_Init();
	simuMS_pool   = osPoolCreate(osPool(simuMS_pool));	//�����ڴ��
	MsgBox_simuMS 	= osMessageCreate(osMessageQ(MsgBox_simuMS), NULL);	//������Ϣ����
	MsgBox_MTsimuMS = osMessageCreate(osMessageQ(MsgBox_MTsimuMS), NULL);//������Ϣ����
	MsgBox_DPsimuMS = osMessageCreate(osMessageQ(MsgBox_DPsimuMS), NULL);//������Ϣ����
	tid_simuMS_Thread = osThreadCreate(osThread(simuMS_Thread),NULL);
}
