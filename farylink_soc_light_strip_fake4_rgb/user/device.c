
#define __DEVICE_GLOBALS
#include "device.h"
#include "mem_pool.h"
#include "smart_wf_frame.h"
#include "key.h"
#include "led_indicator.h"
#include "system/sys_timer.h"
#include "system/uni_thread.h"
#include "system/uni_mutex.h"
#include "system/uni_semaphore.h"
#include "uart.h"
#include "smart_link.h"
#include "pwm.h"
#include "wf_sdk_adpt.h"
#include "gpio_pin.h"
#include "uart.h"
#include "ir_tx_rx.h"
#include "ir_code.h"
#include "math.h"
/***********************************************************
*************************  新面板  新DP  ***********************
***********************************************************/
#define R_OUT_IO_MUX        GPIO_NAME_14
#define R_OUT_IO_FUNC       GPIO_FUNC_14
#define R_OUT_IO_NUM        14
#define R_OUT_CHANNEL       0

#define G_OUT_IO_MUX        GPIO_NAME_12
#define G_OUT_IO_FUNC       GPIO_FUNC_12
#define G_OUT_IO_NUM        12
#define G_OUT_CHANNEL       1

#define B_OUT_IO_MUX        GPIO_NAME_13
#define B_OUT_IO_FUNC       GPIO_FUNC_13
#define B_OUT_IO_NUM        13
#define B_OUT_CHANNEL       2

// reset key define
#define WF_RESET_KEY GPIO_ID_PIN(3)


#define CHAN_NUM 3
#define PWM_MIN 0
#define PWM_MAX 1024

uint32 duty[CHAN_NUM] = {100};
uint32 pwm_val = 0;

uint32 io_info[][3] ={
    {R_OUT_IO_MUX,R_OUT_IO_FUNC,R_OUT_IO_NUM},
    {G_OUT_IO_MUX,G_OUT_IO_FUNC,G_OUT_IO_NUM},
    {B_OUT_IO_MUX,B_OUT_IO_FUNC,B_OUT_IO_NUM},
};

typedef struct
{
	THREAD msg_thread;
	SEM_HANDLE press_key_sem;
}TY_MSG_S;

STATIC TY_MSG_S ty_msg;
STATIC VOID key_process(INT gpio_no,PUSH_KEY_TYPE_E type,INT cnt);
//STATIC KEY_ENTITY_S key_tbl[] = {
//	{WF_RESET_KEY,3000,key_process,0,0,0,0},
//};

#define REG_WRITE(_r,_v)    (*(volatile uint32 *)(_r)) = (_v)
#define REG_READ(_r)        (*(volatile uint32 *)(_r))
#define WDEV_NOW()          REG_READ(0x3ff20c00)

#define DPID_0				20		//开关
#define DPID_1				21		//模式
#define DPID_2				22		//亮度
#define DPID_4				24		//彩光
#define DPID_5				25		//场景
#define DPID_6				26		//倒计时
#define DPID_7				27		//音乐灯
#define DPID_8				28		//调节
#define DPID_9				101		//麦克风

typedef enum {
    WHITE_MODE = 0,
    COLOUR_MODE,
    SCENE_MODE,
	MUSIC_MODE,
}SCENE_MODE_E;

typedef struct {
	UINT RED_VAL;
	UINT GREEN_VAL;
	UINT BLUE_VAL;
	UINT WHITE_VAL;
	UINT LAST_RED_VAL;
	UINT LAST_GREEN_VAL;
	UINT LAST_BLUE_VAL;
	UINT LAST_WHITE_VAL;
	UINT FIN_RED_VAL;
	UINT FIN_GREEN_VAL;
	UINT FIN_BLUE_VAL;
	UINT FIN_WHITE_VAL;
	USHORT HUE;
	UCHAR SATURATION;
	UCHAR VALUE;
}LIGHT_DATA_DEF;
STATIC LIGHT_DATA_DEF light_data;

//数据结构 灯明亮(2字节)+灯冷暖(2字节) 灯频率(2字节)+数目(2字节)+ 一组灯(3个字节)

typedef struct {
	UINT RED_VAL;
	UINT GREEN_VAL;
	UINT BLUE_VAL;
	UINT WHITE_VAL;
}DATA_GROUP_DEF;

typedef struct {
	UINT BRIGHT;
	UCHAR SPEED;
	UCHAR NUM;
	UCHAR color_num;
	DATA_GROUP_DEF data_group[8];
}FLASH_LIGHT_DATA_DEF;
STATIC FLASH_LIGHT_DATA_DEF flash_light_data;


typedef struct
{
	BOOL scale_flag;
    THREAD gra_thread;
    SEM_HANDLE gra_key_sem;
}L_GRA_CHANGE_DEF;
STATIC L_GRA_CHANGE_DEF gra_change;

typedef struct
{
	MUTEX_HANDLE  mutex;
    THREAD flash_scene_thread;
    xSemaphoreHandle flash_scene_sem;
}FLASH_SCENE_HANDLE_DEF;
STATIC FLASH_SCENE_HANDLE_DEF flash_scene_handle;

typedef struct
{
    THREAD mic_music_thread;
    BOOL mic_scan_flag;
}MIC_MUSIC_HANDLE_DEF;
STATIC MIC_MUSIC_HANDLE_DEF mic_music_handle;

typedef struct
{
	BOOL SWITCH;
	BOOL MIC_SWITCH;
	SCENE_MODE_E WORK_MODE;
	UINT BRIGHT;
	UCHAR COLOUR_DATA[12];
	UCHAR MUSIC_DATA[22];
	UCHAR SCENE_DATA[210];
	UCHAR SCENE_ROUHE_DATA[210];
	UCHAR SCENE_BINFEN_DATA[210];
	UCHAR SCENE_XUANCAI_DATA[210];
	UCHAR SCENE_BANLAN_DATA[210];
	INT APPT_SEC;
}DP_DEF;
STATIC DP_DEF dp_data;

typedef enum {
    FUC_TEST1 = 0,
	AGING_TEST,
	FUC_TEST2,
}TEST_MODE_DEF;

typedef enum {
	FUN_SUC = 0,
    NO_KEY,
	WIFI_TEST_ERR,
}FUN_TEST_RET;


typedef struct
{
	UINT pmd_times;
	UINT aging_times;
	UINT aging_tested_time;
	BOOL wf_test_ret;
	FUN_TEST_RET fun_test_ret;
	TEST_MODE_DEF test_mode;
	TIMER_ID fuc_test_timer;
	TIMER_ID aging_test_timer;
}TEST_DEF;

typedef enum
{
	statical=0,		//静态
	jump,			//跳变
	gradual,		//呼吸
}move_mode;
UCHAR change_mode;

UCHAR scene_len;
uint16_t color_r,color_g,color_b;

/* 总长为30分钟，白光老化10分钟，黄光老化10分钟 ，剩余的时间RGB全亮*/
STATIC TEST_DEF test_handle;
#define AGING_TEST_TIME 30
#define AGING_TEST_C_TIME 10
#define AGING_TEST_W_TIME 10

#define     COLOUR_MODE_DEFAULT         "000003e803e8"

#define DEVICE_MOD "device_mod"
#define DEVICE_PART "device_part"
#define FASE_SW_CNT_KEY "fsw_cnt_key"
#define DP_DATA_KEY   "dp_data_key"
#define LIGHT_TEST_KEY   "light_test_key"
#define AGING_TESTED_TIME  "aging_tested_time"


#define BRIGHT_INIT_VALUE 1000
#define BRIGHT_LOW_POWER 100
#define BRIGHT_SMARTCONFIG 200
#define COL_TEMP_INIT_VALUE 1000

#define NORMAL_DELAY_TIME 3
#define RESO_VAL 15

#define ONE_SEC   1
#define WM_SUCCESS 0
#define WM_FAIL 1

STATIC UINT TEST_R_BRGHT = 0x03E8;
STATIC UINT TEST_G_BRGHT = 0x03E8;
STATIC UINT TEST_B_BRGHT = 0x03E8;
STATIC UINT TEST_C_BRGHT = 0x03E8;
STATIC UINT TEST_W_BRGHT = 0x03E8;
volatile STATIC BOOL flash_scene_flag = TRUE;
/***********************************************************
*************************function define********************
***********************************************************/
STATIC OPERATE_RET device_differ_init(VOID);
STATIC INT ABS(INT value);
STATIC VOID wf_direct_timer_cb(UINT timerID,PVOID pTimerArg);
STATIC VOID reset_fsw_cnt_cb(UINT timerID,PVOID pTimerArg);
STATIC OPERATE_RET dev_inf_get(VOID);
STATIC OPERATE_RET dev_inf_set(VOID);
STATIC VOID init_upload_proc(VOID);
STATIC VOID sl_datapoint_proc(cJSON *root);
STATIC USHORT byte_combine(UCHAR hight, UCHAR low);
STATIC VOID idu_timer_cb(UINT timerID,PVOID pTimerArg);
STATIC VOID char_change(UINT temp, UCHAR *hight, UCHAR *low);
STATIC INT string_combine_int(u32 a,u32 b, u32 c,u32 d);
STATIC VOID start_gra_change(TIME_MS delay_time);
STATIC VOID light_gra_change(PVOID pArg);
STATIC VOID flash_scene_change(PVOID pArg);
STATIC INT ty_get_enum_id(UCHAR dpid, UCHAR *enum_str);
STATIC UCHAR *ty_get_enum_str(DP_CNTL_S *dp_cntl, UCHAR enum_id);
STATIC VOID wfl_timer_cb(UINT timerID,PVOID pTimerArg);
STATIC VOID work_mode_change(SCENE_MODE_E mode);
STATIC VOID hw_test_timer_cb(void);
STATIC VOID get_light_data(VOID);
STATIC VOID set_default_dp_data(VOID);
STATIC VOID send_light_data(UINT R_value, UINT G_value, UINT B_value, UINT CW_value, UINT WW_value);
STATIC VOID msg_proc(PVOID pArg);
void hsv2rgb(float h, float s, float v);
STATIC VOID msg_send(INT cmd);
STATIC INT msg_upload_sec(VOID);
STATIC VOID count_timer_cb(UINT timerID,PVOID pTimerArg);
STATIC VOID mic_adc_change(PVOID pArg);
STATIC VOID ir_rcvd_timer_cb(UINT timerID,PVOID pTimerArg);
/***********************************************************
*************************variable define********************
***********************************************************/

TIMER_ID wf_stat_dir;
TIMER_ID timer_init_dpdata;
TIMER_ID gradua_timer;
TIMER_ID timer;
TIMER_ID data_save_timer;
TIMER_ID count_timer;;
TIMER_ID ir_rcvd_timer;

STATIC UINT irq_cnt = 0;
STATIC UINT num_cnt = 0;
STATIC INT flash_dir = 0;

BOOL sta_cha_flag = true;
/***********************************************************
*************************function define********************
***********************************************************/

STATIC USHORT byte_combine(UCHAR hight, UCHAR low)
{
    USHORT temp;
    temp = (hight << 8) | low;
    return temp;
}
STATIC VOID char_change(UINT temp, UCHAR *hight, UCHAR *low)
{
    *hight = (temp & 0xff00) >> 8;
    *low = temp & 0x00ff;
}

STATIC INT string_combine_byte(u32 a,u32 b)
{
   INT combine_data = (a<<4)|(b&0xf);
   return combine_data;
}
STATIC INT string_combine_short(u32 a,u32 b, u32 c,u32 d)
{
   INT combine_data = (a<<12)|(b<<8)|(c<<4)|(d&0xf);
   return combine_data;
}


STATIC INT ABS(INT value)
{
	if(value < 0){
		return 0-value;
	}else{
		return value;
	}
}

STATIC UCHAR get_max_value(UCHAR a, UCHAR b, UCHAR c, UCHAR d, UCHAR e)
{
	int x = a > b ? a : b; //1次比较，1次赋值
	int y = c > d ? c : d; //1次比较，1次赋值
	int z = x > y ? x : y;
	return z > e ? z : e;  //1次比较
}


STATIC CHAR* my_itoa(int num,char*str,int radix)
{
/*索引表*/
    char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned unum;/*中间变量*/
    int i=0,j,k;
    char temp;
    /*确定unum的值*/
    if(radix==10&&num<0)/*十进制负数*/
    {
        unum=(unsigned)-num;
        str[i++]='-';
    }
    else unum=(unsigned)num;/*其他情况*/
    /*转换*/
    do{
        str[i++]=index[unum%(unsigned)radix];
        unum/=radix;
    }while(unum);
    str[i]='\0';
    /*逆序*/
    if(str[0]=='-')k=1;/*十进制负数*/
    else k=0;
    for(j=k;j<=(i-1)/2;j++)
    {
        temp=str[j];
        str[j]=str[i-1+k-j];
        str[i-1+k-j]=temp;
    }
    return str;
}
static unsigned char abcd_to_asc(unsigned char ucBcd)
{
	unsigned char ucAsc = 0;

	ucBcd &= 0x0f;
	if (ucBcd <= 9)
		ucAsc = ucBcd + '0';
	else
		ucAsc = ucBcd + 'A' - 10;
	return (ucAsc);
}

void BcdToAsc_Api(char * sAscBuf, unsigned char * sBcdBuf, int iAscLen)
{
	int i, j;j = 0;

	if((sBcdBuf == NULL) || (sAscBuf == NULL) || (iAscLen < 0))
		return;

	for (i = 0; i < iAscLen / 2; i++)
	{
		sAscBuf[j] = (sBcdBuf[i] & 0xf0) >> 4;
		sAscBuf[j] = abcd_to_asc(sAscBuf[j]);
		j++;
		sAscBuf[j] = sBcdBuf[i] & 0x0f;
		sAscBuf[j] = abcd_to_asc(sAscBuf[j]);
		j++;
	}
	if (iAscLen % 2)
	{
		sAscBuf[j] = (sBcdBuf[i] & 0xf0) >> 4;
		sAscBuf[j] = abcd_to_asc(sAscBuf[j]);
	}
}

VOID device_cb(SMART_CMD_E cmd,cJSON *root)
{
    BOOL op_led = FALSE;
    CHAR *buf = cJSON_PrintUnformatted(root);
    if(NULL == buf) {
        PR_ERR("malloc error");
        return;
    }
    PR_DEBUG("the receive buf is %s",buf);
    cJSON *nxt = root->child;
    while(nxt) {
        sl_datapoint_proc(nxt);
        nxt = nxt->next;
        op_led = TRUE;
    }
    if(TRUE == op_led) {
		if((!IsThisSysTimerRun(data_save_timer)) && (!IR_RX_BUFF.fill_cnt)){
        	sys_start_timer(data_save_timer,5000,TIMER_CYCLE);
		}
        op_led = FALSE;
    }
	//返回信息
	OPERATE_RET op_ret = tuya_obj_dp_report(buf);
	if(OPRT_OK != op_ret) {
		PR_ERR("tuya_obj_dp_report err:%d",op_ret);
		PR_DEBUG_RAW("%s\r\n",buf);
		Free(buf);
		return;
    }
    Free(buf);
}

STATIC VOID data_save_timer_cb(UINT timerID,PVOID pTimerArg)
{
     dev_inf_set();
	 sys_stop_timer(data_save_timer);
}

STATIC VOID send_light_data(UINT R_value, UINT G_value, UINT B_value, UINT CW_value, UINT WW_value)
{
//	PR_DEBUG("R=%d,G=%d,B=%d,C=%d,W=%d",R_value,G_value,B_value,CW_value,WW_value);
	pwm_val = PWM_MAX*R_value/1000 + PWM_MAX*CW_value/1000;
	pwm_set_duty(pwm_val, R_OUT_CHANNEL);

	pwm_val = PWM_MAX*G_value/1000 + PWM_MAX*CW_value/1000;
	pwm_set_duty(pwm_val, G_OUT_CHANNEL);

	pwm_val = PWM_MAX*B_value/1000 + PWM_MAX*CW_value/1000;
	pwm_set_duty(pwm_val, B_OUT_CHANNEL);

	pwm_start();
}

/*************************************************************light test********************************/

STATIC OPERATE_RET get_light_test_flag(VOID)
{
	OPERATE_RET op_ret;

	UCHAR *buf;
	buf = Malloc(32);
	if(NULL == buf) {
		PR_ERR("Malloc failed");
		goto ERR_EXT;
	}

	op_ret = msf_get_single(DEVICE_MOD, LIGHT_TEST_KEY, buf, 32);
	if(OPRT_OK != op_ret){
		PR_ERR("msf_get_single failed");
		Free(buf);
		goto ERR_EXT;
	}
	PR_DEBUG("buf:%s",buf);

	cJSON *root = NULL;
	root = cJSON_Parse(buf);
	if(NULL == root) {
		Free(buf);
		test_handle.test_mode = FUC_TEST1;
		return OPRT_CJSON_PARSE_ERR;
	}
	Free(buf);

	cJSON *json;
	json = cJSON_GetObjectItem(root,"test_mode");
	if(NULL == json) {
		test_handle.test_mode = FUC_TEST1;
	}else{
		test_handle.test_mode = json->valueint;
	}

	cJSON_Delete(root);
	return OPRT_OK;

ERR_EXT:
	test_handle.test_mode = FUC_TEST1;
	return OPRT_COM_ERROR;

}

STATIC OPERATE_RET set_light_test_flag(VOID)
{
	OPERATE_RET op_ret;
    INT i = 0;
	CHAR *out = NULL;

	cJSON *root_test = NULL;
	root_test = cJSON_CreateObject();
	if(NULL == root_test) {
		PR_ERR("json creat failed");
		goto ERR_EXT;
	}

	cJSON_AddNumberToObject(root_test, "test_mode", test_handle.test_mode);

	out=cJSON_PrintUnformatted(root_test);
	cJSON_Delete(root_test);
	if(NULL == out) {
		PR_ERR("cJSON_PrintUnformatted err:");
		Free(out);
		goto ERR_EXT;
	}
	op_ret = msf_set_single(DEVICE_MOD, LIGHT_TEST_KEY, out);
	if(OPRT_OK != op_ret) {
		PR_ERR("data write psm err: %d",op_ret);
		Free(out);
		goto ERR_EXT;
	}
	Free(out);
	return OPRT_OK;
ERR_EXT:
	return OPRT_COM_ERROR;

}

STATIC OPERATE_RET get_aging_tested_time(VOID)
{
	OPERATE_RET op_ret;

	UCHAR *buf;
	buf = Malloc(64);
	if(NULL == buf) {
		PR_ERR("Malloc failed");
		goto ERR_EXT;
	}

	op_ret = msf_get_single(DEVICE_MOD, AGING_TESTED_TIME, buf, 64);
	if(OPRT_OK != op_ret){
		PR_ERR("msf_get_single failed");
		Free(buf);
		goto ERR_EXT;
	}
	PR_DEBUG("buf:%s",buf);

	cJSON *root = NULL;
	root = cJSON_Parse(buf);
	if(NULL == root) {
		Free(buf);
		test_handle.aging_tested_time = 0;
		return OPRT_CJSON_PARSE_ERR;
	}
	Free(buf);

	cJSON *json;
	json = cJSON_GetObjectItem(root,"aging_tested_time");
	if(NULL == json) {
		test_handle.aging_tested_time = 0;
	}else{
		test_handle.aging_tested_time = json->valueint;
	}

	cJSON_Delete(root);
	return OPRT_OK;

ERR_EXT:
	test_handle.aging_tested_time = 0;
	return OPRT_COM_ERROR;

}

STATIC OPERATE_RET set_aging_tested_time(VOID)
{
	OPERATE_RET op_ret;
    INT i = 0;
	CHAR *out = NULL;

	cJSON *root_test = NULL;
	root_test = cJSON_CreateObject();
	if(NULL == root_test) {
		PR_ERR("json creat failed");
		goto ERR_EXT;
	}

	cJSON_AddNumberToObject(root_test, "aging_tested_time", test_handle.aging_tested_time);

	out=cJSON_PrintUnformatted(root_test);
	cJSON_Delete(root_test);
	if(NULL == out) {
		PR_ERR("cJSON_PrintUnformatted err:");
		Free(out);
		goto ERR_EXT;
	}
	PR_DEBUG("out[%s]", out);
	op_ret = msf_set_single(DEVICE_MOD, AGING_TESTED_TIME, out);
	if(OPRT_OK != op_ret) {
		PR_ERR("data write psm err: %d",op_ret);
		Free(out);
		goto ERR_EXT;
	}
	Free(out);
	return OPRT_OK;
ERR_EXT:
	return OPRT_COM_ERROR;

}

STATIC INT get_reset_cnt(VOID)
{
	OPERATE_RET op_ret;
	INT cnt = 0;
	UCHAR *buf;
	buf = Malloc(32);
	if(NULL == buf) {
		PR_ERR("Malloc failed");
		goto ERR_EXT;
	}

	op_ret = msf_get_single(DEVICE_MOD, FASE_SW_CNT_KEY, buf, 32);
	if(OPRT_OK != op_ret){
		PR_ERR("get_reset_cnt failed");
		Free(buf);
		goto ERR_EXT;
	}
	PR_DEBUG("buf:%s",buf);

	cJSON *root = NULL;
	root = cJSON_Parse(buf);
	if(NULL == root) {
		Free(buf);
		goto ERR_EXT;
	}
	Free(buf);

	cJSON *json;
	json = cJSON_GetObjectItem(root,"fsw_cnt_key");
	if(NULL == json) {
		cnt = 0;
	}else{
		cnt = json->valueint;
	}

	cJSON_Delete(root);
	return cnt;

ERR_EXT:
	return 0;

}

STATIC OPERATE_RET set_reset_cnt(INT val)
{
	OPERATE_RET op_ret;
	CHAR *out = NULL;

	cJSON *root_test = NULL;
	root_test = cJSON_CreateObject();
	if(NULL == root_test) {
		PR_ERR("json creat failed");
		goto ERR_EXT;
	}

	cJSON_AddNumberToObject(root_test, "fsw_cnt_key", val);

	out=cJSON_PrintUnformatted(root_test);
	cJSON_Delete(root_test);
	if(NULL == out) {
		PR_ERR("cJSON_PrintUnformatted err:");
		Free(out);
		goto ERR_EXT;
	}
	PR_DEBUG("out[%s]", out);
	op_ret = msf_set_single(DEVICE_MOD, FASE_SW_CNT_KEY, out);
	if(OPRT_OK != op_ret) {
		PR_ERR("data write psm err: %d",op_ret);
		Free(out);
		goto ERR_EXT;
	}
	Free(out);
	return OPRT_OK;
ERR_EXT:
	return OPRT_COM_ERROR;

}

STATIC VOID fuc_test_timer_cb(UINT timerID,PVOID pTimerArg)
{
	test_handle.pmd_times ++;


	if(test_handle.test_mode == FUC_TEST1){
		 switch(test_handle.pmd_times % 6){
			case 1: send_light_data(TEST_R_BRGHT, 0, 0, 0, 0); break;
			case 2: send_light_data(0, TEST_G_BRGHT, 0, 0, 0); break;
			case 3: send_light_data(0, 0, TEST_B_BRGHT, 0, 0); break;
			case 4:	send_light_data(0, 0, 0, TEST_C_BRGHT, 0); break;
            case 5:
			case 0:	send_light_data(0, 0, 0, 0, TEST_W_BRGHT); break;
			default:break;
		}
	 }
	 else{
		switch(test_handle.pmd_times % 11){
		 	case 1: send_light_data(TEST_R_BRGHT, 0, 0, 0, 0); break;
			case 2: send_light_data(0, 0, 0, 0, 0); break;
			case 3: send_light_data(0, TEST_G_BRGHT, 0, 0, 0); break;
			case 4: send_light_data(0, 0, 0, 0, 0); break;
			case 5: send_light_data(0, 0, TEST_B_BRGHT, 0, 0); break;
			case 6: send_light_data(0, 0, 0, 0, 0); break;
			case 7: send_light_data(0, 0, 0, TEST_C_BRGHT, 0); break;
			case 8: send_light_data(0, 0, 0, 0, 0); break;
			case 9:
			case 10:send_light_data(0, 0, 0, 0, TEST_W_BRGHT); break;
			case 0: send_light_data(0, 0, 0, 0, 0); break;
			default:break;
		 }
	 }

	if(test_handle.pmd_times == 120){
		if(test_handle.test_mode == FUC_TEST1){
			test_handle.test_mode = AGING_TEST;
			if(OPRT_OK != set_light_test_flag()) {
				sys_stop_timer(test_handle.fuc_test_timer);
				send_light_data(0x00, 0x00, 0x00, 0x00, 0x00);
			}else{
				SystemReset();
			}
		}
	}
}

STATIC VOID aging_test_timer_cb(UINT timerID,PVOID pTimerArg)
{
		test_handle.aging_tested_time ++;
		if(OPRT_OK != set_aging_tested_time()){
			send_light_data(0x00, 0x00, TEST_B_BRGHT, 0x00, 0x00);
			sys_stop_timer(test_handle.aging_test_timer);
		}

	if(test_handle.aging_tested_time == AGING_TEST_TIME){
		sys_stop_timer(test_handle.aging_test_timer);
		send_light_data(0, TEST_G_BRGHT, 0, 0, 0);
		test_handle.test_mode = FUC_TEST2;
		test_handle.aging_tested_time = 0;
		if(OPRT_OK != set_light_test_flag()){
			send_light_data(0x00, 0x00, TEST_B_BRGHT, 0x00, 0x00);
		}

		if(OPRT_OK != set_aging_tested_time()){
			send_light_data(0x00, 0x00, TEST_B_BRGHT, 0x00, 0x00);
		}
	}else{
        if(test_handle.aging_tested_time >= (AGING_TEST_C_TIME + AGING_TEST_W_TIME)) {
            send_light_data(TEST_R_BRGHT, TEST_G_BRGHT, TEST_B_BRGHT, 0x00, 0x00);
        } else if(test_handle.aging_tested_time < AGING_TEST_C_TIME){
            send_light_data(0x00, 0x00, 0x00, TEST_C_BRGHT, 0);
        } else if((test_handle.aging_tested_time >= AGING_TEST_C_TIME) && (test_handle.aging_tested_time < (AGING_TEST_C_TIME + AGING_TEST_W_TIME))) {
            send_light_data(0x00, 0x00, 0x00, 0, TEST_W_BRGHT);
        }


	}
}

STATIC VOID gpio_func_test_cb(UINT timerID,PVOID pTimerArg)
{
	 STATIC u32 times = 0;
	 times ++;
	 switch(times % 5){
	 	case 1: send_light_data(100, 0, 0, 0, 0); break;
		case 2: send_light_data(0, 100, 0, 0, 0); break;
        case 3: send_light_data(0, 0, 100, 0, 0); break;
        case 4: send_light_data(0, 0, 0, 100, 0); break;
		case 0: send_light_data(0, 0, 0, 0, 100); break;
		default:break;
	 }
}

BOOL gpio_func_test(VOID)
{
	STATIC TIMER_ID gpio_func_test_timer;
	STATIC BOOL timer_add_flag = FALSE;

	if(timer_add_flag == FALSE){
		if(sys_add_timer(gpio_func_test_cb,NULL,&gpio_func_test_timer) != OPRT_OK) {
			return FALSE;
		}
		timer_add_flag = TRUE;
	}

	if(!IsThisSysTimerRun(gpio_func_test_timer)){
		if(OPRT_OK != sys_start_timer(gpio_func_test_timer, 500, TIMER_CYCLE)){
			return FALSE;
		}
	}

	return TRUE;
}

/*********************************************************************************************************/
VOID prod_test(BOOL flag, CHAR rssi)
{
	OPERATE_RET op_ret;
	flash_scene_flag = FALSE;
	print_port_init(0);
	PR_DEBUG("rssi:%d", rssi);
	set_reset_cnt(0);

	if(OPRT_OK != get_light_test_flag()){
		PR_ERR("get_light_test_flag err.......");
	}

	if(test_handle.test_mode == AGING_TEST){
		get_aging_tested_time();
		op_ret = sys_add_timer(aging_test_timer_cb,NULL,&test_handle.aging_test_timer);
	    if(OPRT_OK != op_ret) {
			send_light_data(0x00, 0x00, 0x00, 0x00, 0x00);
	        return;
	    }

		if(test_handle.aging_tested_time >= (AGING_TEST_C_TIME + AGING_TEST_W_TIME)) {
			send_light_data(TEST_R_BRGHT, TEST_G_BRGHT, TEST_B_BRGHT, 0x00, 0x00);
		} else if(test_handle.aging_tested_time < AGING_TEST_C_TIME){
			send_light_data(0x00, 0x00, 0x00, TEST_C_BRGHT, 0);
		} else if((test_handle.aging_tested_time >= AGING_TEST_C_TIME) && ((test_handle.aging_tested_time < (AGING_TEST_C_TIME + AGING_TEST_W_TIME)))) {
			send_light_data(0x00, 0x00, 0x00, 0, TEST_W_BRGHT);
		}

		test_handle.aging_times = 0;
        sys_start_timer(test_handle.aging_test_timer, 60000, TIMER_CYCLE);
	} else {
		if(rssi < -60 || flag == FALSE) {
			send_light_data(TEST_R_BRGHT, 0, 0, 0, 0);
			return;
	    }

		op_ret = sys_add_timer(fuc_test_timer_cb,NULL,&test_handle.fuc_test_timer);
	    if(OPRT_OK != op_ret) {
			send_light_data(0x00, 0x00, 0x00, 0x00, 0x00);
	        return;
	    }
		test_handle.pmd_times = 0;
		sys_start_timer(test_handle.fuc_test_timer, 1000, TIMER_CYCLE);
	}
	return;
}

VOID reset_light_sta(VOID)
{
    dev_inf_get();
	if(dp_data.SWITCH == TRUE){
		get_light_data();
		switch(dp_data.WORK_MODE)
		{
			case WHITE_MODE:
				light_data.LAST_RED_VAL = light_data.FIN_RED_VAL/RESO_VAL;
				light_data.LAST_GREEN_VAL = light_data.FIN_GREEN_VAL/RESO_VAL;
				light_data.LAST_BLUE_VAL = light_data.FIN_BLUE_VAL/RESO_VAL;
				light_data.LAST_WHITE_VAL = light_data.FIN_WHITE_VAL/RESO_VAL;
//				light_data.LAST_WARM_VAL = light_data.FIN_WARM_VAL/RESO_VAL;
				send_light_data(light_data.FIN_RED_VAL,light_data.FIN_GREEN_VAL,light_data.FIN_BLUE_VAL,light_data.FIN_WHITE_VAL,0);
				if(dp_data.MIC_SWITCH == TRUE)
					mic_music_handle.mic_scan_flag = TRUE;
				break;
			case COLOUR_MODE:
				light_data.LAST_RED_VAL = light_data.FIN_RED_VAL/RESO_VAL;
				light_data.LAST_GREEN_VAL = light_data.FIN_GREEN_VAL/RESO_VAL;
				light_data.LAST_BLUE_VAL = light_data.FIN_BLUE_VAL/RESO_VAL;
				light_data.LAST_WHITE_VAL = light_data.FIN_WHITE_VAL/RESO_VAL;
//				light_data.LAST_WARM_VAL = light_data.FIN_WARM_VAL/RESO_VAL;
				send_light_data(light_data.FIN_RED_VAL,light_data.FIN_GREEN_VAL,light_data.FIN_BLUE_VAL,light_data.FIN_WHITE_VAL,0);
				break;
			case SCENE_MODE:
				if(change_mode==statical)
				{
					light_data.LAST_RED_VAL = light_data.FIN_RED_VAL/RESO_VAL;
					light_data.LAST_GREEN_VAL = light_data.FIN_GREEN_VAL/RESO_VAL;
					light_data.LAST_BLUE_VAL = light_data.FIN_BLUE_VAL/RESO_VAL;
					light_data.LAST_WHITE_VAL = light_data.FIN_WHITE_VAL/RESO_VAL;
//					light_data.LAST_WARM_VAL = light_data.FIN_WARM_VAL/RESO_VAL;
					send_light_data(light_data.FIN_RED_VAL,light_data.FIN_GREEN_VAL,light_data.FIN_BLUE_VAL,light_data.FIN_WHITE_VAL,0);
				}
				else
				{
					light_data.LAST_RED_VAL = 0;
					light_data.LAST_GREEN_VAL = 0;
					light_data.LAST_BLUE_VAL = 0;
					light_data.LAST_WHITE_VAL = 0;
//					light_data.LAST_WARM_VAL = 0;
					flash_scene_flag = TRUE;
				}
				break;
			case MUSIC_MODE:
				mic_music_handle.mic_scan_flag = TRUE;
				send_light_data(800,200,0,0,0);
				break;
		}
	}
}

VOID light_init(VOID)
{
	OPERATE_RET op_ret;
	tuya_app_cfg_set(WCM_OLD, NULL);
//	tuya_app_cfg_set(WCM_SPCL_MODE, prod_test);

	ir_rx_init();
   	pwm_init(100, &pwm_val, CHAN_NUM, io_info);

	op_ret = tuya_psm_register_module(DEVICE_MOD, DEVICE_PART);
	if(op_ret != OPRT_OK && \
		op_ret != OPRT_PSM_E_EXIST) {
		PR_ERR("msf_register_module err:%d",op_ret);
		return;
	}

	op_ret = CreateMutexAndInit(&flash_scene_handle.mutex);
    if(op_ret != OPRT_OK) {
        return ;
    }

	gra_change.gra_key_sem = CreateSemaphore();
	if(NULL == gra_change.gra_key_sem){
		return ;
	}
    op_ret = InitSemaphore(gra_change.gra_key_sem,0,1);
    if(OPRT_OK != op_ret) {
        return ;
    }

   	hw_timer_init(1,hw_test_timer_cb);
	//1ms ?§?????
	hw_timer_arm(2000);
    hw_timer_disable();

   op_ret = CreateAndStart(&gra_change.gra_thread, light_gra_change, NULL,1024+512,TRD_PRIO_2,"gra_task");
   if(op_ret != OPRT_OK) {
       return ;
   }

   op_ret = CreateAndStart(&flash_scene_handle.flash_scene_thread, flash_scene_change, NULL,1024+512,TRD_PRIO_2,"flash_scene_task");
	if(op_ret != OPRT_OK) {
        return ;
    }

	op_ret = CreateAndStart(&mic_music_handle.mic_music_thread, mic_adc_change, NULL,1024+512,TRD_PRIO_2,"mic__task");
	if(op_ret != OPRT_OK) {
        return ;
    }

}

STATIC VOID reset_fsw_cnt_cb(UINT timerID,PVOID pTimerArg)
{
    PR_DEBUG("%s",__FUNCTION__);

    set_reset_cnt(0);
}

VOID dev_reset_judge(VOID)
{
	OPERATE_RET op_ret;
	struct rst_info *rst_inf = system_get_rst_info();
	PR_DEBUG("rst_inf:%d",rst_inf->reason);
	if((rst_inf->reason == REASON_DEFAULT_RST) || (rst_inf->reason == REASON_EXT_SYS_RST)) {
		set_reset_cnt(get_reset_cnt()+1);
	    TIMER_ID timer;
		op_ret = sys_add_timer(reset_fsw_cnt_cb,NULL,&timer);
		if(OPRT_OK != op_ret) {
			PR_ERR("reset_fsw_cnt timer add err:%d",op_ret);
		    return;
		}else {
		    sys_start_timer(timer,5000,TIMER_ONCE);
		}
	}
}

VOID pre_app_init(VOID)
{
	uint8 gpio_out_config[]={R_OUT_IO_NUM,G_OUT_IO_NUM,B_OUT_IO_NUM};
	set_gpio_out(gpio_out_config,CNTSOF(gpio_out_config));
}

VOID app_init(VOID)
{
	set_console(FALSE);
	light_init();
	reset_light_sta();
	dev_reset_judge();
}

VOID set_firmware_tp(IN OUT CHAR *firm_name, IN OUT CHAR *firm_ver)
{
    strcpy(firm_name,APP_BIN_NAME);
    strcpy(firm_ver,USER_SW_VER);
    return;
}

//查询回调函数，推送设备当前状态
STATIC VOID dp_qeury_cb(IN CONST TY_DP_QUERY_S *dp_qry)
{
	init_upload_proc();
	return;
}

/***********************************************************
*  Function: device_init
*  Input:
*  Output:
*  Return:
***********************************************************/
OPERATE_RET device_init(VOID)
{
    OPERATE_RET op_ret;

//    print_port_init(0);
//    user_uart_raw_init(74880);

    PR_NOTICE("Shenzhen Farylink Technology Co. Ltd.\r");
    PR_NOTICE("PID: %s\r",PRODECT_KEY);
    PR_NOTICE("fireware: %s %s\r",APP_BIN_NAME, USER_SW_VER);
    PR_NOTICE("compiled @ %s %s\r\n", __DATE__, __TIME__);

    op_ret = tuya_device_init(PRODECT_KEY,device_cb,USER_SW_VER);
    if(op_ret != OPRT_OK) {
        PR_ERR("smart_frame_init failed");
    }

	op_ret = device_differ_init();
    if(op_ret != OPRT_OK) {
        return op_ret;
    }

	TY_IOT_CBS_S wf_cbs = {
		.dev_dp_query_cb = dp_qeury_cb,
		.ug_reset_inform_cb = NULL,
	};
	gw_register_cbs(&wf_cbs);

    return op_ret;
}

STATIC OPERATE_RET device_differ_init(VOID)
{
    OPERATE_RET op_ret;

	if(get_reset_cnt() >= 3){
		set_reset_cnt(0);
        set_default_dp_data();
        tuya_dev_reset_factory();
	}

	if(UN_ACTIVE == tuya_get_gw_status()){
		set_default_dp_data();
	}

    //读取存储的信息
    dev_inf_get();

    op_ret = sys_add_timer(wfl_timer_cb,NULL,&timer);
    if(OPRT_OK != op_ret) {
        return op_ret;
    }else {
        sys_start_timer(timer,300,TIMER_CYCLE);
    }

	op_ret = sys_add_timer(idu_timer_cb,NULL,&timer_init_dpdata);
    if(OPRT_OK != op_ret) {
        return op_ret;
    }else {
        sys_start_timer(timer_init_dpdata,300,TIMER_CYCLE);
    }

    op_ret = sys_add_timer(wf_direct_timer_cb,NULL,&wf_stat_dir);
    if(OPRT_OK != op_ret) {
        return op_ret;
    }

	op_ret = sys_add_timer(data_save_timer_cb,NULL,&data_save_timer);
    if(OPRT_OK != op_ret) {
        return op_ret;
    }

    op_ret = sys_add_timer(count_timer_cb,NULL,&count_timer);
    if(OPRT_OK != op_ret) {
        PR_ERR("add syn_timer err");
        return op_ret;
    }else {
        sys_start_timer(count_timer,ONE_SEC*1000,TIMER_CYCLE);
    }

    op_ret = sys_add_timer(ir_rcvd_timer_cb,NULL,&(ir_rcvd_timer));
    if(OPRT_OK != op_ret) {
        return op_ret;
    }else {
        sys_start_timer(ir_rcvd_timer, 100, TIMER_CYCLE);
    }

    ty_msg.press_key_sem = CreateSemaphore();
    if( NULL == ty_msg.press_key_sem ) {
        return 0;
    }

    op_ret = InitSemaphore(ty_msg.press_key_sem,0,1);
    if(OPRT_OK != op_ret) {
        return op_ret;
    }

    // 取消连击，需在按键初始化前设置
    set_key_detect_time(0);

    // key process init
    op_ret = tuya_kb_init();
    if(OPRT_OK != op_ret) {
        return op_ret;
    }

    // register key to process
    op_ret = tuya_kb_reg_proc(WF_RESET_KEY,3000,key_process);
    if(OPRT_OK  != op_ret) {
        return op_ret;
    }

    op_ret = CreateAndStart(&ty_msg.msg_thread,msg_proc,NULL,1024,TRD_PRIO_2,"ty_task");
    if(op_ret != OPRT_OK) {
        return op_ret;
    }

    return OPRT_OK;
}
//mqtt连接成功后发送初始化消息
STATIC VOID idu_timer_cb(UINT timerID,PVOID pTimerArg)
{
    if(TRUE == tuya_get_cloud_stat()){
    	init_upload_proc();
    	sys_stop_timer(timer_init_dpdata);
    }
}

void hsv2rgb(float h, float s, float v)
{
	float h60;
	float f;
	int h60f;
	int hi;
	float r, g, b;
	float p, q, t;
	s/=1000;
	v/=1000;
	h60 = h / 60.0;
	h60f = h / 60;
	hi = (int)h60f % 6;
	f = h60 - h60f;
	p = v * (1 - s);
	q = v * (1 - f * s);
	t = v * (1 - (1 - f) * s);
	r = g = b = 0;
	if(hi == 0)
	{
		r = v;
		g = t;
		b = p;
	}
	else if(hi == 1)
	{
		r = q;
		g = v;
		b = p;
	}
	else if(hi == 2)
	{
		r = p;
		g = v;
		b = t;
	}
	else if(hi == 3)
	{
		r = p;
		g = q;
		b = v;
	}
	else if(hi == 4)
	{
		r = t;
		g = p;
		b = v;
	}
	else if(hi == 5)
	{
		r = v;
		g = p;
		b = q;
	}
	r = (r * 1000.0);
	g = (g * 1000.0);
	b = (b * 1000.0);
	color_r = r;
	color_g = g;
	color_b = b;
}

STATIC VOID get_light_data(VOID)
{
	float H,S,V;
	float color[8];
	INT i;
	UCHAR scene_num;
	switch(dp_data.WORK_MODE)
	{
		case WHITE_MODE:
			light_data.FIN_WHITE_VAL = dp_data.BRIGHT;
			light_data.FIN_RED_VAL = 0;
			light_data.FIN_GREEN_VAL = 0;
			light_data.FIN_BLUE_VAL = 0;
			light_data.RED_VAL = light_data.FIN_RED_VAL/RESO_VAL;
			light_data.GREEN_VAL = light_data.FIN_GREEN_VAL/RESO_VAL;
			light_data.BLUE_VAL = light_data.FIN_BLUE_VAL/RESO_VAL;
			light_data.WHITE_VAL = light_data.FIN_WHITE_VAL/RESO_VAL;
			break;
		case COLOUR_MODE:
			H=string_combine_short(asc2hex(dp_data.COLOUR_DATA[0]), asc2hex(dp_data.COLOUR_DATA[1]),asc2hex(dp_data.COLOUR_DATA[2]), asc2hex(dp_data.COLOUR_DATA[3]));
			S=string_combine_short(asc2hex(dp_data.COLOUR_DATA[4]), asc2hex(dp_data.COLOUR_DATA[5]),asc2hex(dp_data.COLOUR_DATA[6]), asc2hex(dp_data.COLOUR_DATA[7]));
			V=string_combine_short(asc2hex(dp_data.COLOUR_DATA[8]), asc2hex(dp_data.COLOUR_DATA[9]),asc2hex(dp_data.COLOUR_DATA[10]), asc2hex(dp_data.COLOUR_DATA[11]));
			hsv2rgb(H,S,V);
			light_data.FIN_RED_VAL=color_r;
			light_data.FIN_GREEN_VAL=color_g;
			light_data.FIN_BLUE_VAL=color_b;
			light_data.FIN_WHITE_VAL = 0;
			light_data.RED_VAL = light_data.FIN_RED_VAL/RESO_VAL;
			light_data.GREEN_VAL = light_data.FIN_GREEN_VAL/RESO_VAL;
			light_data.BLUE_VAL = light_data.FIN_BLUE_VAL/RESO_VAL;
			light_data.WHITE_VAL = light_data.FIN_WHITE_VAL/RESO_VAL;
			light_data.HUE =H;
			light_data.SATURATION = S;
			light_data.VALUE= V;
			break;
		case SCENE_MODE:
			scene_num = string_combine_byte(dp_data.SCENE_DATA[0],dp_data.SCENE_DATA[1]);
			switch(scene_num)
			{
			case 0x04:
				memset(dp_data.SCENE_ROUHE_DATA,0,sizeof(dp_data.SCENE_ROUHE_DATA));
				memcpy(dp_data.SCENE_ROUHE_DATA,dp_data.SCENE_DATA,sizeof(dp_data.SCENE_DATA));
				break;
			case 0x05:
				memset(dp_data.SCENE_BINFEN_DATA,0,sizeof(dp_data.SCENE_BINFEN_DATA));
				memcpy(dp_data.SCENE_BINFEN_DATA,dp_data.SCENE_DATA,sizeof(dp_data.SCENE_DATA));
				break;
			case 0x06:
				memset(dp_data.SCENE_XUANCAI_DATA,0,sizeof(dp_data.SCENE_XUANCAI_DATA));
				memcpy(dp_data.SCENE_XUANCAI_DATA,dp_data.SCENE_DATA,sizeof(dp_data.SCENE_DATA));
				break;
			case 0x07:
				memset(dp_data.SCENE_BANLAN_DATA,0,sizeof(dp_data.SCENE_BANLAN_DATA));
				memcpy(dp_data.SCENE_BANLAN_DATA,dp_data.SCENE_DATA,sizeof(dp_data.SCENE_DATA));
				break;
			default:
				break;
			}
			change_mode=string_combine_byte(dp_data.SCENE_DATA[6],dp_data.SCENE_DATA[7]);
			if(change_mode==statical)
			{
				H=string_combine_short(asc2hex(dp_data.SCENE_DATA[8]),asc2hex(dp_data.SCENE_DATA[9]),asc2hex(dp_data.SCENE_DATA[10]),asc2hex(dp_data.SCENE_DATA[11]));
				S=string_combine_short(asc2hex(dp_data.SCENE_DATA[12]),asc2hex(dp_data.SCENE_DATA[13]),asc2hex(dp_data.SCENE_DATA[14]),asc2hex(dp_data.SCENE_DATA[15]));
				V=string_combine_short(asc2hex(dp_data.SCENE_DATA[16]),asc2hex(dp_data.SCENE_DATA[17]),asc2hex(dp_data.SCENE_DATA[18]),asc2hex(dp_data.SCENE_DATA[19]));
				hsv2rgb(H,S,V);
				light_data.FIN_RED_VAL =color_r;
				light_data.FIN_GREEN_VAL =color_g;
				light_data.FIN_BLUE_VAL = color_b;
				flash_light_data.BRIGHT =string_combine_short(asc2hex(dp_data.SCENE_DATA[20]),asc2hex(dp_data.SCENE_DATA[21]),asc2hex(dp_data.SCENE_DATA[22]),asc2hex(dp_data.SCENE_DATA[23]));
				light_data.FIN_WHITE_VAL=flash_light_data.BRIGHT;
				light_data.RED_VAL = light_data.FIN_RED_VAL/RESO_VAL;
				light_data.GREEN_VAL = light_data.FIN_GREEN_VAL/RESO_VAL;
				light_data.BLUE_VAL = light_data.FIN_BLUE_VAL/RESO_VAL;
				light_data.WHITE_VAL = light_data.FIN_WHITE_VAL/RESO_VAL;
				light_data.HUE =H;
				light_data.SATURATION =S;
				light_data.VALUE=V;
			}
			else
			{
				light_data.FIN_WHITE_VAL = 0;
				light_data.WHITE_VAL = 0;
				light_data.FIN_RED_VAL=0;
				light_data.FIN_GREEN_VAL=0;
				light_data.FIN_BLUE_VAL=0;
				light_data.RED_VAL=0;
				light_data.GREEN_VAL=0;
				light_data.BLUE_VAL=0;
				light_data.LAST_RED_VAL = 0;
				light_data.LAST_GREEN_VAL = 0;
				light_data.LAST_BLUE_VAL = 0;
				light_data.LAST_WHITE_VAL = 0;
				flash_light_data.color_num=0;

				flash_light_data.BRIGHT = string_combine_short(asc2hex(dp_data.SCENE_DATA[20]), asc2hex(dp_data.SCENE_DATA[21]),asc2hex(dp_data.SCENE_DATA[22]),asc2hex(dp_data.SCENE_DATA[23]));
				flash_light_data.SPEED = 140 - (string_combine_byte(asc2hex(dp_data.SCENE_DATA[2]), asc2hex(dp_data.SCENE_DATA[3])));
				PR_DEBUG("flash_light_data.SPEED:%d",flash_light_data.SPEED);
				flash_light_data.NUM =(scene_len-2)/26;
				for(i=0; i<flash_light_data.NUM; i++)
				{
					H=string_combine_short(asc2hex(dp_data.SCENE_DATA[i*26+8]), asc2hex(dp_data.SCENE_DATA[i*26+9]),asc2hex(dp_data.SCENE_DATA[i*26+10]),asc2hex(dp_data.SCENE_DATA[i*26+11]));
					S=string_combine_short(asc2hex(dp_data.SCENE_DATA[i*26+12]), asc2hex(dp_data.SCENE_DATA[i*26+13]),asc2hex(dp_data.SCENE_DATA[i*26+14]),asc2hex(dp_data.SCENE_DATA[i*26+15]));
					V=string_combine_short(asc2hex(dp_data.SCENE_DATA[i*26+16]), asc2hex(dp_data.SCENE_DATA[i*26+17]),asc2hex(dp_data.SCENE_DATA[i*26+18]),asc2hex(dp_data.SCENE_DATA[i*26+19]));
					flash_light_data.BRIGHT =string_combine_short(asc2hex(dp_data.SCENE_DATA[i*26+20]),asc2hex(dp_data.SCENE_DATA[i*26+21]),asc2hex(dp_data.SCENE_DATA[i*26+22]),asc2hex(dp_data.SCENE_DATA[i*26+23]));
					hsv2rgb(H,S,V);
					flash_light_data.data_group[i].RED_VAL =color_r;
					flash_light_data.data_group[i].GREEN_VAL =color_g;
					flash_light_data.data_group[i].BLUE_VAL =color_b;
					flash_light_data.data_group[i].WHITE_VAL=flash_light_data.BRIGHT;
					color[i]=H;
				}
				for(i=0;i<flash_light_data.NUM-1;i++)
				{
					if(color[i]!=color[i+1])
					{
						flash_light_data.color_num++;
					}
				}
				light_data.FIN_WHITE_VAL = 0;
				light_data.WHITE_VAL = 0;
			}
			break;
		case MUSIC_MODE:
			H=string_combine_short(asc2hex(dp_data.MUSIC_DATA[1]), asc2hex(dp_data.MUSIC_DATA[2]),asc2hex(dp_data.MUSIC_DATA[3]), asc2hex(dp_data.MUSIC_DATA[4]));
			S=string_combine_short(asc2hex(dp_data.MUSIC_DATA[5]), asc2hex(dp_data.MUSIC_DATA[6]),asc2hex(dp_data.MUSIC_DATA[7]), asc2hex(dp_data.MUSIC_DATA[8]));
			V=string_combine_short(asc2hex(dp_data.MUSIC_DATA[9]), asc2hex(dp_data.MUSIC_DATA[10]),asc2hex(dp_data.MUSIC_DATA[11]), asc2hex(dp_data.MUSIC_DATA[12]));
			hsv2rgb(H,S,V);
			light_data.FIN_RED_VAL=color_r;
			light_data.FIN_GREEN_VAL=color_g;
			light_data.FIN_BLUE_VAL=color_b;
			light_data.FIN_WHITE_VAL = string_combine_short(asc2hex(dp_data.MUSIC_DATA[13]), asc2hex(dp_data.MUSIC_DATA[14]),asc2hex(dp_data.MUSIC_DATA[15]), asc2hex(dp_data.MUSIC_DATA[16]));;
			light_data.RED_VAL = light_data.FIN_RED_VAL/RESO_VAL;
			light_data.GREEN_VAL = light_data.FIN_GREEN_VAL/RESO_VAL;
			light_data.BLUE_VAL = light_data.FIN_BLUE_VAL/RESO_VAL;
			light_data.WHITE_VAL = light_data.FIN_WHITE_VAL/RESO_VAL;
			break;
		default:
			break;
	}

}

STATIC VOID set_default_dp_data(VOID)
{
	dp_data.SWITCH = TRUE;
	dp_data.MIC_SWITCH = FALSE;
	dp_data.WORK_MODE = WHITE_MODE;
	dp_data.BRIGHT = BRIGHT_INIT_VALUE;
	memcpy(dp_data.COLOUR_DATA, COLOUR_MODE_DEFAULT, 12);
	memcpy(dp_data.SCENE_DATA, "000e0d00002e03e8000000c80000", 28);
	memcpy(dp_data.SCENE_ROUHE_DATA, "04464602007803e803e800000000464602007803e8000a00000000", 54);
	memcpy(dp_data.SCENE_BINFEN_DATA, "05464601000003e803e800000000464601007803e803e80000000046460100f003e803e800000000464601003d03e803e80000000046460100ae03e803e800000000464601011303e803e800000000", 158);
	memcpy(dp_data.SCENE_XUANCAI_DATA, "06464601000003e803e800000000464601007803e803e80000000046460100f003e803e800000000", 80);
	memcpy(dp_data.SCENE_BANLAN_DATA, "07464602000003e803e800000000464602007803e803e80000000046460200f003e803e800000000464602003d03e803e80000000046460200ae03e803e800000000464602011303e803e800000000", 158);
	scene_len=28;
	dev_inf_set();
}

STATIC VOID start_gra_change(TIME_MS delay_time)
{
	gra_change.scale_flag = FALSE;
	hw_timer_arm(2000*delay_time);
    hw_timer_enable();
}

STATIC VOID hw_test_timer_cb(void)
{
	PostSemaphore(gra_change.gra_key_sem);
}


STATIC VOID light_gra_change(PVOID pArg)
{
	signed int delata_red = 0;
	signed int delata_green = 0;
    signed int delata_blue = 0;
    signed int delata_white = 0;
    UCHAR MAX_VALUE;
	STATIC FLOAT r_scale;
	STATIC FLOAT g_scale;
	STATIC FLOAT b_scale;
	STATIC FLOAT w_scale;
	STATIC FLOAT ww_scale;
	UINT RED_GRA_STEP = 1;
	UINT GREEN_GRA_STEP = 1;
	UINT BLUE_GRA_STEP = 1;
	UINT WHITE_GRA_STEP = 1;
	UINT WARM_GRA_STEP = 1;

	while(1)
	{
	    WaitSemaphore(gra_change.gra_key_sem);
		if(light_data.WHITE_VAL != light_data.LAST_WHITE_VAL || light_data.RED_VAL != light_data.LAST_RED_VAL ||\
			light_data.GREEN_VAL != light_data.LAST_GREEN_VAL || light_data.BLUE_VAL != light_data.LAST_BLUE_VAL)
	    {
			delata_red = light_data.RED_VAL - light_data.LAST_RED_VAL;
			delata_green = light_data.GREEN_VAL - light_data.LAST_GREEN_VAL;
			delata_blue = light_data.BLUE_VAL - light_data.LAST_BLUE_VAL;
			delata_white = light_data.WHITE_VAL - light_data.LAST_WHITE_VAL;
			MAX_VALUE = get_max_value(ABS(delata_red), ABS(delata_green), ABS(delata_blue), ABS(delata_white), 0);

			if(gra_change.scale_flag == FALSE){
				r_scale = ABS(delata_red)/1.0/MAX_VALUE;
				g_scale = ABS(delata_green)/1.0/MAX_VALUE;
				b_scale = ABS(delata_blue)/1.0/MAX_VALUE;
				w_scale = ABS(delata_white)/1.0/MAX_VALUE;
				gra_change.scale_flag = TRUE;
			}
			if(MAX_VALUE == ABS(delata_red)){
				RED_GRA_STEP = 1;
			}else{
				RED_GRA_STEP =  ABS(delata_red) - MAX_VALUE*r_scale;
			}
			if(MAX_VALUE == ABS(delata_green)){
				GREEN_GRA_STEP = 1;
			}else{
				GREEN_GRA_STEP =  ABS(delata_green) - MAX_VALUE*g_scale;
			}
			if(MAX_VALUE == ABS(delata_blue)){
				BLUE_GRA_STEP = 1;
			}else{
				BLUE_GRA_STEP =  ABS(delata_blue) - MAX_VALUE*b_scale;
			}
			if(MAX_VALUE == ABS(delata_white)){
				WHITE_GRA_STEP = 1;
			}else{
				WHITE_GRA_STEP =  ABS(delata_white) - MAX_VALUE*w_scale;
			}

			if(delata_red != 0){
			    if(ABS(delata_red) < RED_GRA_STEP)
			    {
					 light_data.LAST_RED_VAL += delata_red;
				}else{
					if(delata_red < 0)
						light_data.LAST_RED_VAL -= RED_GRA_STEP;
					else
						light_data.LAST_RED_VAL += RED_GRA_STEP;
				}
			}
			if(delata_green != 0){
			    if(ABS(delata_green) < GREEN_GRA_STEP)
			    {
					 light_data.LAST_GREEN_VAL += delata_green;
				}else{
					if(delata_green < 0)
						light_data.LAST_GREEN_VAL -= GREEN_GRA_STEP;
					else
						light_data.LAST_GREEN_VAL += GREEN_GRA_STEP;
				}
			}
			if(delata_blue != 0){
			    if(ABS(delata_blue) < BLUE_GRA_STEP)
			    {
					 light_data.LAST_BLUE_VAL += delata_blue;
				}else{
					if(delata_blue < 0)
						light_data.LAST_BLUE_VAL -= BLUE_GRA_STEP;
					else
						light_data.LAST_BLUE_VAL += BLUE_GRA_STEP;
				}
			}
			if(delata_white != 0){
			    if(ABS(delata_white) < WHITE_GRA_STEP)
			    {
					 light_data.LAST_WHITE_VAL += delata_white;
				}else{
					if(delata_white < 0)
						light_data.LAST_WHITE_VAL -= WHITE_GRA_STEP;
					else
						light_data.LAST_WHITE_VAL += WHITE_GRA_STEP;
				}
			}

			if((dp_data.SWITCH == FALSE) && ((dp_data.WORK_MODE==SCENE_MODE)&&(change_mode==gradual))){
				;
			}else{
				MutexLock(flash_scene_handle.mutex);
				if(dp_data.WORK_MODE!=SCENE_MODE||(dp_data.WORK_MODE==SCENE_MODE&&change_mode!=jump))
				{
					send_light_data(light_data.LAST_RED_VAL*RESO_VAL, light_data.LAST_GREEN_VAL*RESO_VAL, light_data.LAST_BLUE_VAL*RESO_VAL, light_data.LAST_WHITE_VAL*RESO_VAL, 0);
				}
				MutexUnLock(flash_scene_handle.mutex);
			}
		}else{
			if(dp_data.WORK_MODE!=SCENE_MODE||(dp_data.WORK_MODE==SCENE_MODE&&change_mode!=gradual)){
				send_light_data(light_data.FIN_RED_VAL, light_data.FIN_GREEN_VAL, light_data.FIN_BLUE_VAL, light_data.FIN_WHITE_VAL, 0);
				hw_timer_disable();
			}
		}
	}
}

STATIC VOID flash_scene_change(PVOID pArg)
{
	UINT require_time;
	while(1)
	{
		MutexLock(flash_scene_handle.mutex);
		if(dp_data.SWITCH == TRUE && flash_scene_flag == TRUE ){
			if(dp_data.WORK_MODE==SCENE_MODE)
			{
			switch(change_mode){
				case gradual:
					if(flash_light_data.color_num==0)
					{
						require_time = flash_light_data.SPEED*2+15;
					}
					else
					{
						require_time = flash_light_data.SPEED+10;
					}
					if(sta_cha_flag == TRUE)
					{
						irq_cnt = require_time;
						sta_cha_flag = FALSE;
					}
					else
					{
						irq_cnt ++;
					}
					if(irq_cnt >= require_time){
						light_data.FIN_RED_VAL = flash_light_data.data_group[num_cnt].RED_VAL;
						light_data.FIN_GREEN_VAL = flash_light_data.data_group[num_cnt].GREEN_VAL;
						light_data.FIN_BLUE_VAL = flash_light_data.data_group[num_cnt].BLUE_VAL;
						light_data.FIN_WHITE_VAL = flash_light_data.data_group[num_cnt].WHITE_VAL;
						light_data.RED_VAL = light_data.FIN_RED_VAL/RESO_VAL;
						light_data.GREEN_VAL = light_data.FIN_GREEN_VAL/RESO_VAL;
						light_data.BLUE_VAL = light_data.FIN_BLUE_VAL/RESO_VAL;
						light_data.WHITE_VAL = light_data.FIN_WHITE_VAL/RESO_VAL;
						num_cnt ++;
						if(num_cnt == flash_light_data.NUM)
							 num_cnt = 0;
						irq_cnt = 0;
						if(flash_light_data.color_num==0)
						{
							start_gra_change(flash_light_data.SPEED*2/5+2);
						}
						else
						{
							start_gra_change(flash_light_data.SPEED/7+3);
						}
					}
					break;
				case jump:
					require_time = flash_light_data.SPEED*2.5;
					if(sta_cha_flag == TRUE)
					{
						irq_cnt = require_time;
						sta_cha_flag = FALSE;
					}
					else
					{
						irq_cnt ++;
					}
					if(irq_cnt >= require_time)
					{
						if(dp_data.SWITCH == TRUE)
						{
							send_light_data(flash_light_data.data_group[num_cnt].RED_VAL, flash_light_data.data_group[num_cnt].GREEN_VAL, \
											flash_light_data.data_group[num_cnt].BLUE_VAL, flash_light_data.data_group[num_cnt].WHITE_VAL, 0);
						}
						num_cnt ++;
						if(num_cnt == flash_light_data.NUM)
						{
							num_cnt = 0;
						}
						irq_cnt = 0;
					}

					break;
				default:
					break;
			}
		  }
		}
		MutexUnLock(flash_scene_handle.mutex);
		SystemSleep(20);
	}
}



STATIC VOID wfl_timer_cb(UINT timerID,PVOID pTimerArg)
{
    OPERATE_RET op_ret;
    STATIC UINT last_wf_stat = 0xffffffff;
	STATIC BOOL config_flag = FALSE;
    GW_WIFI_STAT_E wf_stat = tuya_get_wf_status();
    if(last_wf_stat != wf_stat) {
		PR_DEBUG("size:%d",system_get_free_heap_size());
        switch(wf_stat) {
            case STAT_UNPROVISION: {
				config_flag = TRUE;
				flash_scene_flag = FALSE;
                sys_start_timer(wf_stat_dir, 250, TIMER_CYCLE);
            }
            break;

            case STAT_AP_STA_UNCONN: {
				 config_flag = TRUE;
				 flash_scene_flag = FALSE;
                 sys_start_timer(wf_stat_dir, 1500, TIMER_CYCLE);
            }
            break;

            case STAT_AP_STA_CFG_UNC:
            case STAT_STA_UNCONN:
			case STAT_LOW_POWER:
				 if(IsThisSysTimerRun(wf_stat_dir)){
				 	sys_stop_timer(wf_stat_dir);
				 }
				 if((wf_stat == STAT_STA_UNCONN) || (wf_stat == STAT_AP_STA_CFG_UNC)) {
				 	PR_DEBUG("config_flag:%d",config_flag);
				 	if(config_flag == TRUE){
						config_flag = FALSE;
						reset_light_sta();
						flash_scene_flag = TRUE;
					}
				    PR_DEBUG("STAT_STA_UNCONN");
                 }else {
                	send_light_data(BRIGHT_LOW_POWER, BRIGHT_LOW_POWER, BRIGHT_LOW_POWER, 0, 0);
                    PR_DEBUG("LOW POWER");
                 }
                 break;
            case STAT_STA_CONN:
            case STAT_AP_STA_CONN:{
            }
            break;
        }
        last_wf_stat = wf_stat;
    }
}

STATIC VOID wf_direct_timer_cb(UINT timerID,PVOID pTimerArg)
{
    STATIC INT flag = 0;
    if(flag == 0) {
        flag = 1;
		send_light_data(0, 0, 0, 0, 0);
    }else {
        flag = 0;
		send_light_data(BRIGHT_SMARTCONFIG, 0, 0, 0, 0);
    }
}

STATIC OPERATE_RET dev_inf_get(VOID)
{
    OPERATE_RET op_ret;

	UCHAR *buf;

	buf = Malloc(1378);

	if(NULL == buf) {
		PR_ERR("Malloc failed");
		goto ERR_EXT;
	}

	op_ret = msf_get_single(DEVICE_MOD, DP_DATA_KEY, buf, 1378);
	if(OPRT_OK != op_ret){
		PR_ERR("msf_get_single failed");
		Free(buf);
		goto ERR_EXT;
	}
	PR_DEBUG("buf:%s",buf);

	cJSON *root = NULL;
	root = cJSON_Parse(buf);
	if(NULL == root) {
		Free(buf);
		return OPRT_CJSON_PARSE_ERR;
	}
	Free(buf);

	cJSON *json;

	json = cJSON_GetObjectItem(root,"switch");
	if(NULL == json) {
		dp_data.SWITCH = TRUE;
	}else{
		dp_data.SWITCH = json->valueint;
	}
	if(FALSE == dp_data.SWITCH) {
		  struct rst_info *rst_inf = system_get_rst_info();
		  if((rst_inf->reason == REASON_DEFAULT_RST)  || (rst_inf->reason == REASON_EXT_SYS_RST)) {
			  dp_data.SWITCH = TRUE;
		  }
	 }

	json = cJSON_GetObjectItem(root,"work_mode");
	if(NULL == json) {
		dp_data.WORK_MODE = WHITE_MODE;
	}else{
		dp_data.WORK_MODE = json->valueint;
	}
	json = cJSON_GetObjectItem(root,"bright");
	if(NULL == json) {
		dp_data.BRIGHT = BRIGHT_INIT_VALUE;
	}else{
		dp_data.BRIGHT = json->valueint;
	}
	json = cJSON_GetObjectItem(root,"new_scene_len");
	if(NULL == json) {
		scene_len=28;
	}else{
		scene_len=json->valueint;
	}
	json = cJSON_GetObjectItem(root,"colour_data");
	if(NULL == json) {
		memcpy(dp_data.COLOUR_DATA, COLOUR_MODE_DEFAULT, 12);
	}else{
		memcpy(dp_data.COLOUR_DATA, json->valuestring, 12);
	}
	json = cJSON_GetObjectItem(root,"scene_data");
	if(NULL == json) {
		memcpy(dp_data.SCENE_DATA, "000e0d00002e03e8000000c80000", 28);
	}else{
		memcpy(dp_data.SCENE_DATA, json->valuestring, 210);
	}
	json = cJSON_GetObjectItem(root,"scene_rouhe_data");
	if(NULL == json) {
		memcpy(dp_data.SCENE_ROUHE_DATA, "04464602007803e803e800000000464602007803e8000a00000000", 54);
	}else{
		memcpy(dp_data.SCENE_ROUHE_DATA, json->valuestring, 210);
	}
	json = cJSON_GetObjectItem(root,"scene_binfen_data");
	if(NULL == json) {
		memcpy(dp_data.SCENE_BINFEN_DATA, "05464601000003e803e800000000464601007803e803e80000000046460100f003e803e800000000464601003d03e803e80000000046460100ae03e803e800000000464601011303e803e800000000", 158);
	}else{
		memcpy(dp_data.SCENE_BINFEN_DATA, json->valuestring, 210);
	}
	json = cJSON_GetObjectItem(root,"scene_xuancai_data");
	if(NULL == json) {
		memcpy(dp_data.SCENE_XUANCAI_DATA, "06464601000003e803e800000000464601007803e803e80000000046460100f003e803e800000000", 80);
	}else{
		memcpy(dp_data.SCENE_XUANCAI_DATA, json->valuestring, 210);
	}
	json = cJSON_GetObjectItem(root,"scene_banlan_data");
	if(NULL == json) {
		memcpy(dp_data.SCENE_BANLAN_DATA, "07464602000003e803e800000000464602007803e803e80000000046460200f003e803e800000000464602003d03e803e80000000046460200ae03e803e800000000464602011303e803e800000000", 158);
	}else{
		memcpy(dp_data.SCENE_BANLAN_DATA, json->valuestring, 210);
	}
	json = cJSON_GetObjectItem(root,"mic_switch");
	if(NULL == json) {
		dp_data.MIC_SWITCH = FALSE;
	}else{
		dp_data.MIC_SWITCH = json->valueint;
	}
	dp_data.APPT_SEC = 0;
	cJSON_Delete(root);
	return OPRT_OK;
ERR_EXT:
	set_default_dp_data();
	return OPRT_COM_ERROR;
}

STATIC OPERATE_RET dev_inf_set(VOID)
{
    OPERATE_RET op_ret;
    INT i = 0;
	CHAR *out = NULL;

	cJSON *root_test = NULL;
	root_test = cJSON_CreateObject();
	if(NULL == root_test) {
		PR_ERR("json creat failed");
		goto ERR_EXT;
	}
	cJSON_AddBoolToObject(root_test, "switch", dp_data.SWITCH);
	cJSON_AddNumberToObject(root_test, "work_mode", dp_data.WORK_MODE);
	cJSON_AddNumberToObject(root_test, "bright", dp_data.BRIGHT);
	cJSON_AddNumberToObject(root_test, "new_scene_len", scene_len);
	cJSON_AddStringToObject(root_test, "colour_data", dp_data.COLOUR_DATA);
	cJSON_AddStringToObject(root_test, "scene_data", dp_data.SCENE_DATA);
	cJSON_AddStringToObject(root_test, "scene_rouhe_data", dp_data.SCENE_ROUHE_DATA);
	cJSON_AddStringToObject(root_test, "scene_binfen_data", dp_data.SCENE_BINFEN_DATA);
	cJSON_AddStringToObject(root_test, "scene_xuancai_data", dp_data.SCENE_XUANCAI_DATA);
	cJSON_AddStringToObject(root_test, "scene_banlan_data", dp_data.SCENE_BANLAN_DATA);
	cJSON_AddBoolToObject(root_test, "mic_switch", dp_data.MIC_SWITCH);
	out=cJSON_PrintUnformatted(root_test);
	cJSON_Delete(root_test);
	if(NULL == out) {
		PR_ERR("cJSON_PrintUnformatted err:");
        Free(out);
		goto ERR_EXT;
	}
	PR_DEBUG("write psm[%s]", out);
	op_ret = msf_set_single(DEVICE_MOD, DP_DATA_KEY, out);
	if(OPRT_OK != op_ret) {
		PR_ERR("data write psm err: %d",op_ret);
		Free(out);
		goto ERR_EXT;
	}
	Free(out);
    return OPRT_OK;
ERR_EXT:
	return OPRT_COM_ERROR;
}


//第一次连接服务器时上传信息
STATIC VOID init_upload_proc(VOID)
{
    cJSON *root_test = NULL;
    OPERATE_RET op_ret;
    CHAR *out = NULL;

    GW_WIFI_STAT_E wf_stat = get_wf_gw_status();
    if((wf_stat < STAT_STA_CONN) || \
    		(get_gw_status() != STAT_WORK)) {
    	return;
    }

	DEV_CNTL_N_S *dev_cntl = get_single_wf_dev();
	if( dev_cntl == NULL )
		return;
	DP_CNTL_S *dp_cntl =  NULL;
	dp_cntl = &dev_cntl->dp[1];


	root_test = cJSON_CreateObject();
	if(NULL == root_test) {
		return;
	}
	cJSON_AddBoolToObject(root_test, "20", dp_data.SWITCH);
	cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
    cJSON_AddNumberToObject(root_test, "22", dp_data.BRIGHT);
	cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
	cJSON_AddStringToObject(root_test, "25", dp_data.SCENE_DATA);
	cJSON_AddNumberToObject(root_test, "26", dp_data.APPT_SEC);


    out=cJSON_PrintUnformatted(root_test);
	cJSON_Delete(root_test);
	if(NULL == out) {
		PR_ERR("cJSON_PrintUnformatted err:");
        Free(out);
		return;
	}
	PR_DEBUG("out[%s]", out);
    op_ret = tuya_obj_dp_report(out);
    if(OPRT_OK != op_ret) {
        PR_ERR("sf_obj_dp_report err:%d",op_ret);
        Free(out);
        return;
    }
	Free(out);
	return;

}

STATIC VOID work_mode_change(SCENE_MODE_E mode)
{
	if(dp_data.WORK_MODE == mode){
		;
    }else {
    	mic_music_handle.mic_scan_flag = FALSE;
    	dp_data.WORK_MODE = mode;
    	MutexLock(flash_scene_handle.mutex);
		get_light_data();
		switch(mode)
		{
			case WHITE_MODE:
				if(dp_data.SWITCH == TRUE)
				{
					start_gra_change(NORMAL_DELAY_TIME);
					if(dp_data.MIC_SWITCH == TRUE)
					{
						mic_music_handle.mic_scan_flag = TRUE;
					}
				}
				if(dp_data.MIC_SWITCH == TRUE)
				{
					mic_music_handle.mic_scan_flag = TRUE;
				}
				break;
			case COLOUR_MODE:
				if(dp_data.SWITCH == TRUE)
				{
					start_gra_change(NORMAL_DELAY_TIME);
				}
				break;
			case SCENE_MODE:
				if(change_mode==statical)
				{
					if(dp_data.SWITCH == TRUE)
					{
						start_gra_change(NORMAL_DELAY_TIME);
					}
				}
				else
				{
					if(dp_data.SWITCH == TRUE)
					{
						sta_cha_flag = TRUE;
					}
					else
					{
						sta_cha_flag = FALSE;
					}
					num_cnt = 0;
					flash_dir = 0;
					light_data.LAST_RED_VAL = 0;
					light_data.LAST_GREEN_VAL = 0;
					light_data.LAST_BLUE_VAL = 0;
					light_data.LAST_WHITE_VAL = 0;
					send_light_data(0, 0, 0, 0, 0);
				}
				break;
			default:
				break;
		}
		MutexUnLock(flash_scene_handle.mutex);
    }
}

STATIC VOID sl_datapoint_proc(cJSON *root)
{
	UCHAR dpid, type;
	WORD len, rawlen;
    dpid = atoi(root->string);
    switch(dpid) {
        case DPID_0:			//开关
			switch(root->type) {
			    case cJSON_False:
			        //关灯
			        dp_data.SWITCH = FALSE;
					MutexLock(flash_scene_handle.mutex);
					light_data.RED_VAL = 0;
					light_data.GREEN_VAL = 0;
					light_data.BLUE_VAL = 0;
					light_data.WHITE_VAL = 0;
					light_data.FIN_RED_VAL = 0;
					light_data.FIN_GREEN_VAL = 0;
					light_data.FIN_BLUE_VAL = 0;
					light_data.FIN_WHITE_VAL = 0;
					switch(dp_data.WORK_MODE)
					{
						case WHITE_MODE:
						case COLOUR_MODE:
						case MUSIC_MODE:
							MutexUnLock(flash_scene_handle.mutex);
							start_gra_change(NORMAL_DELAY_TIME);
							break;
						case SCENE_MODE:
							MutexUnLock(flash_scene_handle.mutex);
							if(change_mode==statical)
							{
								start_gra_change(NORMAL_DELAY_TIME);
							}
							if(change_mode==gradual)
							{
								hw_timer_disable();
								send_light_data(light_data.RED_VAL, light_data.GREEN_VAL, light_data.BLUE_VAL, light_data.WHITE_VAL, 0);
							}
							if(change_mode==jump)
							{
								send_light_data(light_data.RED_VAL, light_data.GREEN_VAL, light_data.BLUE_VAL, light_data.WHITE_VAL, 0);
							}
							MutexUnLock(flash_scene_handle.mutex);
							break;
					}
					dp_data.APPT_SEC = 0;
					msg_upload_sec();
			        break;
			    case cJSON_True:
			        //开灯
			        MutexLock(flash_scene_handle.mutex);
					dp_data.SWITCH = TRUE;
					get_light_data();
					switch(dp_data.WORK_MODE)
					{
						case WHITE_MODE:
						case COLOUR_MODE:
						case MUSIC_MODE:
							MutexUnLock(flash_scene_handle.mutex);
							start_gra_change(NORMAL_DELAY_TIME);
							break;
						case SCENE_MODE:
							if(change_mode==statical)
							{
								start_gra_change(NORMAL_DELAY_TIME);
							}
							else
							{
								sta_cha_flag = TRUE;
								num_cnt = 0;
								flash_dir = 0;
							}
							MutexUnLock(flash_scene_handle.mutex);
							break;
					}
					dp_data.APPT_SEC = 0;
					msg_upload_sec();
			        break;
			    default:
			        break;
			}
			break;
		case DPID_1:						//模式
			if(dp_data.SWITCH== FALSE){
			   break;
			}
			work_mode_change(ty_get_enum_id(atoi(root->string), root->valuestring));
            break;
        case DPID_2:						//亮度
			//白光模式下的数据
			if(root->valueint < 10 || root->valueint >1000){
				PR_ERR("the data length is wrong: %d", root->valueint);
			}else {
				dp_data.BRIGHT = (root->valueint);
				get_light_data();
				 if((dp_data.SWITCH == TRUE) && (dp_data.MIC_SWITCH == TRUE))
				 {
					 start_gra_change(NORMAL_DELAY_TIME);
				 }
			}

			break;

		case DPID_4:				 //彩光
			len = strlen(root->valuestring);
			if(len != 12){
				PR_ERR("the data length is wrong: %d", len);
		    }else {
		    	memcpy(dp_data.COLOUR_DATA, root->valuestring, len);
				get_light_data();
				 if(dp_data.SWITCH == TRUE)
				 {
					 start_gra_change(NORMAL_DELAY_TIME);
				 }
			}
		break;
		case DPID_5:				//场景
			scene_len= strlen(root->valuestring);
			memset(dp_data.SCENE_DATA,0,sizeof(dp_data.SCENE_DATA));
			memcpy(dp_data.SCENE_DATA, root->valuestring, scene_len);
			get_light_data();
			if(change_mode==statical)
			{
				if(dp_data.SWITCH == TRUE)
				{
					start_gra_change(NORMAL_DELAY_TIME);
				}
			}
			else
			{
				if(change_mode==jump)
				{
					hw_timer_disable();
				}
				if(dp_data.SWITCH == TRUE)
				{
					sta_cha_flag = TRUE;
				}
				else
				{
					sta_cha_flag = FALSE;
				}
				num_cnt = 0;
				flash_dir = 0;
			}
		break;
		case DPID_6:
			if(root->valueint < 0 || root->valueint >86400){
				PR_ERR("the data length is wrong: %d", root->valueint);
		    }else {
		    	dp_data.APPT_SEC = (root->valueint) == 0 ? 0:(wmtime_time_get_posix() + root->valueint);
		    }
			break;
		case DPID_7:			//音乐
			len = strlen(root->valuestring);
	    	memcpy(dp_data.MUSIC_DATA, root->valuestring, len);
			get_light_data();
			if(dp_data.SWITCH == TRUE) {
			send_light_data(light_data.FIN_RED_VAL,light_data.FIN_GREEN_VAL,light_data.FIN_BLUE_VAL,light_data.FIN_WHITE_VAL,0);
			}
			break;
		case DPID_8:
			len = strlen(root->valuestring);
			if(len != 21){
				PR_ERR("the data length is wrong: %d", len);
		    }else {
		    	UCHAR CONTROL_TEMP[21];
		    	if(dp_data.WORK_MODE == COLOUR_MODE)
		    	{
		    		memcpy(dp_data.COLOUR_DATA, root->valuestring+1, 12);
		    	}
		    	else if(dp_data.WORK_MODE == WHITE_MODE)
		    	{
		    		memcpy(CONTROL_TEMP, root->valuestring, 21);
		    		dp_data.BRIGHT=string_combine_short(asc2hex(CONTROL_TEMP[13]), asc2hex(CONTROL_TEMP[14]),asc2hex(CONTROL_TEMP[15]), asc2hex(CONTROL_TEMP[16]));
		    	}

				get_light_data();
				if(dp_data.SWITCH == TRUE)
				{
					start_gra_change(NORMAL_DELAY_TIME);
				}
			}
			break;
		case DPID_9:
			switch(root->type) {
			    case cJSON_False:
			    	dp_data.MIC_SWITCH = FALSE;
			    	mic_music_handle.mic_scan_flag = FALSE;
			    	get_light_data();
			    	start_gra_change(NORMAL_DELAY_TIME);
			    	break;
			    case cJSON_True:
			    	dp_data.MIC_SWITCH = TRUE;
			    	mic_music_handle.mic_scan_flag = TRUE;
			    	break;
			}
			break;
         default:
                break;
    }
}

STATIC VOID msg_send(INT cmd)
{
	if(cmd == dp_data.SWITCH) {
		msg_upload_proc();
	}else {
        if(dp_data.SWITCH != FALSE) {
        	dp_data.SWITCH = FALSE;
        }else {
        	dp_data.SWITCH = TRUE;
        }

        //根据power状态控制开关并清除相应的倒计时数值
	    cJSON *root_test = NULL;
		root_test = cJSON_CreateObject();
		if(NULL == root_test) {
			return;
		}

		cJSON_AddBoolToObject(root_test, "20", dp_data.SWITCH);
		device_cb(LAN_CMD, root_test);
		cJSON_Delete(root_test);
	}
}

STATIC INT msg_upload_sec(VOID)
{
    GW_WIFI_STAT_E wf_stat = tuya_get_wf_status();
    if((wf_stat < STAT_STA_CONN) || \
       (tuya_get_gw_status() != STAT_WORK)) {
        return WM_FAIL;
    }

    cJSON *root = cJSON_CreateObject();
    if(NULL == root) {
        return WM_FAIL;
    }

    cJSON_AddNumberToObject(root, "26", dp_data.APPT_SEC>0?(dp_data.APPT_SEC - wmtime_time_get_posix()):0);

    CHAR *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if(NULL == out) {
        PR_ERR("cJSON_PrintUnformatted err:");
        return WM_FAIL;
    }

    OPERATE_RET op_ret;
    op_ret = tuya_obj_dp_report(out);
    PR_DEBUG("msg_upload_sec %s",out);
    Free(out);
    if( OPRT_OK == op_ret ) {
        return WM_SUCCESS;
    }else {
        return WM_FAIL;
    }
}

STATIC VOID count_timer_cb(UINT timerID,PVOID pTimerArg)
{
	if(!dp_data.APPT_SEC == 0)
	{
		INT count_temp = 0;
		INT cur_posix = wmtime_time_get_posix();
		INT power_tmp;

		if( cur_posix < dp_data.APPT_SEC){
			if( (dp_data.APPT_SEC - cur_posix)%60 == 0 ) {
				//60秒更新一次
				PR_DEBUG("count_timer_cb");
				msg_upload_sec();
			}
		}else {
			dp_data.APPT_SEC = 0;
			if(dp_data.SWITCH != 0) {
				power_tmp = 0;
			}else {
				power_tmp = 1;
			}
			//到时控制开关
			msg_send(power_tmp);
			PR_DEBUG("count down...");
			msg_upload_sec();
		}
	}
}


typedef struct
{
	float real;
	float img;
}complex;
#define NPT		16								//FFT采样点数
#define log2N	4
long lBUFMAG[NPT];								//求模后数据
complex lBUFOUT[NPT];//FFT输出序列
complex lBUFIN[NPT];//FFT输入序列
/*复数类型*/


complex plural_change(long plural)
{
	complex temp;
	temp.real = plural / 1024.0;
	temp.img = 0;
	return temp;
}
/*复数加法*/
complex add(complex a,complex b)
{
	complex c;
	c.real=a.real+b.real;
	c.img=a.img+b.img;
	return c;
}
/*复数减法*/
complex sub(complex a,complex b)
{
	complex c;
	c.real=a.real-b.real;
	c.img=a.img-b.img;
	return c;
}
/*复数乘法*/
complex mul(complex a,complex b)
{
	complex c;
	c.real=a.real*b.real - a.img*b.img;
	c.img=a.real*b.img + a.img*b.real;
	return c;
}
/***码位倒序函数***/
void Reverse(void)
{
	unsigned int i,j,k;
	unsigned int t;
	complex temp;//临时交换变量
	for(i=0;i<NPT;i++)//从第0个序号到第N-1个序号
	{
		k = i;//当前第i个序号
		j = 0;//存储倒序后的序号，先初始化为0
		for(t = 0;t < log2N;t++)//共移位t次，其中log2N是事先宏定义算好的
		{
			j <<= 1;
			j |= (k & 1);//j左移一位然后加上k的最低位
			k >>= 1;//k右移一位，次低位变为最低位
		}
		if(j > i)//如果倒序后大于原序数，就将两个存储单元进行交换（判断j>i是为了防止重复交换）
		{
			temp = lBUFIN[i];
			lBUFIN[i] = lBUFIN[j];
			lBUFIN[j] = temp;
		}
	}
}
/********************************************************
 为节省CPU计算时间，旋转因子采用查表处理
根据实际FFT的点数N，该表数据需自行修改
以下结果通过Excel自动生成
WN[k].real=cos(2*PI/N*k);
WN[k].img=-sin(2*PI/N*k);
**********************************************************/
complex WN[NPT]=//旋转因子数组
{
  {1.00000,0.00000},{0.92388,0.38268},{0.70711,0.70711},{0.382683,0.92388},
  {0.00000,1.00000},{-0.38268,0.92388},{-0.70711,0.70711},{-0.92388,0.38268},
  {-1.00000,0.00000},{-0.92388,-0.38268},{-0.70711,-0.70711},{-0.38268,-0.92388},
  {0.00000,-1.00000},{0.382683,-0.92388},{0.70711,-0.70711},{0.92388,-0.38268},

};
void FFT(void)
{
	unsigned int i,j,k,l;
	complex top,bottom,xW;
	Reverse();					//码位倒序
	for(i = 0;i < log2N;i++)		/*共log2N级*/
	{							//一级蝶形运算
		l = 1 << i;					//l等于2的i次方
		for(j = 0;j < NPT;j += 2*l)	/*每L个蝶形是一组，每级有N/2L组*/
		{						//一组蝶形运算
			for(k = 0;k < l;k++)	/*每组有L个*/
			{					//一个蝶形运算
				xW = mul(lBUFIN[j+k+l],WN[NPT/(2*l)*k]); //碟间距为l
				top = add(lBUFIN[j+k],xW); //每组的第k个蝶形
				bottom = sub(lBUFIN[j+k],xW);
				lBUFIN[j+k] = top;
				lBUFIN[j+k+l] = bottom;
			}
		}
	}
	memcpy(lBUFOUT,lBUFIN,NPT*8);
}


float InvSqrt(float x)
{
    float xhalf = 0.5f * x;
    int i = *(int*)&x;
    i = 0x5f375a86 - (i>>1);
    x = *(float*)&i;
    x = x*(1.5f-xhalf*x*x);
    x = x*(1.5f-xhalf*x*x);
    x = x*(1.5f-xhalf*x*x);

    return 1/x;
}

void powerMag()
{	 float X,Y;
	uint32_t i;
	for (i=0;i<16;i++)
	{
		X= lBUFOUT[i].real; /* sine_cosine --> cos */
		Y= lBUFOUT[i].img;   /* sine_cosine --> sin */
		{
			float Mag = InvSqrt(X*X+ Y*Y)/16;
			lBUFMAG[i] =(long)(Mag*1024);
		}
	}
}

CHAR vChange_Cnt;
UINT last_adc_value;
STATIC VOID mic_adc_change(PVOID pArg)
{
	while(1)
	{
		if((dp_data.SWITCH == TRUE) && (mic_music_handle.mic_scan_flag == TRUE))
		{
			INT i;
			PR_DEBUG("system_adc_read:%d",system_adc_read());
//			os_printf("system_adc_read:%d\n",system_adc_read());

			for(i = 0;i < 16;i++)
			{

				lBUFIN[i] =plural_change(system_adc_read());
			}
//			for(i = 0;i < 16;i++)
//			{
//				PR_DEBUG("lBUFIN[%d]:%d",i,lBUFIN[i].real);
//			}
			FFT();
			powerMag();
			int a,b,c,max;
			for(i = 0;i < 16;i++)
			{
				PR_DEBUG("lBUFMAG[%d]:%d",i,lBUFMAG[i]);
			}
			a = get_max_value(lBUFMAG[1],lBUFMAG[2],lBUFMAG[3],lBUFMAG[4],lBUFMAG[5]);
			b = get_max_value(lBUFMAG[6],lBUFMAG[7],lBUFMAG[8],lBUFMAG[9],lBUFMAG[10]);
			c = get_max_value(lBUFMAG[11],lBUFMAG[12],lBUFMAG[13],lBUFMAG[14],lBUFMAG[15]);
			max = get_max_value(a,b,c,0,0);
			PR_DEBUG("max:%d",i,max);
			if (max > 0)
//			if(ABS(system_adc_read() - last_adc_value)>= 80)
//			if((system_adc_read() - last_adc_value)>= 100)
			{
				switch(vChange_Cnt++)
				{
				case 1:
					send_light_data(800,200,0,0,0);
					break;
				case 2:
					send_light_data(0,1000,0,0,0);
					break;
				case 3:
					send_light_data(0,650,350,0,0);
					break;
				case 4:
					send_light_data(0,0,0,800,0);
					break;
				case 5:
					send_light_data(200,0,800,0,0);
					break;
				case 6:
					send_light_data(350,250,400,0,0);
					break;
				case 7:
					send_light_data(400,600,0,0,0);
					break;
				case 8:
					send_light_data(50,800,150,0,0);
					break;
				case 9:
					send_light_data(600,200,200,0,0);
					break;
				case 10:
					send_light_data(100,800,100,0,0);
					break;
				case 11:
					send_light_data(1000,0,0,0,0);
					vChange_Cnt = 0;
					break;
				default:
					break;
				}
			}
//			last_adc_value = system_adc_read();
		}
		SystemSleep(20);
	}
}

STATIC VOID key_process(INT gpio_no,PUSH_KEY_TYPE_E type,INT cnt)
{
    PR_DEBUG("gpio_no: %d",gpio_no);
    PR_DEBUG("type: %d",type);
    PR_DEBUG("cnt: %d",cnt);
    PR_DEBUG("remain memory:%d",system_get_free_heap_size());

    if(WF_RESET_KEY == gpio_no) {
        if(LONG_KEY == type) {
            tuya_dev_reset_factory();
        }else if(NORMAL_KEY == type) {
            PostSemaphore(ty_msg.press_key_sem);
        }
    }
}

STATIC VOID msg_proc(PVOID pArg)
{
	while(1)
	{
		WaitSemaphore(ty_msg.press_key_sem);

		cJSON *root_test = NULL;
		root_test = cJSON_CreateObject();
		if(NULL == root_test){
			return;
		}

		if(dp_data.SWITCH == TRUE){
			dp_data.SWITCH = FALSE;
			cJSON_AddBoolToObject(root_test, "20", dp_data.SWITCH);
		}else{
			dp_data.SWITCH = TRUE;
			cJSON_AddBoolToObject(root_test, "20", dp_data.SWITCH);
		}

		device_cb(LAN_CMD, root_test);
		cJSON_Delete(root_test);
	}
}

STATIC VOID ir_rcvd_timer_cb(UINT timerID,PVOID pTimerArg)
{
	if(IR_RX_BUFF.fill_cnt)
	{
		while (IR_RX_BUFF.fill_cnt)
		{
			uint8 ir_data;
			UINT color_bright;

		    cJSON *root_test = NULL;
			root_test = cJSON_CreateObject();
			if(NULL == root_test) {
				return;
			}

			DEV_CNTL_N_S *dev_cntl = get_single_wf_dev();
			if( dev_cntl == NULL )
				return;
			DP_CNTL_S *dp_cntl =  NULL;
			dp_cntl = &dev_cntl->dp[1];

			RINGBUF_Get(&IR_RX_BUFF, &ir_data, 1);
			PR_DEBUG("IR CMD : %02x", ir_data);

			switch(ir_data)
			{
			case IR_CODE_BRIGHT_UP:
				if(dp_data.WORK_MODE == WHITE_MODE)
				{
					dp_data.BRIGHT = dp_data.BRIGHT>=900?1000:dp_data.BRIGHT+100;
					cJSON_AddNumberToObject(root_test, "22", dp_data.BRIGHT);
				}
				else if(dp_data.WORK_MODE == COLOUR_MODE)
				{
					char color_v[1];
					color_bright = string_combine_short(asc2hex(dp_data.COLOUR_DATA[8]), asc2hex(dp_data.COLOUR_DATA[9]),
							asc2hex(dp_data.COLOUR_DATA[10]), asc2hex(dp_data.COLOUR_DATA[11]));
					color_bright = color_bright>=900?1000:color_bright+100;
					Int2HexStr(color_bright,color_v);
					dp_data.COLOUR_DATA[8] = color_v[4];
					dp_data.COLOUR_DATA[9] = color_v[5];
					dp_data.COLOUR_DATA[10] = color_v[6];
					dp_data.COLOUR_DATA[11] = color_v[7];
					cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				}
				break;
			case IR_CODE_BRIGHT_DOWN:
				if(dp_data.WORK_MODE == WHITE_MODE)
				{
					dp_data.BRIGHT = dp_data.BRIGHT<=100?10:dp_data.BRIGHT-100;
					cJSON_AddNumberToObject(root_test, "22", dp_data.BRIGHT);
				}
				else if(dp_data.WORK_MODE == COLOUR_MODE)
				{
					char color_v[1];
					color_bright = string_combine_short(asc2hex(dp_data.COLOUR_DATA[8]), asc2hex(dp_data.COLOUR_DATA[9]),
							asc2hex(dp_data.COLOUR_DATA[10]), asc2hex(dp_data.COLOUR_DATA[11]));
					color_bright = color_bright<=100?10:color_bright-100;
					Int2HexStr(color_bright,color_v);
					dp_data.COLOUR_DATA[8] = color_v[4];
					dp_data.COLOUR_DATA[9] = color_v[5];
					dp_data.COLOUR_DATA[10] = color_v[6];
					dp_data.COLOUR_DATA[11] = color_v[7];
					cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				}
				break;
			case IR_CODE_POWER_OFF:
				dp_data.SWITCH = FALSE;
				cJSON_AddBoolToObject(root_test, "20", dp_data.SWITCH);
				break;
			case IR_CODE_POWER_ON:
				dp_data.SWITCH = TRUE;
				cJSON_AddBoolToObject(root_test, "20", dp_data.SWITCH);
				break;
			case IR_CODE_R:
				dp_data.WORK_MODE = COLOUR_MODE;
				memcpy(dp_data.COLOUR_DATA, "000003e803e8", 12);
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				break;
			case IR_CODE_G:
				dp_data.WORK_MODE = COLOUR_MODE;
				memcpy(dp_data.COLOUR_DATA, "007803e803e8", 12);
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				break;
			case IR_CODE_B:
				dp_data.WORK_MODE = COLOUR_MODE;
				memcpy(dp_data.COLOUR_DATA, "00f003e803e8", 12);
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				break;
			case IR_CODE_W:
				dp_data.WORK_MODE = WHITE_MODE;
				dp_data.BRIGHT = 1000;
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddNumberToObject(root_test, "22", dp_data.BRIGHT);
				break;
			case IR_CODE_COLOR1:
				dp_data.WORK_MODE = COLOUR_MODE;
				memcpy(dp_data.COLOUR_DATA, "000f03e803e8", 12);
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				break;
			case IR_CODE_COLOR2:
				dp_data.WORK_MODE = COLOUR_MODE;
				memcpy(dp_data.COLOUR_DATA, "008703e803e8", 12);
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				break;
			case IR_CODE_COLOR3:
				dp_data.WORK_MODE = COLOUR_MODE;
				memcpy(dp_data.COLOUR_DATA, "00ff03e803e8", 12);
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				break;
			case IR_CODE_COLOR4:
				dp_data.WORK_MODE = COLOUR_MODE;
				memcpy(dp_data.COLOUR_DATA, "001e03e803e8", 12);
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				break;
			case IR_CODE_COLOR5:
				dp_data.WORK_MODE = COLOUR_MODE;
				memcpy(dp_data.COLOUR_DATA, "009603e803e8", 12);
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				break;
			case IR_CODE_COLOR6:
				dp_data.WORK_MODE = COLOUR_MODE;
				memcpy(dp_data.COLOUR_DATA, "010e03e803e8", 12);
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				break;
			case IR_CODE_COLOR7:
				dp_data.WORK_MODE = COLOUR_MODE;
				memcpy(dp_data.COLOUR_DATA, "002d03e803e8", 12);
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				break;
			case IR_CODE_COLOR8:
				dp_data.WORK_MODE = COLOUR_MODE;
				memcpy(dp_data.COLOUR_DATA, "00a503e803e8", 12);
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				break;
			case IR_CODE_COLOR9:
				dp_data.WORK_MODE = COLOUR_MODE;
				memcpy(dp_data.COLOUR_DATA, "011d03e803e8", 12);
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				break;
			case IR_CODE_COLOR10:
				dp_data.WORK_MODE = COLOUR_MODE;
				memcpy(dp_data.COLOUR_DATA, "003c03e803e8", 12);
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				break;
			case IR_CODE_COLOR11:
				dp_data.WORK_MODE = COLOUR_MODE;
				memcpy(dp_data.COLOUR_DATA, "00b403e803e8", 12);
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				break;
			case IR_CODE_COLOR12:
				dp_data.WORK_MODE = COLOUR_MODE;
				memcpy(dp_data.COLOUR_DATA, "012c03e803e8", 12);
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "24", dp_data.COLOUR_DATA);
				break;
			case IR_CODE_FLASH:
				dp_data.WORK_MODE = SCENE_MODE;
				memset(dp_data.SCENE_DATA,0,sizeof(dp_data.SCENE_DATA));
				memcpy(dp_data.SCENE_DATA,dp_data.SCENE_ROUHE_DATA,sizeof(dp_data.SCENE_DATA));
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "25", dp_data.SCENE_DATA);
				break;
			case IR_CODE_STROBE:
				dp_data.WORK_MODE = SCENE_MODE;
				memset(dp_data.SCENE_DATA,0,sizeof(dp_data.SCENE_DATA));
				memcpy(dp_data.SCENE_DATA,dp_data.SCENE_BINFEN_DATA,sizeof(dp_data.SCENE_DATA));
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "25", dp_data.SCENE_DATA);
				break;
			case IR_CODE_FADE:
				dp_data.WORK_MODE = SCENE_MODE;
				memset(dp_data.SCENE_DATA,0,sizeof(dp_data.SCENE_DATA));
				memcpy(dp_data.SCENE_DATA,dp_data.SCENE_XUANCAI_DATA,sizeof(dp_data.SCENE_DATA));
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "25", dp_data.SCENE_DATA);
				break;
			case IR_CODE_SMOOTH:
				dp_data.WORK_MODE = SCENE_MODE;
				memset(dp_data.SCENE_DATA,0,sizeof(dp_data.SCENE_DATA));
				memcpy(dp_data.SCENE_DATA,dp_data.SCENE_BANLAN_DATA,sizeof(dp_data.SCENE_DATA));
				cJSON_AddStringToObject(root_test,"21",ty_get_enum_str(dp_cntl,(UCHAR)dp_data.WORK_MODE));
				cJSON_AddStringToObject(root_test, "25", dp_data.SCENE_DATA);
				break;
			default:
				break;
			}

			device_cb(LAN_CMD, root_test);
			cJSON_Delete(root_test);
		}
	}
}


STATIC INT ty_get_enum_id(UCHAR dpid, UCHAR *enum_str)
{
	UCHAR i = 0;
	UCHAR enum_id = 0;
	DP_CNTL_S *dp_cntl =  NULL;
	DEV_CNTL_N_S *dev_cntl = get_single_wf_dev();

	for(i = 0; i < dev_cntl->dp_num; i++) {
		if(dev_cntl->dp[i].dp_desc.dp_id == dpid) {
			dp_cntl = &dev_cntl->dp[i];
			break;
		}
	}

	if(i >= dev_cntl->dp_num) {
		PR_ERR("not find enum_str");
		return -1;
	}

	if( dp_cntl == NULL ) {
		PR_ERR("dp_cntl is NULL");
		return -1;
	}

	for( i = 0; i < dp_cntl->prop.prop_enum.cnt; i++ )
	{
		if( strcmp(enum_str, dp_cntl->prop.prop_enum.pp_enum[i]) == 0 )
			break;
	}

	return i;
}

STATIC UCHAR *ty_get_enum_str(DP_CNTL_S *dp_cntl, UCHAR enum_id)
{
	if( dp_cntl == NULL ) {
		return NULL;
	}

	if( enum_id >= dp_cntl->prop.prop_enum.cnt ) {
		return NULL;
	}

	return dp_cntl->prop.prop_enum.pp_enum[enum_id];
}





