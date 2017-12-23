#include "debugUart.h"

const uint8_t TestCMD_PAGSIZE = 20;
const uint8_t TestCMD_MAXSIZE = 80;

const char *TestCMD[TestCMD_PAGSIZE] = {

	"��ʾ�װ尴��������Ϣ",
	"���صװ尴��������Ϣ",
	"��ʾ����ת����չ�尴��������Ϣ",
	"���غ���ת����չ�尴��������Ϣ",
};

const char *TestREP[TestCMD_PAGSIZE] = {

	"�װ尴��������Ϣ�ѹ�ʾ\r\n",
	"�װ尴��������Ϣ������\r\n",
	"����ת����չ�尴��������Ϣ�ѹ�ʾ\r\n",
	"����ת����չ�尴��������Ϣ������\r\n",
};

funDebug funDebugTab[] = {funDB_keyMB_ON,funDB_keyMB_OFF,funDB_keyIFR_ON,funDB_keyIFR_OFF};

extern ARM_DRIVER_USART Driver_USART1;

extern osThreadId tid_keyMboard_Thread;	//�������尴������ID�����ڴ�����Ϣ����ʹ���ź�
extern osThreadId tid_keyIFR_Thread;	//��������ת����չ�尴������ID�����ڴ�����Ϣ����ʹ���ź�

osThreadId tid_USARTDebug_Thread;
osThreadDef(USARTDebug_Thread,osPriorityNormal,1,1024);

void funDB_keyMB_ON(void){  // �������尴��Debug_log

	osSignalSet (tid_keyMboard_Thread, KEY_DEBUG_ON);
}

void funDB_keyMB_OFF(void){ // �ر����尴��Debug_log

	osSignalSet (tid_keyMboard_Thread, KEY_DEBUG_OFF);
}

void funDB_keyIFR_ON(void){  // ��������ת����չ�尴��Debug_log

	osSignalSet (tid_keyIFR_Thread, KEY_DEBUG_ON);
}

void funDB_keyIFR_OFF(void){ // �رպ���ת����չ�尴��Debug_log

	osSignalSet (tid_keyIFR_Thread, KEY_DEBUG_OFF);
}

void USART1Debug_Init(void){

	/*Initialize the USART driver */
	Driver_USART1.Initialize(myUSART1_callback);
	/*Power up the USART peripheral */
	Driver_USART1.PowerControl(ARM_POWER_FULL);
	/*Configure the USART to 4800 Bits/sec */
	Driver_USART1.Control(ARM_USART_MODE_ASYNCHRONOUS |
									ARM_USART_DATA_BITS_8 |
									ARM_USART_PARITY_NONE |
									ARM_USART_STOP_BITS_1 |
							ARM_USART_FLOW_CONTROL_NONE, 115200);

	/* Enable Receiver and Transmitter lines */
	Driver_USART1.Control (ARM_USART_CONTROL_TX, 1);
	Driver_USART1.Control (ARM_USART_CONTROL_RX, 1);

	Driver_USART1.Send("i'm usart1 for debug log\r\n", 26);
}

void myUSART1_callback(uint32_t event)
{
//  uint32_t mask;
//  mask = ARM_USART_EVENT_RECEIVE_COMPLETE  |
//         ARM_USART_EVENT_TRANSFER_COMPLETE |
//         ARM_USART_EVENT_SEND_COMPLETE     |
//         ARM_USART_EVENT_TX_COMPLETE       ;
//  if (event & mask) {
////    /* Success: Wakeup Thread */
////    osSignalSet(tid_myUART_Thread, 0x01);
//  }
//  if (event & ARM_USART_EVENT_RX_TIMEOUT) {
//    __breakpoint(0);  /* Error: Call debugger or replace with custom error handling */
//  }
//  if (event & (ARM_USART_EVENT_RX_OVERFLOW | ARM_USART_EVENT_TX_UNDERFLOW)) {
//    __breakpoint(0);  /* Error: Call debugger or replace with custom error handling */
//  }
}

void USARTDebug_Thread(const void *argument){

	const uint8_t cmdsize = TestCMD_MAXSIZE;
	char cmd[cmdsize] = "abc";
	uint8_t loop;
	
	osSignalWait(USARTDEBUG_THREAD_EN,osWaitForever);		//�ȴ��߳�ʹ���ź�
	osSignalClear(tid_USARTDebug_Thread,USARTDEBUG_THREAD_EN);	
	
	for(;;){

		osDelay(10);													//������ʱ��������
		Driver_USART1.Receive(cmd,TestCMD_MAXSIZE);
		
		for(loop = 0; loop < 4; loop ++){
		
			if(strstr(cmd,TestCMD[loop])){							//�Ӵ��Ƚ�
			//if(!strcmp(TestCMD[loop],cmd)){						//ȫ�ȱȽ�
			
				osDelay(10);
					
				funDebugTab[loop]();
				Driver_USART1.Send((char*)(TestREP[loop]),strlen((char*)(TestREP[loop])));
				memset(cmd,0,cmdsize*sizeof(char));
			}
		}
	}
}

void USART_debugInit(void){

	USART1Debug_Init();
}

void debugThread_Active(void){
	
	USART_debugInit();
	tid_USARTDebug_Thread = osThreadCreate(osThread(USARTDebug_Thread),NULL);
}

