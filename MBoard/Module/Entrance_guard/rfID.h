#ifndef _RFID_H_
#define _RFID_H_

#include "Eguard.h"
#include "rc522_config.h"

#define	  RFID_EXERES_TTIT		0xDD

#define   macRC522_DELAY()  delay_us(100)

#define   macDummy_Data  0x00

void             PcdReset                   ( void );                       //��λ
void             M500PcdConfigISOType       ( u8 type );                    //������ʽ
char             PcdRequest                 ( u8 req_code, u8 * pTagType ); //Ѱ��
char             PcdAnticoll                ( u8 * pSnr);                   //������

void rfID_Thread(const void *argument);
void rfIDThread_Active(void);

#endif

