#ifndef __STMFLASH_H__
#define __STMFLASH_H__

#include "IO_Map.h"  
#include "stm32f10x.h"
#include "stm32f10x_flash.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//�û������Լ�����Ҫ����
#define STM32_FLASH_SIZE 256 	 			 //��ѡSTM32��FLASH������С(��λΪK)
#define STM32_FLASH_WREN 1              //ʹ��FLASHд��(0��������;1��ʹ��)
//////////////////////////////////////////////////////////////////////////////////////////////////////

//FLASH��ʼ��ַ
#define  STM32_FLASH_BASE         0x08030000 	//STM32 FLASH����ʼ��ַ

#define  MODULE_IFRid_DATADDR	  	  0x0803F010
#define	 MODULE_IFRdatsLpn_DATADDR	  0x0803F100
#define	 MODULE_IFRdatsHpn_DATADDR	  0x0803F300
#define  MODULE_IFRdatsLp_DATADDR	  0x0803E000
#define  MODULE_IFRdatsHp_DATADDR	  0x0803D000

#define  MODULE_PWMid_DATADDR		  0x0803C010

//FLASH������ֵ

u16 STMFLASH_ReadHalfWord(u32 faddr);		  //��������  
void STMFLASH_WriteLenByte(u32 WriteAddr,u32 DataToWrite,u16 Len);	//ָ����ַ��ʼд��ָ�����ȵ�����
u32 STMFLASH_ReadLenByte(u32 ReadAddr,u16 Len);								//ָ����ַ��ʼ��ȡָ����������
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite);		//��ָ����ַ��ʼд��ָ�����ȵ�����
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead);   		//��ָ����ַ��ʼ����ָ�����ȵ�����

//����д��
void Test_Write(u32 WriteAddr,u16 WriteData);								   
#endif

















