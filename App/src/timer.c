#include "timer.h"
#include "usart.h"
#include "usart2.h"
#include "usart3.h"
#include "string.h"  
#include "stdlib.h"  
#include "device.h"
#include "queue.h"
#include "YR_4G.h"
#include "rtc.h"

extern void Reset_Device_Status(u8 status);

void TIM5_IRQHandler(void)
{ 	
	if (TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET)
	{	 		
		TIM_ClearITPendingBit(TIM5, TIM_IT_Update);  //���TIM5�����жϱ�־    
		USART2_RX_STA|=1<<15;	//��ǽ������
		TIM_Cmd(TIM5, DISABLE);  //�ر�TIM5

		USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;	//��ӽ����� 
		
		//BSP_Printf("USART BUF:%s\r\n",USART2_RX_BUF);
		for(u8 i=0; i<(USART2_RX_STA&0x7FFF); i++)
			BSP_Printf("0x%02x ",USART2_RX_BUF[i]);
		BSP_Printf("\n");
		//Clear_Usart2();
	}

}

//��ʱ��6�жϷ������		    
void TIM6_IRQHandler(void)
{
	u8 index;
	if(TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET)					  //�Ǹ����ж�
	{	
		TIM_ClearITPendingBit(TIM6, TIM_IT_Update);  					//���TIM6�����жϱ�־
		//Time_Display(RTC_GetCounter());
		
		if(!dev.hb_ready)
		{
			if(dev.hb_timer >= HB_1_MIN)
			{				
				BSP_Printf("TIM6: HB Ready\r\n");
				dev.hb_ready = TRUE;
				dev.hb_timer = 0;
			}
			else
				dev.hb_timer++;
		}

		for(index=DEVICE_01; index<DEVICEn; index++)
		{
			switch(g_device_status[index].power)
			{
				case ON:
				{
					if(g_device_status[index].total==0)
					{
						BSP_Printf("Error: Dev[%d] %d %d %d\n", index, g_device_status[index].power, g_device_status[index].total, g_device_status[index].passed);
						g_device_status[index].power = UNKNOWN;
					}
					else
					{
						if(g_device_status[index].passed >= g_device_status[index].total)
						{
							BSP_Printf("Dev[%d]: %d %d %d\n", index, g_device_status[index].power, g_device_status[index].total, g_device_status[index].passed);					
							g_device_status[index].passed=g_device_status[index].total=0;
							g_device_status[index].power=OFF;
							Device_OFF(index);
							BSP_Printf("TIM6: �����豸״̬ΪCLOSE_DEVICE\r\n");
							dev.portClosed |= 1<<index;
							dev.wait_reply = FALSE;
						}
						else
							g_device_status[index].passed++;
					}
				}	
				break;
					
				case OFF:
					if(g_device_status[index].total!=0)
						g_device_status[index].power = UNKNOWN;
				break;
					
				case UNKNOWN:
				default:
				break;
			}
		}
	
		if(dev.wait_reply || !dev.is_login)
		{
			if(dev.reply_timer < REPLY_1_MIN)
				dev.reply_timer++;
		}
		
		TIM_SetCounter(TIM6,0); 
	}
}

//��ʱ��7�жϷ������		    
void TIM7_IRQHandler(void)
{ 	
	u16 i = 0; 

	if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)
	{	 		
		TIM_ClearITPendingBit(TIM7, TIM_IT_Update);  //���TIM7�����жϱ�־    
		USART3_RX_STA|=1<<15;	//��ǽ������
		TIM_Cmd(TIM7, DISABLE);  //�ر�TIM7
		
		//Way 1
		USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;	//��ӽ����� 

		//Way 2
		for(i=0; i<(USART3_RX_STA&0X7FFF); i++)
			CycQueueIn(q,USART3_RX_BUF[i]);
		
		BSP_Printf("USART BUF:%s\r\n",USART3_RX_BUF);
		Clear_Usart3();
	}

}

void TIM5_Int_Init(u16 arr,u16 psc)
{	
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);//TIM5ʱ��ʹ��    
	
	//��ʱ��TIM5��ʼ��
	TIM_TimeBaseStructure.TIM_Period = arr;                     //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler =psc;                   //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);             //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
 
	TIM_ITConfig(TIM5,TIM_IT_Update,ENABLE );                   //ʹ��ָ����TIM5�ж�,��������ж�
	
	TIM_Cmd(TIM5,ENABLE);//������ʱ��5
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2 ;	//��ռ���ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		    	//�����ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			      	//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
	
}

//ͨ�ö�ʱ��6�жϳ�ʼ��
//����ѡ��ΪAPB1��1������APB1Ϊ72M
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//��ʱ�����ʱ����㷽��:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=��ʱ������Ƶ��,��λ:Mhz 
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��	
void TIM6_Int_Init(u16 arr,u16 psc)
{	
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);				//TIM6ʱ��ʹ��    
	
	//��ʱ��TIM6��ʼ��
	TIM_TimeBaseStructure.TIM_Period = arr;                     //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler =psc;                   //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);             //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
 
	TIM_ITConfig(TIM6,TIM_IT_Update,ENABLE );                   //ʹ��ָ����TIM6�ж�,��������ж�
	
	//TIM_Cmd(TIM6,ENABLE);//������ʱ��6
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM6_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2 ;//��ռ���ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
	
}

//TIMER7�ĳ�ʼ�� ����USART3���Խ�SIM800�����жϽ��ճ���/////////
//ͨ�ö�ʱ��7�жϳ�ʼ��
//����ѡ��ΪAPB1��1������APB1Ϊ24M
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//��ʱ�����ʱ����㷽��:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=��ʱ������Ƶ��,��λ:Mhz 
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��		 
void TIM7_Int_Init(u16 arr,u16 psc)
{	
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);//TIM7ʱ��ʹ��    
	
	//��ʱ��TIM7��ʼ��
	TIM_TimeBaseStructure.TIM_Period = arr;                     //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler =psc;                   //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure);             //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
 
	TIM_ITConfig(TIM7,TIM_IT_Update,ENABLE );                   //ʹ��ָ����TIM7�ж�,��������ж�
	
	TIM_Cmd(TIM7,ENABLE);//������ʱ��7
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2 ;	//��ռ���ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		    	//�����ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			      	//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
	
}

