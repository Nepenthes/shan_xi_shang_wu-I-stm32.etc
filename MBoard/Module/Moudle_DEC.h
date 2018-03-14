#ifndef	_MOUDLE_DEC_H_
#define	_MOUDLE_DEC_H_

#include "IO_Map.h"
#include "stm32f10x.h"

#include "stdio.h"
#include "string.h"
#include "Driver_USART.h"
#include "delay.h"

#include "infraTrans.h"
#include "Eguard.h"

#include "LCD_1.44.h"

#include "fireMS.h"
#include "gasMS.h"
#include "lightMS.h"
#include "pyroMS.h"
#include "analogMS.h"
#include "smokeMS.h"
#include "tempMS.h"

#include "pwmCM.h"
#include "curtainCM.h"
#include "sourceCM.h"
#include "speakCM.h"
#include "RelaysCM.h"

#include "debugUART.h"
#include <dataTrans_USART.h>

#define		MID_TRANS_Wifi		0x02
#define 	MID_TRANS_Zigbee	0x01
	
#define		MID_EXEC_DEVPWM		0x83
#define		MID_EXEC_CURTAIN	0x43
#define 	MID_EXEC_RELAYS		0x42
#define 	MID_EXEC_SOURCE		0x82
#define		MID_EXEC_SPEAK		0x41

#define 	MID_SENSOR_FIRE		0x01
#define 	MID_SENSOR_PYRO		0x02
#define 	MID_SENSOR_SMOKE	0x03
#define 	MID_SENSOR_GAS		0x04
#define 	MID_SENSOR_TEMP		0x05
#define 	MID_SENSOR_LIGHT	0x09
#define 	MID_SENSOR_ANALOG	0x08

#define 	MID_EXEC_DEVIFR		0x81
#define 	MID_EGUARD			0x0A

#define		extension_FLG	0x80	//��չģ���Ƿ����߱�־λ
#define		wireless_FLG	0x40	//����ͨѶģ���Ƿ����߱�־λ
#define		LCD4_3_FLG		0x20	//��ʾģ���Ƿ����߱�־λ

#define		EVTSIG_MDEC_EN	123		//ģ���ַ������ʹ���ź�

#define		extension_IF	(!PEin(7))
#define		wireless_IF		(((GPIO_ReadInputData(GPIOD) >> 3) & 0x1f) != 0x1f)   //����ģ�����߼��(������)
#define		LCD4_3_IF		(!PEin(4))

typedef struct M_attr{

	u8 Extension_ID;
	u8 Wirless_ID;
	
	u8 Alive;	//bit7:��չģ��,bit6:����ͨ��ģ��,bit5:��ʾģ��
}Moudle_attr;

extern osThreadId tid_MBDEC_Thread;
extern Moudle_attr Moudle_GTA;

void MoudleDEC_Init(void);

void MBDEC_Thread(const void *argument);

#endif
