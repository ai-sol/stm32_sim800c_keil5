/**
  ******************************************************************************
  * @file    Demo/src/main.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    09/13/2010
  * @brief   Main program body
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32F10x.h"
#include "usart.h"
#include "usart3.h"
#include "delay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"
#include "SIM800_TP.h"
#include "device.h"
#include "queue.h"
#include "rtc.h"

////////////////////////ST官方程序框架需要的变量和函数///////////////////////////////////////////

/**********************程序中断优先级设置******************************************  
                PreemptionPriority        SubPriority
USART3                 0											0

DMA1                   1											0

TIM7									 2											0	

TIM6									 3											0

TIM4                   3                      1

************************************************************************************/

////////////////////////用户程序自定义的变量和函数///////////////////////////////////////////
void Reset_Device_Status(void)
{
	dev.is_login = FALSE;
	dev.hb_ready = FALSE;
	dev.hb_timer = 0;
	dev.wait_reply = FALSE;
	dev.reply_timer = 0;
	dev.need_reset = 0;
	dev.portClosed = 0;
	dev.msg_seq = 0;
}

int main(void)
{
	u8 i;
	char recv[100];
	char *uart_data_left;
	char *p, *p1;	
	u16 length = 0; 
	u8 sum = 0;
	u8 sum_msg = 0;	
	MsgSrv *msgSrv=NULL;
	//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 
	/* Enable GPIOx Clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	q = CycQueueInit();
	
	if(q == NULL)
	{
		//BSP_Printf("malloc error!\n");
	}
	
	delay_init();

#ifdef LOG_ENABLE	
	//注意串口1仅仅用来发送，接收的宏没有使能
	//#define EN_USART1_RX 			   0		//使能（1）/禁止（0）串口1接收
	usart1_init(115200);                            //串口1,Log
#endif
	
	usart3_init(115200);                            //串口3,对接SIM800/YR4G

	rtc_init();	
	Reset_Device_Status();
	Clear_Usart3();
	Device_Init();
	//Device_ON(DEVICE_01);
	//Device_ON(DEVICE_04);
	for(i=DEVICE_01; i<DEVICEn; i++)
	{
		BSP_Printf("Power[%d]: %d\n", i, Device_Power_Status(i));
	}
	
	SIM800_TP_POWER_ON();  
	SIM800_TP_PWRKEY_ON();
	BSP_Printf("SIM800 power up\r\n");
	
	while(!SIM800_TP_Link_Server())
	{
		BSP_Printf("INIT: SIM Module not working\r\n");
		dev.need_reset = ERR_INIT_MODULE;
	}

	BSP_Printf("SIM Connect to Network\r\n");
	
	lastInActivity = lastOutActivity = RTC_GetCounter();
	SendLogin();

	BSP_Printf("SIM Send Login\r\n");

#if defined(STM32F10X_MD_VL)
	TIM6_Int_Init(9999,2399);						     // 1s中断
#elif defined (STM32F10X_HD)
	TIM6_Int_Init(29999,2399);	
#endif
	TIM_SetCounter(TIM6,0); 
	TIM_Cmd(TIM6,ENABLE);
	
	while(1)
	{		
		while(isWorking())
		{	
			if(((lastInActivity>lastOutActivity)&&((lastInActivity-lastOutActivity)>DISCONNECT_TIMEOUT))
				|| ((lastOutActivity>lastInActivity)&&((lastOutActivity-lastInActivity)>DISCONNECT_TIMEOUT)))
			{
				BSP_Printf("lastInActivity: %d, lastOutActivity%d\n", lastInActivity, lastOutActivity);
				goto Restart;
			}
			
			if(!dev.is_login)
			{
				if(dev.reply_timer >= REPLY_1_MIN)
					SendLogin();  //is_login: login msg switch
			}
			else
			{
				if(dev.hb_ready)
				{
					SendHeart();
					dev.hb_ready = FALSE;
				}

				if(((dev.portClosed != 0) && !dev.wait_reply) ||(dev.wait_reply && (dev.reply_timer >= REPLY_1_MIN)))
				{
					SendFinish();
				}
			}
			
			if(DumpQueue(recv) != NULL)
			{
				if((strstr(recv,"CLOSED")!=NULL)||(strstr(recv,"DEACT")!=NULL))
				{
					goto Restart;
				}
				
				uart_data_left = (char *)recv;
				while((p=strstr(uart_data_left, MSG_STR_SERVER_HEADER))!=NULL)
				{
					if((p1=strstr((const char*)p,"#"))!=NULL)
					{
						//调用异或和函数来校验回文	
						length = p1 - p +1;
						//校验数据
						sum = CheckSum((char *)(p),length-5);
						BSP_Printf("sum:%d\r\n",sum);
						
						//取字符串中的校验值,校验值转化为数字，并打印
						sum_msg = atoi((const char *)(p+length-5));	
						BSP_Printf("sum_msg:%d\r\n",sum_msg);
						
						//回文正确
						if(sum == sum_msg)
						{
							msgSrv = (MsgSrv *)p;						
							u8 seq = atoi(msgSrv->seq);
							lastInActivity = RTC_GetCounter();
							BSP_Printf("[%d]: Recv[%d] Seq[%d] Dup[%d] from Server\n", lastInActivity, atoi(msgSrv->id), seq, atoi(msgSrv->dup));

							if(!dev.is_login)
							{
								if(atoi(msgSrv->id) == MSG_STR_ID_LOGIN)
								{
									uint32_t TimeVar=atoi(p+sizeof(MsgSrv));
									if((TimeVar > 0) && (TimeVar < TIMEVAL_MAX))
									{
										RTC_Configuration();
										Time_Adjust(TimeVar);
										lastInActivity = lastOutActivity = RTC_GetCounter();										
										dev.is_login = TRUE;
									}
								}

								break;
							}
								
							switch(atoi(msgSrv->id))
							{
								case MSG_STR_ID_OPEN:
								{
									dev.msg_seq_s = seq;
									
									char *interfaces, *periods;
									bool interface_on[DEVICEn]={FALSE};
									int period_on[DEVICEn]={0};
									//根据当前设备状态进行开启(GPIO)，已经开了的就不处理了
									//开启设备并本地计时
									interfaces = strtok(p+sizeof(MsgSrv), ",");
									if(interfaces)
									{						
										//BSP_Printf("ports: %s\n", interfaces);
									}
									for(i=DEVICE_01; i<DEVICEn; i++)
									{
										interface_on[i]=(interfaces[i]=='1')?TRUE:FALSE;
										if((interface_on[i]) && (g_device_status[i].seq == seq))
										{
											SendStartAck();
											break;
										}
									}
									
									periods = strtok(NULL, ",");
									if(periods)
									{						
										//BSP_Printf("periods: %s\n", periods);	
									}
									sscanf(periods, "%02d%02d%02d%02d,", &period_on[DEVICE_01], 
										&period_on[DEVICE_02], &period_on[DEVICE_03], &period_on[DEVICE_04]);

									for(i=DEVICE_01; i<DEVICEn; i++)
									{
										if(interface_on[i] && (g_device_status[i].power == OFF))
										{
											g_device_status[i].total = period_on[i] * NUMBER_TIMER_1_MINUTE;
											g_device_status[i].passed = 0;
											g_device_status[i].power = ON;	
											g_device_status[i].seq = seq;
											Device_ON(i);	
										}			
									}

									SendStartAck();
								}
								break;
								case MSG_STR_ID_CLOSE:
								{
									if(seq < dev.msg_seq)
										break;
									
									dev.portClosed = 0;
									dev.wait_reply = FALSE;
								}
								break;
								case MSG_STR_ID_HB:
								break;
								default:
								break;

							}
						}
						uart_data_left = p1;
					}
					else
						break;
				}		
			}
		}

Restart:
	#ifdef LOG_ENABLE	
		usart1_init(115200);
	#endif
	
		usart3_init(115200);

		Reset_Device_Status();
		Clear_Usart3();	

		SIM800_TP_Power_Restart();	
		while(!SIM800_TP_Link_Server())
		{
			BSP_Printf("INIT: SIM Module not working\r\n");
			dev.need_reset = ERR_INIT_MODULE;
		}

		SendLogin();	
	}
}


#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
