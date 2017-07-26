#include "yr_4g.h"	 	 	
#include "string.h"  
#include "stdio.h"	
#include "usart.h" 
#include "usart3.h" 
#include "timer.h"
#include "delay.h"
#include <stdlib.h>
#include "queue.h"

enum{
	AT=0,
	AT_Z,
	AT_REBOOT,
	AT_E,
	AT_ENTM,
	AT_WKMOD,
	AT_CMDPW,
	AT_STMSG,
	AT_RSTIM,
	AT_CSQ,
	AT_SYSINFO,
	AT_RELD,
	AT_CLEAR,
	AT_CFGTF,
	AT_VER,
	AT_SN,
	AT_ICCID,
	AT_IMEI,
	
	AT_MAX,
};

typedef struct
{
	char *cmd;
	char *ack;
}AtPair;

AtPair atpair[]=
{
	{"", "OK"},
	{"+Z", "OK"},	
	{"+REBOOT", "OK"},	
	{"+E", "+E:"},	
	{"+ENTM", "OK"},
	{"+WKMOD", "+WKMOD:"},
	{"+CMDPW", "+CMDPW:"},	
	{"+RSTIM", "+RSTIM:"},	
	{"+CSQ", "+CSQ:"},	
	{"+SYSINFO", "+SYSINFO:"},		
	{"+RELD", "+RELD:"},
	{"+CLEAR", "OK"},		
	{"+CFGIF", "OK"},	
	{"+VER", "+VER:"},		
	{"+SN", "+SN:"},
	{"+ICCID", "+ICCID:"},	
	{"+IMEI", "+IMEI:"},
	{NULL, NULL},
};
	
#define COUNT_AT 3
#define RETRY_AT 3

u8 mode = 0;				//0,TCP����;1,UDP����
const char *modetbl[2] = {"TCP","UDP"};//����ģʽ

//const char  *ipaddr = "tuzihog.oicp.net";
//const char  *port = "28106";

//const char  *ipaddr = "42.159.117.91";
const char  *ipaddr = "116.62.187.167";
//const char  *ipaddr = "42.159.107.250";
const char  *port = "8090";
const char delim=',';
const char ending='#';
const char *Error="Err";

char version[LENGTH_VERSION_BUF] = {0};
char sysinfo[LENGTH_SYSINFO_BUF] = {0};
char iccid[LENGTH_ICCID_BUF] = {0};

t_DEV dev={0};
extern void Reset_Device_Status(u8 status);

const char *msg_id[MSG_STR_ID_MAX]={"TRVAP00", "TRVAP01", "TRVAP03", "TRVAP05"};
const char *msg_id_s[MSG_STR_ID_MAX]={"TRVBP00", "TRVBP01", "TRVBP03", "TRVBP05"};
const char *msg_device="000";





const char *passcode="usr.cn";
char *DumpQueue(char * recv)
{
	u16 i=0;
	char *p=recv;
	if(CycQueueIsEmpty(q) == 0 )
	{
		while(CycQueueIsEmpty(q) == 0 ) // CycQueueIsEmpty() return true when queue is empty.
		{
			uint8_t ch = CycQueueOut(q);
			recv[i++] = ch;	
		}
		recv[i]=0;
	}
	else
		p=NULL;

	return p;
}

//waittime:�ȴ�ʱ��(��λ:10ms)
bool YR4G_Send_Cmd(char *cmd,char *ack,char *recv,u16 waittime)
{
	bool ret = FALSE; 
	char cmd_str[100]={0};
	
	sprintf(cmd_str, "%s%s%s", passcode, "AT", cmd);
	u3_printf("%s\r\n",cmd_str);//��������

	if(ack&&waittime)		//��Ҫ�ȴ�Ӧ��
	{
		while(waittime!=0)	//�ȴ�����ʱ
		{ 
			delay_ms(10);
			if(DumpQueue(recv) != NULL)
			{
				if(strstr(recv, ack))
				{
					ret = TRUE;
					break;
				}
				//No need bad cmd check?
				//if(strstr(recv, Error))
				{
					//ret = FALSE;
					//break;
				}					
			}
			waittime--;	
		}
	}

	return ret;
} 

bool CheckModule(void)
{
	u8 retry = RETRY_AT;
	u8 id=AT;
	char recv[50];		
	bool ret = FALSE;
	while(retry != 0)
	{
		if(ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100)) 
			break;
		delay_ms(2000);
		retry--;
	}
	
	return ret;
	
}

bool GetVersion(void)
{
	u8 retry = RETRY_AT;
	u8 id=AT_VER;
	char recv[50];	
	bool ret = FALSE;
	while(retry != 0)
	{
		if(ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100)) 
		{
			memset(version, 0, sizeof(version));
			strcpy(version, strstr(recv, atpair[id].ack)+strlen(atpair[id].ack));
			BSP_Printf("Version: %s\n", version);
			break;
		}
		delay_ms(2000);
		retry--;
	}
	
	return ret;
	
}

bool CheckSysinfo(void)
{
	u8 retry = RETRY_AT;
	u8 id=AT_SYSINFO;
	char recv[50];	
	bool ret = FALSE;
	while(retry != 0)
	{
		if(ret = YR4G_Send_Cmd(atpair[id].cmd,atpair[id].ack,recv,100)) 
		{
			memset(version, 0, sizeof(version));
			strcpy(version, strstr(recv, atpair[id].ack)+strlen(atpair[id].ack));
			BSP_Printf("Version: %s\n", version);
			break;
		}
		delay_ms(2000);
		retry--;
	}
	
	return ret;
	
	
}

//�鿴SIM�Ƿ���ȷ��⵽
u8 CheckSIMCard(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;

	delay_ms(10000);	
	while(count != 0)
	{
		ret = YR4G_Send_Cmd("AT+ICCID?","+ICCID",1000);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}

	//Clear_Usart3();
	delay_ms(2000);	
	return ret;
}

u8 CheckOPS(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = YR4G_Send_Cmd("AT+COPS?","CHINA",500);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	return ret;
}


//�鿴��������
u8 CheckCSQ(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	u8 *p1 = NULL; 
	u8 *p2 = NULL;
	u8 p[50] = {0}; 
  	u8 signal=0;

	while(signal < 5)
	{
		delay_ms(2000);
		while(count != 0)
		{
			ret = YR4G_Send_Cmd("AT+CSQ","+CSQ:",200);
			if(ret == CMD_ACK_NONE)
			{
				delay_ms(2000);
			}
			else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
				break;
			
			count--;
		}		
		
		if(ret == CMD_ACK_OK)
		{
			//ATָ���Ѿ�ָ����ɣ�����Է���ֵ���д���
			p1=(u8*)strstr((const char*)(dev.usart_data),":");
			p2=(u8*)strstr((const char*)(p1),",");
			p2[0]=0;//���������
			signal = atoi((const char *)(p1+2));
			//sprintf((char*)p,"�ź�����:%s",p1+2);
			sprintf((char*)p,"�ź�����:%d",signal);
			BSP_Printf("%s\r\n",p);
		}
		//ATָ��Ļ����Ѿ�������ɣ�����
	}	
	return ret;
}

//��ȡSIM����ICCID
//SIM����ICCID,ȫ��Ψһ�ԣ���������PCB�����ID
//��ӡUSART3_RX_BUF�ă��� �{ԇ��;
		/*****  ע��+��ǰ���������ո�
  +CCID: 1,"898602B8191650216485"

		OK
		****/
//���������û������ȷ��....
u8 GetICCID(void)
{
	u8 index = 0;
	char *p_temp = NULL;
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	
	while(count != 0)
	{
		ret = YR4G_Send_Cmd("AT+CCID","OK",200);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if(ret == CMD_ACK_OK)
		{
			if(strstr(dev.usart_data, "AT+CCID")==NULL)
				break;
			else
				Disable_Echo();
		}
		else if(ret == CMD_ACK_DISCONN)
			break;
		
		count--;
	}

	if(ret == CMD_ACK_OK)	
	{
		//ATָ���Ѿ�ָ����ɣ�����Է���ֵ���д���
		p_temp = dev.usart_data;
		memset(ICCID_BUF, 0, sizeof(ICCID_BUF));
		//��ȡICCID��Ϣ��ȫ�ֱ���ICCID_BUF
		for(index = 0;index < LENGTH_ICCID_BUF;index++)
		{
			ICCID_BUF[index] = *(p_temp+OFFSET_ICCID+index);
		}
		BSP_Printf("ICCID_BUF:%s\r\n",ICCID_BUF);
	}
	//ATָ��Ļ����Ѿ�������ɣ�����
	//Clear_Usart3();
	return ret;
}

u8 YR4G_GPRS_ON(void)
{
	u8 ret = CMD_ACK_NONE;	
	if((ret = Link_Server_AT(0, ipaddr, port)) == CMD_ACK_OK)
		dev.need_reset = ERR_NONE;
	
	//Clear_Usart3();	
	return ret;

}

//�ر�GPRS������
u8 YR4G_GPRS_OFF(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	char recv[50];		
	while(count != 0)
	{
		ret = YR4G_Send_Cmd("AT+CIPCLOSE=1","CLOSE OK",recv,500);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}

	Reset_Device_Status(CMD_NONE);	
	//Clear_Usart3();	
	return ret;
}

//����GPRS
u8 YR4G_GPRS_Adhere(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	char recv[50];			
	while(count != 0)
	{
		ret = YR4G_Send_Cmd("AT+CGATT=1","OK",recv,1000);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	delay_ms(2000);
	return ret;
}

//����ΪGPRS����ģʽ
u8 YR4G_GPRS_Set(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	char recv[50];			
	while(count != 0)
	{
		ret = YR4G_Send_Cmd("AT+CIPCSGP=1,\"CMNET\"","OK",recv,600);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	delay_ms(2000);	
	return ret;
}

//���ý���������ʾIPͷ(�����ж�������Դ)	
u8 YR4G_GPRS_Dispaly_IP(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	char recv[50];			
	while(count != 0)
	{
		ret = YR4G_Send_Cmd("AT+CIPHEAD=1","OK",recv,300);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	return ret;
}

//�ر��ƶ����� 
u8 YR4G_GPRS_CIPSHUT(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	char recv[50];			
	while(count != 0)
	{
		ret = YR4G_Send_Cmd("AT+CIPSHUT","SHUT OK",recv,1000);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	delay_ms(2000);	
	return ret;
}

//����GPRS�ƶ�̨���ΪB,֧�ְ����������ݽ��� 
u8 YR4G_GPRS_CGCLASS(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	char recv[50];			
	while(count != 0)
	{
		ret = YR4G_Send_Cmd("AT+CGCLASS=\"B\"","OK",recv,300);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	delay_ms(2000);	
	return ret;
}


//����PDP������,��������Э��,��������Ϣ
u8 YR4G_GPRS_CGDCONT(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	char recv[50];			
	while(count != 0)
	{
		ret = YR4G_Send_Cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"","OK",recv,600);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	//Clear_Usart3();	
	delay_ms(2000);	
	return ret;
}

u8 Link_Server_AT(u8 mode,const char* ipaddr,const char *port)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	u8 p[100]={0};
	char recv[50];			
	if(mode)
		;
	else 
		;
		
  	sprintf((char*)p,"AT+CIPSTART=\"%s\",\"%s\",\"%s\"",modetbl[mode],ipaddr,port);	

	//��������
	//AT+CIPSTARTָ����ܵĻ����ǣ�CONNECT OK ��ALREADY CONNECT��CONNECT FAIL
	//������ȡ���ֿ��ܻ��ĵĹ�����������Ϊ�жϸ�ָ������ȷ���ĵ�����
	while(count != 0)
	{
		ret = YR4G_Send_Cmd(p,"CONNECT",recv,15000);
		if(ret == CMD_ACK_NONE)
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
		
		ret = YR4G_Send_Cmd("AT+CIPSTATUS","OK",recv,500);
		if(ret == CMD_ACK_OK)
		{
			if(strstr((const char*)(dev.usart_data),"CONNECT OK") != NULL)
				return ret;
			if(strstr((const char*)(dev.usart_data),"CLOSED") != NULL)
			{
				ret = YR4G_Send_Cmd("AT+CIPCLOSE=1","CLOSE OK",recv,500);
				ret = YR4G_Send_Cmd("AT+CIPSHUT","SHUT OK",recv,500);
			}
		}
	}
		
	return ret;
}

u8 Send_Data_To_Server(char* data)
{
	u8 ret = CMD_ACK_NONE;
	char recv[50];		
	u3_printf(data)
	
	return ret;
}

#if 0
u8 Check_Link_Status(void)
{
	u8 count = 0;

	while(YR4G_Send_Cmd("AT+CMSTATE","CONNECTED",500))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
//ATָ���Ѿ�������COUNT_AT�Σ���Ȼʧ�ܣ��ϵ�YR4G������TIME_AT���ӵĶ�ʱ����ʱʱ�䵽���ٴ����ӷ�����
//Ŀǰ������û�е��ñ�������Ҳû�ж�Flag_TIM6_2_S���ж����룬������ʱ���ε�Flag_TIM6_2_S�ĸ�ֵ
			//Flag_TIM6_2_S = 0xAA;
			return 1;		
		}
	}	

		//ATָ��Ļ��Ĳ���Ҫ��������
	Clear_Usart3();
	return 0;

}
#endif

char *YR4G_SMS_Create(char *sms_data, char *raw)
{
	sprintf((char*)sms_data,"Reset Type: %d, Dev Status: %d, Msg expect: %d, Msg recv: %d, HB: %d, HB TIMER: %d, Msg TIMEOUT: %d, Msg: \"%s\", AT-ACK: %s\r\n", dev.need_reset, 
		dev.status, dev.msg_expect, dev.msg_recv, dev.hb_count, dev.hb_timer, dev.msg_timeout, raw, dev.atcmd_ack); 
	return sms_data;
}

//����2Gģ��ĵ�ԴоƬ��������ͣ��ť��ʹ��
void YR4G_POWER_ON(void)
{
	u8 i= 0;
	
	GPIO_InitTypeDef  GPIO_InitStructure;
 	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 ;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
	GPIO_Init(GPIOB, &GPIO_InitStructure);	

	GPIO_SetBits(GPIOB,GPIO_Pin_8);	

	for(i = 0; i < 5; i++)
	{
		delay_ms(1000);	
	}
}

//�ر�2Gģ��ĵ�ԴоƬ��������ͣ��ť��ʹ��
void YR4G_POWER_OFF(void)
{
	u8 i= 0;

	GPIO_InitTypeDef  GPIO_InitStructure;
 	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 ;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
	GPIO_Init(GPIOB, &GPIO_InitStructure);	
	


	//��ԴоƬ��ʧ��
	GPIO_ResetBits(GPIOB,GPIO_Pin_8);	

	for(i = 0; i < 5; i++)
	{
		delay_ms(1000);	
	}

}


//ͨ��2Gģ���PWRKEY��ʵ�ֿ��ػ�
void YR4G_PWRKEY_ON(void)
{
	u8 i= 0;
	
	GPIO_InitTypeDef  GPIO_InitStructure;
 	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 ;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
	GPIO_Init(GPIOB, &GPIO_InitStructure);	

	//PWRKEY��ʹ��
	GPIO_SetBits(GPIOB,GPIO_Pin_9);	

	for(i = 0; i < 2; i++)
	{
		delay_ms(1000);	
	}
	//�������������ͷ�
	GPIO_ResetBits(GPIOB,GPIO_Pin_9);
	for(i = 0; i < 2; i++)
	{
		delay_ms(1000);	
	}
	dev.msg_recv = 0;	
	dev.need_reset = ERR_NONE;
	Reset_Device_Status(CMD_NONE);
	Clear_Usart3();
}

//ͨ��2Gģ���PWRKEY��ʵ�ֿ��ػ�
void YR4G_PWRKEY_OFF(void)
{
	u8 i= 0;
	
	GPIO_InitTypeDef  GPIO_InitStructure;
 	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 ;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
	GPIO_Init(GPIOB, &GPIO_InitStructure);	

	//PWRKEY��ʹ��
	GPIO_SetBits(GPIOB,GPIO_Pin_9);	

	for(i = 0; i < 2; i++)
	{
		delay_ms(1000);	
	}
	//�������������ͷ�
	GPIO_ResetBits(GPIOB,GPIO_Pin_9);

	for(i = 0; i < 2; i++)
	{
		delay_ms(1000);	
	}

}

void YR4G_GPRS_Restart(void)
{
	u8 temp = 0;
	YR4G_GPRS_OFF();
	for(temp = 0; temp < 30; temp++)
	{
		delay_ms(1000);
	}
	YR4G_GPRS_ON();

}

void YR4G_Powerkey_Restart(void)
{
	u8 temp = 0;
	BSP_Printf("Powerkey Restart\r\n");
	YR4G_PWRKEY_OFF();
	for(temp = 0; temp < 30; temp++)
	{
		delay_ms(1000);
	}
	YR4G_PWRKEY_ON();
}

void YR4G_Power_Restart(void)
{
	u8 temp = 0;
	YR4G_PWRKEY_OFF();
	YR4G_POWER_OFF();
	
	for(temp = 0; temp < 30; temp++)
	{
		delay_ms(1000);
	}
	YR4G_POWER_ON();
	YR4G_PWRKEY_ON();

}

//����1   ĳ��ATָ��ִ�д���
//����0   �ɹ������Ϸ�����
bool YR4G_Link_Server_AT(void)
{
	bool ret = FALSE;
	//����ATָ�������������
	if(ret = CheckModule())
		if(ret = GetVersion())
#if 0			
				if((ret = Check_SIM_Card()) == CMD_ACK_OK)
					if((ret = Check_CSQ()) == CMD_ACK_OK)
						if((ret = Get_ICCID()) == CMD_ACK_OK)
							//if((ret = Check_OPS()) == CMD_ACK_OK)
								//if((ret = YR4G_GPRS_OFF()) == CMD_ACK_OK)
									if((ret = YR4G_GPRS_CIPSHUT()) == CMD_ACK_OK)
										if((ret = YR4G_GPRS_CGCLASS()) == CMD_ACK_OK)
											if((ret = YR4G_GPRS_CGDCONT()) == CMD_ACK_OK)
												//if((ret = YR4G_GPRS_Adhere()) == CMD_ACK_OK)
													if((ret = YR4G_GPRS_Set()) == CMD_ACK_OK)
														//if((ret = YR4G_GPRS_Dispaly_IP()) == CMD_ACK_OK)
															if((ret = Link_Server_AT(0, ipaddr, port)) == CMD_ACK_OK)
																Reset_Device_Status(CMD_LOGIN);

#endif
	return ret;
}

bool YR4G_Link_Server_Powerkey(void)
{
	u8 retry = RETRY_AT;
	bool ret = FALSE;	
	while(retry != 0)
	{
		if(ret = YR4G_Link_Server_AT())
			break;
		YR4G_Powerkey_Restart();
		retry--;
	}

	return ret;

}
bool YR4G_Link_Server(void)
{
	u8 retry = RETRY_AT;
	bool ret = FALSE;
	while(retry != 0)
	{
		if(ret = YR4G_Link_Server_Powerkey())
			break;
		YR4G_Power_Restart();
		retry--;
	}
	
	return ret;

}

u8 Get_Device_Upload_Str(u8 msg_str_id, char *msg_str)
{
	msg_data *msg=(msg_data *)msg_str;
	char *p_left=msg_str+sizeof(msg_data);
	u8 Result_Validation = 0;
	u8 i;

	if(msg_str == NULL)
		return 0;

	if(msg_str_id>=MSG_STR_ID_MAX)
		return 0;

	strncpy(msg->id, msg_id[msg_str_id], MSG_STR_LEN_OF_ID);
	msg->id[MSG_STR_LEN_OF_ID] = delim;

  	strncpy(msg->length, "000", MSG_STR_LEN_OF_LENGTH);
	msg->length[MSG_STR_LEN_OF_LENGTH] = delim;

	if(dev.status == CMD_OPEN_DEVICE)
	{
		sprintf(msg->seq,"%03d",dev.msg_seq_s);
	}
	else
	{
	  	sprintf(msg->seq,"%03d",++dev.msg_seq);	
	}
	msg->seq[MSG_STR_LEN_OF_SEQ] = delim;

  	sprintf(msg->dup, "%02d", dev.msg_timeout);
	msg->dup[MSG_STR_LEN_OF_DUP] = delim;
	
	strncpy(msg->device, msg_device, MSG_STR_LEN_OF_DEVICE);
	msg->device[MSG_STR_LEN_OF_DEVICE] = delim;

	//���ڶ�ȡ����GPIO �ߵͣ�������豸ʵʱ״̬
	for(i = 0; i < MSG_STR_LEN_OF_PORTS; i++)
	{
		msg->ports[i] = (ON==Device_Power_Status(i))?'1':'0';		
	}	
	msg->ports[MSG_STR_LEN_OF_PORTS] = delim;

	Device_Timer_Status(msg->period);
	msg->period[MSG_STR_LEN_OF_PORTS_PERIOD] = delim;
	
	switch(msg_str_id)
	{
		case MSG_STR_ID_LOGIN:
			strcpy(p_left, "YR4G_");
			p_left += strlen("YR4G_");
			strncpy(p_left, ICCID_BUF, LENGTH_ICCID_BUF);
			p_left += LENGTH_ICCID_BUF;
			*p_left++ = delim;
		break;
		
		case MSG_STR_ID_HB:
		case MSG_STR_ID_OPEN:
		case MSG_STR_ID_CLOSE:
			
		break;
		
		default:
		break;
	}

  	sprintf(msg->length,"%03d",strlen(msg_str)-sizeof(msg->id)-sizeof(msg->length)+5);
	msg->length[MSG_STR_LEN_OF_LENGTH] = delim;	
	
	//���У���
	Result_Validation = Check_Xor_Sum(msg_str, strlen(msg_str));
	
	//У��ֵת��Ϊ�ַ���
  	sprintf(p_left,"%03d",Result_Validation);
	p_left += 3;
	*p_left++ = delim;
	*p_left++ = ending;
	*p_left = 0;

	memset(dev.sms_backup, 0, sizeof(dev.sms_backup));
	strncpy(dev.sms_backup, msg_str, strlen(msg_str));
	
	return strlen(msg_str);
}

//���͵�½��Ϣ��������
u8 Send_Login_Data(void)
{
	u8 ret = CMD_ACK_NONE;
	char Login_buf[100]={0};
	if(Get_Device_Upload_Str(MSG_STR_ID_LOGIN, Login_buf) != 0)
	{
		BSP_Printf("Login_Buffer:%s\r\n",Login_buf);	
		//Get_Login_Data();
		//BSP_Printf("Login_Buffer:%s\r\n",Login_Buffer);
		//ret = Send_Data_To_Server(Login_Buffer);
		ret = Send_Data_To_Server(Login_buf);
	}
	return ret;
}

u8 Send_Login_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = 5;	//ִ��count�Σ������ɹ��Ļ���������GPRS
	while(count != 0)
	{
		//Clear_Usart3();
		ret = Send_Login_Data();
		//Clear_Usart3();
		if(ret == CMD_ACK_NONE)
		{
			//��������ʧ��
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
		}
		else if((ret == CMD_ACK_OK) ||(ret == CMD_ACK_DISCONN))
			break;
		count -= 1;
	}
	
	return ret;
	
}

u8 Send_Login_Data_To_Server(void)
{
	u8 count = 5;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = Send_Login_Data_Normal();
		if(ret != CMD_ACK_OK)
		{
			YR4G_GPRS_Restart();
		}
		else
			break;
		count--;	
	}

	return ret;

}

//������������������
u8 Send_Heart_Data(void)
{
	u8 ret = CMD_ACK_NONE;
	char HB_buf[100]={0};
	if(Get_Device_Upload_Str(MSG_STR_ID_HB, HB_buf)!=0)
	{
		BSP_Printf("New HB:%s\r\n",HB_buf);		
		//Get_Heart_Data();
		//ret = Send_Data_To_Server(Heart_Buffer);
		ret = Send_Data_To_Server(HB_buf);
	}
	return ret;
}

u8 Send_Heart_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = 5;	//ִ��count�Σ������ɹ��Ļ���������GPRS
	while(count != 0)
	{
		//Clear_Usart3();
		ret = Send_Heart_Data();
		//Clear_Usart3();
		if(ret == CMD_ACK_NONE)
		{
			//��������ʧ��
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
		}
		else if((ret == CMD_ACK_OK) ||(ret == CMD_ACK_DISCONN))
			break;
		count -= 1;
	}
	
	return ret;

}

u8 Send_Heart_Data_To_Server(void)
{
	u8 ret = CMD_ACK_NONE;
	ret = Send_Heart_Data_Normal();
	return ret;
}

//���ͽ���ҵ��ָ����ɻ��ĸ�������
u8 Send_Open_Device_Data(void)
{
	u8 ret = CMD_ACK_NONE;
	char Open_Device_buf[100]={0};
	if(Get_Device_Upload_Str(MSG_STR_ID_OPEN, Open_Device_buf)!=0)
	{
		BSP_Printf("New Open:%s\r\n",Open_Device_buf);		
		//Get_Open_Device_Data();
		//ret = Send_Data_To_Server(Enbale_Buffer);
		ret = Send_Data_To_Server(Open_Device_buf);
	}
	return ret;
}

u8 Send_Open_Device_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = 5;	//ִ��count�Σ������ɹ��Ļ���������GPRS
	while(count != 0)
	{
		//Clear_Usart3();
		ret= Send_Open_Device_Data();
		//Clear_Usart3();
		if(ret == CMD_ACK_NONE)
		{
			//��������ʧ��
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
		}
		else if((ret == CMD_ACK_OK) ||(ret == CMD_ACK_DISCONN))
			break;
		count -= 1;
	}
	
	return ret;

}

//���������ʵ��һ���豸���ģ��豸��CMD_NONE ״̬��ʱ��(��ǰû�д����κ���Ϣ)�Ż���������
//��ȻӲ���Ŀ������յ�������ָ��ʱ����Ҫ��ɣ���������������Ϣʲôʱ����    
u8 Send_Open_Device_Data_To_Server(void)
{
	u8 ret = CMD_ACK_NONE;
	ret = Send_Open_Device_Data_Normal();	
	return ret;
}

//����ҵ��ִ�����ָ���������
u8 Send_Close_Device_Data(void)
{
	u8 ret = CMD_ACK_NONE;
	char Close_Device_buf[100]={0};
	if(Get_Device_Upload_Str(MSG_STR_ID_CLOSE, Close_Device_buf)!=0)
	{
		BSP_Printf("New Close:%s\r\n",Close_Device_buf);		
		//Get_Close_Device_Data();
		//ret = Send_Data_To_Server(Device_OK_Buffer);
		ret = Send_Data_To_Server(Close_Device_buf);
	}
	return ret;
}

u8 Send_Close_Device_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = 5;	//ִ��count�Σ������ɹ��Ļ���������GPRS
	while(count != 0)
	{
		//Clear_Usart3();
		ret = Send_Close_Device_Data();
		//Clear_Usart3();
		if(ret == CMD_ACK_NONE)
		{
			//��������ʧ��
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
		}
		else if((ret == CMD_ACK_OK) ||(ret == CMD_ACK_DISCONN))
			break;
		count --;		
	}

	return ret;

}

u8 Send_Close_Device_Data_To_Server(void)
{
	u8 ret = CMD_ACK_NONE;
	ret = Send_Close_Device_Data_Normal();
	return ret;
}

void Clear_buffer(char* buffer,u16 length)
{
	u16 i = 0;
	for(i = 0; i < length;i++)
	{
		buffer[i] = 0;
	}
}

//////////////���У��ͺ���///////
u8 Check_Xor_Sum(char* pBuf, u16 len)
{
	u8 Sum = 0;
	u8 i = 0;
	Sum = pBuf[0];
	
	for (i = 1; i < len; i++ )
	{
		Sum = (Sum ^ pBuf[i]);
	}
	
	return Sum;
}

