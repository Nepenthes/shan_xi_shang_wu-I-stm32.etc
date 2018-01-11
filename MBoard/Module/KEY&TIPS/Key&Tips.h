#ifndef KEY$TIPS_H
#define KEY$TIPS_H

#define osObjectsPublic                     // define objects in main module

#include "stm32f10x.h"
#include "stdint.h"
#include "osObjects.h"                      // RTOS object definitions
#include "Driver_USART.h"

#include "pwmCM.h"
#include "infraTrans.h"

#define KEY_DEBUG_ON		0x00000004		//��������������Ϣ����źţ�����1����������Ϣ��
#define KEY_DEBUG_OFF		0x00000005		//�رհ���������Ϣ����źţ�����1����������Ϣ��

#define K1	GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_11)		//����1���
#define K2	GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_12)		//����2���

#define KEY_TICK					20			 // �������� / (ms)

#define KEY_COMFIRM					3			 // ����������������ֶ�ÿ�ʼ������ȷ��ʱ�� / ����KEY_LONG_PERIOD * KEY_TICK(ms)

#define KEY_LONG_PERIOD				150		 // ����ʱ������ / ����KEY_TICK(ms) ������8λ ���ֵ254
#define KEY_KEEP_PERIOD				40			 // �������ּ�� / ����KEY_TICK(ms)
#define KEY_CONTINUE_PERIOD			40		*1000000		// ������� / (us)

#define KEY_VALUE_0		0x0000		//������ֵ0
#define KEY_VALUE_1		0x0010		//������ֵ1
#define KEY_VALUE_2		0x0020		//������ֵ2
#define KEY_VALUE_3		0x0030		//������ֵ3
#define KEY_VALUE_4		0x0040		//������ֵ4
#define KEY_VALUE_5		0x0050		//������ֵ5
#define KEY_VALUE_6		0x0060		//������ֵ6
#define KEY_VALUE_7		0x0070		//������ֵ7
#define KEY_VALUE_8		0x0080		//������ֵ8
#define KEY_VALUE_9		0x0090		//������ֵ9
#define KEY_VALUE_10	0x00A0		//������ֵ10
#define KEY_VALUE_11	0x00B0		//������ֵ11
#define KEY_VALUE_12	0x00C0		//������ֵ12
#define KEY_VALUE_13	0x00D0		//������ֵ13
#define KEY_VALUE_14	0x00E0		//������ֵ14
#define KEY_VALUE_15	0x00F0		//������ֵ15

#define KEY_DOWN		0x1000		//����״̬������
#define KEY_CONTINUE	0x2000		//����״̬����������
#define KEY_LONG		0x3000		//����״̬����������
#define KEY_KEEP		0x4000		//����״̬�����������󱣳�
#define KEY_UP			0x5000		//����״̬����������
#define KEY_NULL		0x6000		//����״̬���ް����¼�
#define KEY_CTOVER		0x7000		//����״̬����������

#define KEY_STATE_INIT		0x0100	//���״̬��״̬����ʼ��
#define KEY_STATE_WOBBLE	0x0200	//���״̬��״̬���������
#define KEY_STATE_PRESS		0x0300	//���״̬��״̬�������̰����
#define KEY_STATE_CONTINUE	0x0400	//���״̬��״̬�������������	���ϣ���KEY_STATE_RECORD���
#define KEY_STATE_LONG		0x0500	//���״̬��״̬�������������
#define KEY_STATE_KEEP		0x0600	//���״̬��״̬�����������󱣳ּ��
#define KEY_STATE_RELEASE	0x0700	//���״̬��״̬�������ͷż��

#define KEY_STATE_RECORD	0x0800	//���״̬��״̬��������¼������ȷ�ϼ��

#define KEY_OVER_SHORT		0x01
#define KEY_OVER_LONG		0x02
#define KEY_OVER_KEEP		0x03

typedef void (* funEventKey)(void);

typedef void (* funKeyInit)(void);
typedef uint16_t (*funKeyScan)(void);

typedef struct keyStatus {

    uint8_t 	keyOverFlg;			//�����¼�������־
    uint16_t	sKeyCount;			//�����̰�	 ����ֵ
    uint16_t	sKeyKeep;			//���������� ����ֵ
    uint8_t	keyCTflg;			//����������־
} Obj_keyStatus;

typedef struct keyEvent {		//����һ����������̶߳����װ

    funKeyInit keyInit;			//�������ų�ʼ��
    funKeyScan keyScan;			//������ֵ��״̬ɨ��
    /*��Ҫ�ĸ��¼��ʹ�����Ӧ�������и�ֵ����*/
    funEventKey funKeyLONG[16];	//�����¼�
    funEventKey funKeySHORT[16];	//�̰��¼�
    funEventKey funKeyCONTINUE[16][8];	//���������¼�
    funEventKey funKeyKEEP[16][8];	//�����¼�
} Obj_eventKey;

//typedef struct kmBAttr{

//	funKeyInit 		key_Init;
//	Obj_keyStatus 	*orgKeyStatus;
//	funKeyScan 		key_Scan;
//	Obj_eventKey 	keyEvent;
//	const char 		*Tips_head;
//}keyMBAttr;

void eventK23(void);
void eventK24(void);
void eventK25(void);

void key_Thread(	funKeyInit 		key_Init,		
					Obj_keyStatus 	*orgKeyStatus,
					funKeyScan 		key_Scan,		
					Obj_eventKey 	keyEvent,		
					const char 		*Tips_head	);

void keyInit(void);
void keyMboard_Thread(const void *argument);
void keyMboardActive(void);

#endif
