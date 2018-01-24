#include "intrrupt.h"

extern ARM_DRIVER_USART Driver_USART1;

void EXTI9_5_IRQHandler(void) {
	
	switch(Moudle_GTA.Extension_ID){
	
		case MID_EXEC_DEVIFR:{
			
				const uint8_t IFR_PER  = 2;		//��ƽ�����ֱ��� 2us
				const uint8_t BUF_WARN = 200;	//�����������ֵ
				
				const uint16_t SIGLEN_MAX = 50000 / IFR_PER;  //һ����ƽ�ź�ά���ʱ������
				const uint16_t SIGLEN_MIN = 6000 / IFR_PER;  //һ����ƽ�ź�ά�����ʱ������
				
				static uint8_t Tab_Hp,Tab_Lp;
				static uint16_t HT_Tab[Tab_size] = {0};
				static uint16_t LT_Tab[Tab_size] = {0};
				uint16_t time = 0;
				
				Tab_Hp = Tab_Lp = 0;
				
				if(measure_en && EXTI_GetITStatus(EXTI_Line6) == SET){
					
					Tab_Hp = Tab_Lp = 0;
					memset(HT_Tab, 0, Tab_size * sizeof(uint16_t));
					memset(LT_Tab, 0, Tab_size * sizeof(uint16_t));
					while(1){
					
						if(!PCin(6)){
						
							time = LW_ReceiveTime();
							
							if(time > SIGLEN_MIN && Tab_Lp > BUF_WARN){  //���治�������ڽز�����
							
								//Driver_USART1.Send(&Tab_Lp,1);   //�����ڸߵ�ƽ�ź��ܳ��������
								memcpy((uint16_t *)LTtab,LT_Tab,Tab_Lp * 2);	//��������Ϊ u16,��memcpy���ֽ�Ϊ��λ������2
								memcpy((uint16_t *)HTtab,HT_Tab,Tab_Hp * 2);
								tabLp = Tab_Lp;
								tabHp = Tab_Hp;
								break;
							}else{
								
								LT_Tab[Tab_Lp ++] = time;
							}
						}
						if(PCin(6)){
						
							time = HW_ReceiveTime();
								
							if((time > SIGLEN_MAX) || (time > SIGLEN_MIN && Tab_Hp > BUF_WARN)){
								
								//Driver_USART1.Send(&Tab_Hp,1);	//�����ڵ͵�ƽ�ź��ܳ��������
								memcpy((uint16_t *)LTtab,LT_Tab,Tab_Lp * 2);	//��������Ϊ u16,��memcpy���ֽ�Ϊ��λ������2
								memcpy((uint16_t *)HTtab,HT_Tab,Tab_Hp * 2);
								tabLp = Tab_Lp;
								tabHp = Tab_Hp;
								break;
							}else{

								HT_Tab[Tab_Hp ++] = time;
							}
						}
					}
					measure_en = false;
				}
			}break;

		default:break;
	}
	
	EXTI_ClearITPendingBit(EXTI_Line6);
	EXTI_ClearFlag(EXTI_Line6);
}

