#ifndef __YR4G_H__
#define __YR4G_H__	 
#include "sys.h"
#include "device.h"

#define LENGTH_VERSION_BUF 40
extern char version[LENGTH_VERSION_BUF];

#define LENGTH_WKMOD_BUF 40
extern char wkmod[LENGTH_WKMOD_BUF];

#define LENGTH_PASSWORD_BUF 40
extern char password[LENGTH_PASSWORD_BUF];

#define LENGTH_SYSINFO_BUF 40 
extern char sysinfo[LENGTH_SYSINFO_BUF];

#define LENGTH_SN_BUF  40
extern char sn[LENGTH_SN_BUF];

#define LENGTH_ICCID_BUF 40
extern char iccid[LENGTH_ICCID_BUF];

#define LENGTH_IMEI_BUF 40
extern char imei[LENGTH_IMEI_BUF];

#define LENGTH_CSQ_BUF 40
extern char csq[LENGTH_CSQ_BUF];

extern uint32_t  lastOutActivity;
extern uint32_t  lastInActivity;

#define LENGTH_ATCMD_ACK 50
#define LENGTH_DEVICE_OPEN_CMD        50
#define LENGTH_USART_DATA        100
#define LENGTH_SMS_BACKUP        100

/*********WJ*********/
#define YR4G_STARTUP_MSG   "[USR-LTE-7S4]"

#define MSG_STR_DEVICE_HEADER       "TRVAP"
#define MSG_STR_SERVER_HEADER       "TRVBP"

#define MSG_STR_LEN_OF_HEADER                        5         //strlen("TRVXP")
#define MSG_STR_LEN_OF_ID                                  2 
#define MSG_STR_LEN_OF_LENGTH                         3
#define MSG_STR_LEN_OF_SEQ                               3
#define MSG_STR_LEN_OF_DUP                               2
#define MSG_STR_LEN_OF_DEVICE                          3
#define MSG_STR_LEN_OF_PORTS                           4
#define MSG_STR_LEN_OF_PORTS_PERIOD            (MSG_STR_LEN_OF_PORTS*4)

typedef struct
{
	bool is_login;
	bool hb_ready;
	u8 hb_timer;
	bool wait_reply;	
	u8 reply_timer;	
	u8 need_reset;
	u8 portClosed;
	u8 msg_seq;
	u8 msg_seq_s;
	char sms_backup[LENGTH_SMS_BACKUP];	
}t_DEV;

typedef struct
{
	char header[MSG_STR_LEN_OF_HEADER+1];
	char id[MSG_STR_LEN_OF_ID+1];
	char length[MSG_STR_LEN_OF_LENGTH+1];
	char seq[MSG_STR_LEN_OF_SEQ+1];
	char dup[MSG_STR_LEN_OF_DUP+1];	
	char device[MSG_STR_LEN_OF_DEVICE+1];
	char ports[MSG_STR_LEN_OF_PORTS+1];
	char period[MSG_STR_LEN_OF_PORTS_PERIOD+1];
}MsgDev;

typedef struct
{
	char header[MSG_STR_LEN_OF_HEADER+1];
	char id[MSG_STR_LEN_OF_ID+1];
	char length[MSG_STR_LEN_OF_LENGTH+1];
	char seq[MSG_STR_LEN_OF_SEQ+1];
	char dup[MSG_STR_LEN_OF_DUP+1];
	char device[MSG_STR_LEN_OF_DEVICE+1];	
}MsgSrv;

/*********WJ*********/
extern t_DEV dev;

enum
{
	MSG_STR_ID_LOGIN=0,
	MSG_STR_ID_HB,
	MSG_STR_ID_OPEN,
	MSG_STR_ID_CLOSE,

	MSG_STR_ID_MAX
};

enum
{
	ERR_NONE = 0,

	ERR_INIT_MODULE,
	ERR_RETRY_TOO_MANY_TIMES,	

	ERR_DISCONNECT,	

	ERR_SEND_CMD,	
};

enum
{
	READ,
	WRITE,
};

#define MAX_REGISTER_DATA_LENGTH   80
#define MAX_HEART_DATA_LENGTH   80
typedef struct
{
	bool isOn;
	bool isConnected;	
	u8 timeout;
	char *sl;
	char *protocol;
	char addr[50];
	char port[10];
	bool isRegEn;
	char *regtp;
	char regdt[MAX_REGISTER_DATA_LENGTH];
	char *regsnd;
	bool isHeartEn;
	char heartdt[MAX_HEART_DATA_LENGTH];
	char *heartsnd;
	u16 hearttim;
}SockSetting;

enum
{
	SOCK_A,
	SOCK_B,
	SOCK_C,
	SOCK_D,
	SOCK_MAX,
};

extern SockSetting socketSetting[SOCK_MAX];

extern const char *msg_id[MSG_STR_ID_MAX];
extern const char *msg_id_s[MSG_STR_ID_MAX];
extern char  *cell;

bool 	YR4G_Send_Cmd(char *cmd,char *ack,char *recv,u16 waittime);
u8 CheckSum(char* pBuf, u16 len);

char *DumpQueue(char * recv);
bool 	CheckModule(void);
bool 	GetVersion(void);
bool 	GetSysinfo(void);
bool 	GetPassword(void);
bool 	GetResetTime(void);
bool 	GetCSQ(void);
bool GetSN(void);
bool 	GetICCID(void);
bool GetIMEI(void);
bool SaveSetting(void);
bool SocketParam(u8 rw, u8 sock, u8 *protocol, u8 *addr, u8 *port, SockSetting *pSocketSetting);
bool SocketEnable(u8 rw, u8 sock, bool enable, SockSetting *pSocketSetting);
bool SocketSL(u8 rw, u8 sock, u8 sl, SockSetting *pSocketSetting);
bool isConnected(u8 sock, SockSetting *pSocketSetting);
bool SocketTO(u8 sock, SockSetting *pSocketSetting);
	
void YR4G_POWER_ON(void);
void YR4G_POWER_OFF(void);
void YR4G_PWRKEY_ON(void);
void YR4G_PWRKEY_OFF(void);

void YR4G_GPRS_Restart(void);
void YR4G_Powerkey_Restart(void);
void YR4G_Power_Restart(void);

bool YR4G_Link_Server(void);
bool YR4G_Link_Server_AT(void);
bool YR4G_Link_Server_Powerkey(void);

void SendLogin(void);
void SendHeart(void);
void SendStartAck(void);
void SendFinish(void);
bool isWorking(void);
bool RegEnable(u8 rw, u8 sock, bool enable, SockSetting *pSocketSetting);
bool HeartEnable(u8 rw, u8 sock, bool enable, SockSetting *pSocketSetting);
char *YR4G_SMS_Create(char *sms_data, char *raw);
#endif
