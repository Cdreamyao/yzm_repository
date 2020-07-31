#include "ddl.h"
#include "uart.h"
#include "bt.h"
#include "lpm.h"
#include "gpio.h"
#include "stdio.h"
#include "wifi.h"
#include "pca.h"
#include "adt.h"
#include "flash.h"

extern DP_DEF dp_data;
extern LIGHT_DATA_DEF light_data;
extern unsigned char change_mode;
extern char sta_cha_flag;
extern unsigned int irq_cnt;
extern unsigned int num_cnt;
volatile static int flash_scene_flag = 1;
unsigned int require_time;
extern unsigned char scene_len;
extern uint8_t control;
extern uint8_t receive_finish;
static uint8_t color;
static uint8_t bright;
static uint8_t temperature;
extern uint16_t RF433_Addr;
extern uint16_t RF433_Group_Value;
extern uint16_t RF433_Group_New_Value;
uint16_t Flash_433_addr1;
uint16_t Flash_433_addr2;
uint16_t Flash_433_addr3;
char RF433_match_enble;
uint8_t RF433_NUM;
extern uint8_t bt_wifi_mode;

void scene_scan();
void set_default_value();
void set_flash_data();
void get_flash_data();
void get_scene_data();
void RF433_read();
void key_scan();
void CLK_SET_INIT();
void GPIO_INIT();
extern void my_hex_to_str(unsigned char *hex,unsigned char *str,int len);
extern void get_light_data();
uint32_t       Start_Addr   	 		  = 0x0007800;
uint32_t       SWITCH_Addr   	 		  = 0x0007802;
uint32_t       BRIGHT_Addr    			= 0x0007804;
uint32_t			 COL_TEMPERATURE_Addr	= 0x0007806;
uint32_t			 WORK_MODE_Addr				= 0x0007815;
uint32_t			 COLOUR_DATA_Addr			= 0x0007820;
uint32_t 			 scene_len_Addr				= 0x0007a00;
uint32_t			 SCENE_DATA_Addr			= 0x0007A02;
uint32_t			 RF_433_Addr1					= 0x0007C00;
uint32_t			 RF_433_Addr2					= 0x0007C50;
uint32_t			 RF_433_Addr3					= 0x0007CA0;
uint32_t			 RF_433_AddrNUM				= 0x0007E00;
uint32_t			 RF_433_Group_Addr		= 0x0007600;

/*************************************************************************************
函数功能: 设置组号
输入参数: 无
输出参数: 无
返 回 值: 无
备    注: 将得到的组号键值存入Flash中
*************************************************************************************/
void set_RF433_Group()									
{
	Flash_SectorErase(RF_433_Group_Addr);
	Flash_WriteHalfWord(RF_433_Group_Addr,RF433_Group_Value);
}

/*************************************************************************************
函数功能: 获得组号
输入参数: 无
输出参数: 无
返 回 值: 无
备    注: 从Flash中读取组号
*************************************************************************************/
void get_RF433_Group_Value()
{
	RF433_Group_Value = Flash_readByte(RF_433_Group_Addr);
}

/*************************************************************************************
函数功能: 设置遥控器地址
输入参数: 无
输出参数: 无
返 回 值: 无
备    注: 将获得的遥控器地址按顺序依次存入相应的Flash中
*************************************************************************************/
void set_RF433_Addr()
{
	Flash_SectorErase(RF_433_Addr1);
	if(RF433_NUM==1)
	{
		Flash_WriteHalfWord(RF_433_Addr1,RF433_Addr);
	}
	else if(RF433_NUM==2)
	{
		Flash_WriteHalfWord(RF_433_Addr1,Flash_433_addr1);
		Flash_WriteHalfWord(RF_433_Addr2,RF433_Addr);
	}
	else if(RF433_NUM==3)
	{
		Flash_WriteHalfWord(RF_433_Addr1,Flash_433_addr1);
		Flash_WriteHalfWord(RF_433_Addr2,Flash_433_addr2);
		Flash_WriteHalfWord(RF_433_Addr3,RF433_Addr);
	}
	else if(RF433_NUM>3)
	{
		Flash_WriteHalfWord(RF_433_Addr1,Flash_433_addr1);
		Flash_WriteHalfWord(RF_433_Addr2,Flash_433_addr2);
		Flash_WriteHalfWord(RF_433_Addr3,RF433_Addr);
	}

}

/*************************************************************************************
函数功能: 设置遥控器编号
输入参数: 无
输出参数: 无
返 回 值: 无
备    注: 将遥控器编号存入相应的Flash中
*************************************************************************************/
void set_RF433_NUM()
{
	Flash_SectorErase(RF_433_AddrNUM);
	Flash_WriteByte(RF_433_AddrNUM,RF433_NUM);
}

/*************************************************************************************
函数功能: 获得遥控器编号
输入参数: 无
输出参数: 无
返 回 值: 无
备    注: 从Flash中读取遥控器编号
*************************************************************************************/
void get_RF433_NUM()
{
	RF433_NUM=Flash_readByte(RF_433_AddrNUM);
	if(RF433_NUM<0||RF433_NUM>3)
	{
		RF433_NUM=0;
		set_RF433_NUM();
	}
}

/*************************************************************************************
函数功能: 获得遥控器地址
输入参数: 无
输出参数: 无
返 回 值: 无
备    注: 从Flash中读取存放三个遥控器地址
*************************************************************************************/
void get_RF433_Addr()
{
	Flash_433_addr1=Flash_readHalfWord(RF_433_Addr1);
	Flash_433_addr2=Flash_readHalfWord(RF_433_Addr2);
	Flash_433_addr3=Flash_readHalfWord(RF_433_Addr3);
}

/*************************************************************************************
函数功能: 设置灯光数据
输入参数: 无
输出参数: 无
返 回 值: 无
备    注: 将灯光数据存入Flash中
*************************************************************************************/
void set_flash_data()
{
	int i;
  Flash_SectorErase(Start_Addr);
	Flash_WriteByte(Start_Addr,0X1);
	Flash_WriteByte(SWITCH_Addr,dp_data.SWITCH);
	Flash_WriteHalfWord(BRIGHT_Addr,dp_data.BRIGHT);
	Flash_WriteHalfWord(COL_TEMPERATURE_Addr,dp_data.COL_TEMPERATURE);
	Flash_WriteByte(WORK_MODE_Addr,dp_data.WORK_MODE);
	for(i=0;i<12;i++)
	{
		Flash_WriteByte(COLOUR_DATA_Addr+i,dp_data.COLOUR_DATA[i]);	
	}
}

/*************************************************************************************
函数功能: 获得灯光数据
输入参数: 无
输出参数: 无
返 回 值: 无
备    注: 从Flash中读取灯光数据
*************************************************************************************/
void get_flash_data()
{
	int i;
	int start_flag;
	start_flag=Flash_readByte(Start_Addr);
	if(start_flag==0x1)
	{
		dp_data.SWITCH=Flash_readByte(SWITCH_Addr);
		dp_data.BRIGHT=Flash_readHalfWord(BRIGHT_Addr);
		dp_data.COL_TEMPERATURE=Flash_readHalfWord(COL_TEMPERATURE_Addr);
		dp_data.WORK_MODE=Flash_readByte(WORK_MODE_Addr);		
		for(i=0;i<12;i++)
		{
			dp_data.COLOUR_DATA[i]=Flash_readByte(COLOUR_DATA_Addr+i);
		}
	}
	else{
		set_default_value();
	}	
}

/*************************************************************************************
函数功能: 获得场景数据
输入参数: 无
输出参数: 无
返 回 值: 无
备    注: 从Flash中读取场景数据
*************************************************************************************/
void get_scene_data()
{
	int i;
	scene_len=Flash_readByte(scene_len_Addr);	
	for(i=0;i<scene_len;i++)
	{
		dp_data.SCENE_DATA[i]=Flash_readByte(SCENE_DATA_Addr+i*2);
	}
}

int32_t main(void)
{
	//模块初始化	
	CLK_SET_INIT();
	Clk_SetPeripheralGate(ClkPeripheralGpio, TRUE);
	Clk_SetPeripheralGate(ClkPeripheralBt, TRUE);
	
	Flash_Init(FlashInt, 3);
	get_flash_data();
	get_scene_data();
	get_RF433_NUM();
	get_RF433_Addr();
	get_RF433_Group_Value();
	pwm_init();
	send_light_data(0,0,0,0,0);
	get_light_data();
	send_light_data(light_data.RED_VAL,light_data.GREEN_VAL,light_data.BLUE_VAL,light_data.WHITE_VAL,light_data.WARM_VAL);
	GPIO_INIT();
//	uart0_init();
	uart1_init();
//	printf("start\r\n");
	wifi_protocol_init();
	Timer0_init(80);
//	adt_timer_ms_init(AdTIM4,10);
	delay1ms(200);
	while(1)
	{
		wifi_uart_service();			//wifi扫描
		scene_scan();							//场景扫描
		RF433_read();							//遥控器扫描
		key_scan();								//按键扫描
	}
}

/*************************************************************************************
函数功能: 设置灯光数据为默认值
输入参数: 无
输出参数: 无
返 回 值: 无
备    注: 无
*************************************************************************************/
void set_default_value()
{
	dp_data.SWITCH=1;
	dp_data.BRIGHT=1000;
	dp_data.COL_TEMPERATURE=1000;
	dp_data.WORK_MODE=WHITE_MODE;
	dp_data.COLOUR_DATA[0]=0x0;
	dp_data.COLOUR_DATA[1]=0x0;
	dp_data.COLOUR_DATA[2]=0x0;
	dp_data.COLOUR_DATA[3]=0x0;
	dp_data.COLOUR_DATA[4]=0x0;
	dp_data.COLOUR_DATA[5]=0x3;
	dp_data.COLOUR_DATA[6]=0xe;
	dp_data.COLOUR_DATA[7]=0x8;
	dp_data.COLOUR_DATA[8]=0x0;
	dp_data.COLOUR_DATA[9]=0x3;
	dp_data.COLOUR_DATA[10]=0xe;
	dp_data.COLOUR_DATA[11]=0x8;
	dp_data.SCENE_DATA[0]=0x0;
	dp_data.SCENE_DATA[1]=0x0;
	dp_data.SCENE_DATA[2]=0x0;
	dp_data.SCENE_DATA[3]=0xe;
	dp_data.SCENE_DATA[4]=0x0;
	dp_data.SCENE_DATA[5]=0xd;
	dp_data.SCENE_DATA[6]=0x0;
	dp_data.SCENE_DATA[7]=0x0;
	dp_data.SCENE_DATA[8]=0x0;
	dp_data.SCENE_DATA[9]=0x0;
	dp_data.SCENE_DATA[10]=0x2;
	dp_data.SCENE_DATA[11]=0xe;
	dp_data.SCENE_DATA[12]=0x0;
	dp_data.SCENE_DATA[13]=0x3;
	dp_data.SCENE_DATA[14]=0xe;
	dp_data.SCENE_DATA[15]=0x8;
	dp_data.SCENE_DATA[16]=0x0;
	dp_data.SCENE_DATA[17]=0x0;
	dp_data.SCENE_DATA[18]=0x0;
	dp_data.SCENE_DATA[19]=0x0;
	dp_data.SCENE_DATA[20]=0x0;
	dp_data.SCENE_DATA[21]=0x0;
	dp_data.SCENE_DATA[22]=0xc;
	dp_data.SCENE_DATA[23]=0x8;
	dp_data.SCENE_DATA[24]=0x0;
	dp_data.SCENE_DATA[25]=0x0;
	dp_data.SCENE_DATA[26]=0x0;
	dp_data.SCENE_DATA[27]=0x0;
	scene_len=28;
	set_flash_data();
	set_scene_data();

	get_light_data();
	send_light_data(light_data.RED_VAL,light_data.GREEN_VAL,light_data.BLUE_VAL,light_data.WHITE_VAL,light_data.WARM_VAL);
}

/*************************************************************************************
函数功能: clk初始化
输入参数: 无
输出参数: 无
返 回 值: 无
备    注: 无
*************************************************************************************/
void CLK_SET_INIT()
{
	    //CLK INIT
    stc_clk_config_t stcClkCfg;
    stcClkCfg.enClkSrc  = ClkRCH;
    stcClkCfg.enHClkDiv = ClkDiv1;
    stcClkCfg.enPClkDiv = ClkDiv1;

    Clk_Init(&stcClkCfg);
}

/*************************************************************************************
函数功能: GPIO口初始化
输入参数: 无
输出参数: 无
返 回 值: 无
备    注: 无
*************************************************************************************/
void GPIO_INIT()
{
	Gpio_InitIO(0, 1, GpioDirIn);
	Gpio_InitIO(0, 2, GpioDirIn);
	Gpio_InitIO(1, 4, GpioDirIn);
	Gpio_InitIO(1, 5, GpioDirIn);
	Gpio_InitIO(2, 3, GpioDirIn);
	Gpio_InitIO(2, 7, GpioDirIn);
	Gpio_InitIO(3, 1, GpioDirIn);
	Gpio_InitIO(3, 4, GpioDirIn);
}

/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/

/*************************************************************************************
函数功能: 场景扫描函数
输入参数: 无
输出参数: 无
返 回 值: 无
备    注: 无
*************************************************************************************/
void scene_scan()
{
		if(dp_data.SWITCH == 1 && flash_scene_flag == 1 )
		{
			if(dp_data.WORK_MODE==SCENE_MODE)
			{
			switch(change_mode){
				case gradual:
					if(flash_light_data.color_num==0)
					{
						require_time = flash_light_data.SPEED*1.5;
					}
					else
					{
						require_time = flash_light_data.SPEED*1.2;
					}
					if(irq_cnt >= require_time){
						light_data.RED_VAL = flash_light_data.data_group[num_cnt].RED_VAL;
						light_data.GREEN_VAL = flash_light_data.data_group[num_cnt].GREEN_VAL;
						light_data.BLUE_VAL = flash_light_data.data_group[num_cnt].BLUE_VAL;
						light_data.WHITE_VAL = flash_light_data.data_group[num_cnt].WHITE_VAL;
						light_data.WARM_VAL = flash_light_data.data_group[num_cnt].WARM_VAL;
						num_cnt ++;
						if(num_cnt == flash_light_data.NUM)
							 num_cnt = 0;
						irq_cnt = 0;
						if(flash_light_data.color_num==0)
						{
							Timer2_init(10);
						}
						else
						{
							Timer2_init(6);
						}
					}
					break;
				case jump:
					require_time = flash_light_data.SPEED*3.8;
					if(irq_cnt >= require_time)
					{
						if(dp_data.SWITCH == 1)
						{
							send_light_data(flash_light_data.data_group[num_cnt].RED_VAL, flash_light_data.data_group[num_cnt].GREEN_VAL,flash_light_data.data_group[num_cnt].BLUE_VAL, flash_light_data.data_group[num_cnt].WHITE_VAL, flash_light_data.data_group[num_cnt].WARM_VAL);
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
}

/*************************************************************************************
函数功能: 433扫描函数
输入参数: 无
输出参数: 无
返 回 值: 无
备    注: 无
*************************************************************************************/
void RF433_read()
{
	if(receive_finish==1)
	{
		if(RF433_match_enble==0)
		{
			if(Flash_433_addr1==RF433_Addr||Flash_433_addr2==RF433_Addr||Flash_433_addr3==RF433_Addr)
			{
				switch (control)
				{				
					case 0x12:						
					case 0x11:
					case 0x04:
					case 0x0F:				
						RF433_Group_New_Value = control;
						break;
					default:
						break;
				}
				get_RF433_Group_Value();
				if(RF433_Group_Value == RF433_Group_New_Value)
				{
					switch(control)
					{
						case 0x10:			//开关
							if(dp_data.SWITCH==0)
							{
								dp_data.SWITCH=1;
								get_light_data();
								Timer2_init(6);
				//				send_light_data(light_data.RED_VAL,light_data.GREEN_VAL,light_data.BLUE_VAL,light_data.WHITE_VAL,light_data.WARM_VAL);
							}
							else 
							{
								dp_data.SWITCH = 0;
								light_data.RED_VAL = 0;
								light_data.GREEN_VAL = 0;
								light_data.BLUE_VAL = 0;
								light_data.WHITE_VAL = 0;
								light_data.WARM_VAL = 0;
								switch(dp_data.WORK_MODE)
								{
									case WHITE_MODE:
									case COLOUR_MODE:
											Timer2_init(6);
											break;
									case SCENE_MODE:
										if(change_mode==statical)
										{
												Timer2_init(6);
										}
										if(change_mode==gradual)
										{
											Bt_Stop(TIM2);
											send_light_data(light_data.RED_VAL, light_data.GREEN_VAL, light_data.BLUE_VAL, light_data.WHITE_VAL, light_data.WARM_VAL);
										}
										if(change_mode==jump)
										{
											send_light_data(light_data.RED_VAL, light_data.GREEN_VAL, light_data.BLUE_VAL, light_data.WHITE_VAL, light_data.WARM_VAL);
										}
										break;
								}
							}
							break;
						case 0x15:														//亮度+
							if(dp_data.SWITCH==1&&dp_data.WORK_MODE==WHITE_MODE)
							{
								switch(dp_data.BRIGHT)
								{
									case 100:
										bright = 0;
										break;
									
									case 300:
										bright = 1;
										break;
									
									case 500:
										bright = 2;
										break;
									
									case 700:
										bright = 3;
										break;
									
									case 1000:
										bright = 4;
										break;
									
									default:
										break;
								}
								if(bright < 4)
								{
									bright++;
								}
								switch(bright)
								{
									case 0:
										dp_data.BRIGHT = 100;
										break;
									
									case 1:
										dp_data.BRIGHT = 300;
										break;
									
									case 2:
										dp_data.BRIGHT = 500;
										break;
									
									case 3:
										dp_data.BRIGHT = 700;
										break;
									
									case 4:
										dp_data.BRIGHT = 1000;
										break;
									
									default:
										break;
								}
	//							dp_data.BRIGHT+=200;
	//							if(dp_data.BRIGHT>1000)
	//							{
	//								dp_data.BRIGHT=1000;
	//							}
								get_light_data();
								Timer2_init(6);
		//						send_light_data(light_data.RED_VAL,light_data.GREEN_VAL,light_data.BLUE_VAL,light_data.WHITE_VAL,light_data.WARM_VAL);
							}
							break;
						case 0x0C:																				//亮度-
							if(dp_data.SWITCH==1&&dp_data.WORK_MODE==WHITE_MODE)
							{
								switch(dp_data.BRIGHT)
								{
									case 100:
										bright = 0;
										break;
									
									case 300:
										bright = 1;
										break;
									
									case 500:
										bright = 2;
										break;
									
									case 700:
										bright = 3;
										break;
									
									case 1000:
										bright = 4;
										break;
									
									default:
										break;
								}
								if(bright > 0 )
								{
									bright--;
								}
								switch(bright)
								{
									case 0:
										dp_data.BRIGHT = 100;
										break;
									
									case 1:
										dp_data.BRIGHT = 300;
										break;
									
									case 2:
										dp_data.BRIGHT = 500;
										break;
									
									case 3:
										dp_data.BRIGHT = 700;
										break;
									
									case 4:
										dp_data.BRIGHT = 1000;
										break;
									
									default:
										break;
								}
	//							dp_data.BRIGHT-=200;
	//							if(dp_data.BRIGHT<10)
	//							{
	//								dp_data.BRIGHT=10;
	//							}
								get_light_data();
								Timer2_init(6);
		//						send_light_data(light_data.RED_VAL,light_data.GREEN_VAL,light_data.BLUE_VAL,light_data.WHITE_VAL,light_data.WARM_VAL);
							}
							break;
						case 0x01:															//RGBCW循环
							if(dp_data.SWITCH==1)
							{
								color++;
								switch(color%5)
								{
									case 1:
										dp_data.WORK_MODE=COLOUR_MODE;
										dp_data.COLOUR_DATA[0]=0x0;
										dp_data.COLOUR_DATA[1]=0x0;
										dp_data.COLOUR_DATA[2]=0x0;
										dp_data.COLOUR_DATA[3]=0x0;
										dp_data.COLOUR_DATA[4]=0x0;
										dp_data.COLOUR_DATA[5]=0x3;
										dp_data.COLOUR_DATA[6]=0xe;
										dp_data.COLOUR_DATA[7]=0x8;
										dp_data.COLOUR_DATA[8]=0x0;
										dp_data.COLOUR_DATA[9]=0x3;
										dp_data.COLOUR_DATA[10]=0xe;
										dp_data.COLOUR_DATA[11]=0x8;
										break;
									case 2:
										dp_data.WORK_MODE=COLOUR_MODE;
										dp_data.COLOUR_DATA[0]=0x0;
										dp_data.COLOUR_DATA[1]=0x0;
										dp_data.COLOUR_DATA[2]=0x7;
										dp_data.COLOUR_DATA[3]=0x8;
										dp_data.COLOUR_DATA[4]=0x0;
										dp_data.COLOUR_DATA[5]=0x3;
										dp_data.COLOUR_DATA[6]=0xe;
										dp_data.COLOUR_DATA[7]=0x8;
										dp_data.COLOUR_DATA[8]=0x0;
										dp_data.COLOUR_DATA[9]=0x3;
										dp_data.COLOUR_DATA[10]=0xe;
										dp_data.COLOUR_DATA[11]=0x8;
										break;
									case 3:
										dp_data.WORK_MODE=COLOUR_MODE;
										dp_data.COLOUR_DATA[0]=0x0;
										dp_data.COLOUR_DATA[1]=0x0;
										dp_data.COLOUR_DATA[2]=0xf;
										dp_data.COLOUR_DATA[3]=0x0;
										dp_data.COLOUR_DATA[4]=0x0;
										dp_data.COLOUR_DATA[5]=0x3;
										dp_data.COLOUR_DATA[6]=0xe;
										dp_data.COLOUR_DATA[7]=0x8;
										dp_data.COLOUR_DATA[8]=0x0;
										dp_data.COLOUR_DATA[9]=0x3;
										dp_data.COLOUR_DATA[10]=0xe;
										dp_data.COLOUR_DATA[11]=0x8;					
										break;
									case 4:
										dp_data.WORK_MODE=WHITE_MODE;
										dp_data.COL_TEMPERATURE=1000;
										dp_data.BRIGHT=1000;
										break;
									case 0:
										dp_data.WORK_MODE=WHITE_MODE;
										dp_data.COL_TEMPERATURE=0;
										dp_data.BRIGHT=1000;
										color=0;
										break;
									default:
										color=0;
										break;
							}
								get_light_data();
								Timer2_init(6);
		//						send_light_data(light_data.RED_VAL,light_data.GREEN_VAL,light_data.BLUE_VAL,light_data.WHITE_VAL,light_data.WARM_VAL);
							}
							break;
						case 0x13:																				//色温循环
							if(dp_data.SWITCH==1&&dp_data.WORK_MODE==WHITE_MODE)
							{
								if(dp_data.COL_TEMPERATURE == 0)
								{
									temperature = 0;
								}
								else if(dp_data.COL_TEMPERATURE == 1000)
								{
									temperature = 1;
								}
								temperature++;
								switch(temperature%5)
								{
									case 1:
										dp_data.COL_TEMPERATURE=1000;
										break;
									
									case 2:
										dp_data.COL_TEMPERATURE=750;
										break;
									
									case 3:
										dp_data.COL_TEMPERATURE=500;
										break;
									
									case 4:
										dp_data.COL_TEMPERATURE=250;
										break;
									
									case 0:
										dp_data.COL_TEMPERATURE=0;
										temperature=0;
										break;
									
									default:
										break;
								}
								get_light_data();
								Timer2_init(6);
		//						send_light_data(light_data.RED_VAL,light_data.GREEN_VAL,light_data.BLUE_VAL,light_data.WHITE_VAL,light_data.WARM_VAL);
							}
							break;

							
							
						default:
							break;
						}
				}

					set_flash_data();
				}
		}
		else if(RF433_match_enble==1)										//433配对状态
		{
			RF433_match_enble=0;
			if((RF433_Addr!=Flash_433_addr1)&&(RF433_Addr!=Flash_433_addr2)&&(RF433_Addr!=Flash_433_addr3)) //没录入过的遥控器
			{
				get_RF433_NUM();
				RF433_NUM++;
				if(RF433_NUM>3)
				{
					RF433_NUM=3;
				}
				if(control == 0x12 || control == 0x11 || control == 0x04 || control == 0x0F)	//判断是否为分组按键
				{
					RF433_Group_Value = control;
					RF433_Group_New_Value = RF433_Group_Value;
				}
				set_RF433_NUM();
				set_RF433_Addr();
				set_RF433_Group();
			}
			else													//已录入的遥控器更改分组
			{
				if(control == 0x12 || control == 0x11 || control == 0x04 || control == 0x0F)
				{
					RF433_Group_Value = control;
					RF433_Group_New_Value = RF433_Group_Value;
				}
				set_RF433_Group();
			}
			Adt_StopCount(AdTIM5);
			get_light_data();
			send_light_data(light_data.RED_VAL,light_data.GREEN_VAL,light_data.BLUE_VAL,light_data.WHITE_VAL,light_data.WARM_VAL);
			light_data.LAST_RED_VAL=light_data.RED_VAL;
			light_data.LAST_GREEN_VAL=light_data.GREEN_VAL;
			light_data.LAST_BLUE_VAL=light_data.BLUE_VAL;
			light_data.LAST_WHITE_VAL=light_data.WHITE_VAL;
			light_data.LAST_WARM_VAL=light_data.WARM_VAL;			
			get_RF433_Addr();
		}
		delay1ms(100);
		receive_finish=0;
	}
}

/*************************************************************************************
函数功能: 按键扫描函数
输入参数: 无
输出参数: 无
返 回 值: 无
备    注: 无
*************************************************************************************/
void key_scan()
{
	static uint8_t delay_cnt;
	if(KEY_NET==0)
	{
		delay1ms(50);
		if(KEY_NET==0)
		{
			while(KEY_NET==0)													//检测配网键按下时长
			{
				delay1ms(100);
				delay_cnt++;
				if(delay_cnt==50)
				{	
					adt_timer_ms_init(AdTIM5,250);
				}
				else if(delay_cnt>=100)
				{
					if(KEY_BT==1&&bt_wifi_mode==1)
					{
						adt_timer_ms_init(AdTIM5,1500);
					}
					else if(bt_wifi_mode==0)
					{
						Adt_StopCount(AdTIM5);
						break;
					}
				}
				else if(delay_cnt>250)
				{break;}
			}
			if(delay_cnt>=50&&delay_cnt<100)					//配网键按下5~10秒，进入配网状态
			{
				delay_cnt=0;
				set_default_value();
				if(KEY_BT==0)
				{
					mcu_reset_wifi();
				}
				else
				{
					mcu_set_wifi_mode(SMART_CONFIG);
				}
				if(bt_wifi_mode==0)
				{
					RF433_match_enble=1;
				}
			}
			else if(delay_cnt>=100)										//配网键按下10秒，重置WiFi，清空433数据
			{
				delay_cnt=0;
				if(KEY_BT==1&&bt_wifi_mode==1)
				{
					set_default_value();
					mcu_set_wifi_mode(AP_CONFIG);
				}
				else if(bt_wifi_mode==0)
				{
					Flash_433_addr1=0;
					Flash_433_addr2=0;
					Flash_433_addr3=0;
					Flash_SectorErase(RF_433_Addr1);
					Flash_SectorErase(RF_433_AddrNUM);
					get_light_data();
					Timer2_init(6);
				}
			}
			else
			{
				delay_cnt=0;
			}
		}
	}
	
	if(KEY3==0)					//开关
	{
		delay1ms(100);
		if(KEY3==0)
		{
			if(dp_data.SWITCH==0)
			{
				dp_data.SWITCH=1;
				get_light_data();
				Timer2_init(6);
//				send_light_data(light_data.RED_VAL,light_data.GREEN_VAL,light_data.BLUE_VAL,light_data.WHITE_VAL,light_data.WARM_VAL);
			}
			else 
			{
				dp_data.SWITCH = 0;
				light_data.RED_VAL = 0;
				light_data.GREEN_VAL = 0;
				light_data.BLUE_VAL = 0;
				light_data.WHITE_VAL = 0;
				light_data.WARM_VAL = 0;
				switch(dp_data.WORK_MODE)
				{
					case WHITE_MODE:
					case COLOUR_MODE:
							Timer2_init(6);
							break;
					case SCENE_MODE:
						if(change_mode==statical)
						{
								Timer2_init(6);
						}
						if(change_mode==gradual)
						{
							Bt_Stop(TIM2);
							send_light_data(light_data.RED_VAL, light_data.GREEN_VAL, light_data.BLUE_VAL, light_data.WHITE_VAL, light_data.WARM_VAL);
						}
						if(change_mode==jump)
						{
							send_light_data(light_data.RED_VAL, light_data.GREEN_VAL, light_data.BLUE_VAL, light_data.WHITE_VAL, light_data.WARM_VAL);
						}
						break;
				}
			}
			if(KEY_BT==0)
			{
				mcu_dp_bool_update(BT_DPID_SWITCH_LED,dp_data.SWITCH);
			}
			else
			{
				mcu_dp_bool_update(WIFI_DPID_SWITCH_LED,dp_data.SWITCH);	
			}

		}
	}
	
	if(KEY2==0)					 //亮度+
	{
		delay1ms(100);
		if(KEY2==0)
		{
			if(dp_data.SWITCH==1&&dp_data.WORK_MODE==WHITE_MODE)
			{
				switch(dp_data.BRIGHT)
				{
					case 100:
						bright = 0;
						break;
					
					case 300:
						bright = 1;
						break;
					
					case 500:
						bright = 2;
						break;
					
					case 700:
						bright = 3;
						break;
					
					case 1000:
						bright = 4;
						break;
					
					default:
						break;
				}
				if(bright < 4)
				{
					bright++;
				}
				switch(bright)
				{
					case 0:
						dp_data.BRIGHT = 100;
						break;
					
					case 1:
						dp_data.BRIGHT = 300;
						break;
					
					case 2:
						dp_data.BRIGHT = 500;
						break;
					
					case 3:
						dp_data.BRIGHT = 700;
						break;
					
					case 4:
						dp_data.BRIGHT = 1000;
						break;
					
					default:
						break;
				}
//				if(dp_data.BRIGHT==10)
//					dp_data.BRIGHT+=190;
//				else
//					dp_data.BRIGHT+=200;
//				if(dp_data.BRIGHT>1000)
//				{
//					dp_data.BRIGHT=1000;
//				}
				get_light_data();
				Timer2_init(6);
//				send_light_data(light_data.RED_VAL,light_data.GREEN_VAL,light_data.BLUE_VAL,light_data.WHITE_VAL,light_data.WARM_VAL);
				if(KEY_BT==0)
				{
					mcu_dp_value_update(BT_DPID_BRIGHT_VALUE,dp_data.BRIGHT); 
				}
				else
				{
					mcu_dp_value_update(WIFI_DPID_BRIGHT_VALUE,dp_data.BRIGHT); 
				}
				set_flash_data();
			}
	  }
	}
	
	if(KEY4==0)				    //亮度-
	{
		delay1ms(100);
		if(KEY4==0)
		{
			if(dp_data.SWITCH==1&&dp_data.WORK_MODE==WHITE_MODE)
			{
				switch(dp_data.BRIGHT)
				{
					case 100:
						bright = 0;
						break;
					
					case 300:
						bright = 1;
						break;
					
					case 500:
						bright = 2;
						break;
					
					case 700:
						bright = 3;
						break;
					
					case 1000:
						bright = 4;
						break;
					
					default:
						break;
				}
				if(bright > 0)
				{
					bright--;
				}
				switch(bright)
				{
					case 0:
						dp_data.BRIGHT = 100;
						break;
					
					case 1:
						dp_data.BRIGHT = 300;
						break;
					
					case 2:
						dp_data.BRIGHT = 500;
						break;
					
					case 3:
						dp_data.BRIGHT = 700;
						break;
					
					case 4:
						dp_data.BRIGHT = 1000;
						break;
					
					default:
						break;
				}
//				dp_data.BRIGHT-=200;
//				if(dp_data.BRIGHT<10)
//				{
//					dp_data.BRIGHT=10;
//				}
				get_light_data();
				Timer2_init(6);
//				send_light_data(light_data.RED_VAL,light_data.GREEN_VAL,light_data.BLUE_VAL,light_data.WHITE_VAL,light_data.WARM_VAL);
				if(KEY_BT==0)
				{
					mcu_dp_value_update(BT_DPID_BRIGHT_VALUE,dp_data.BRIGHT);			
				}
				else
				{
					mcu_dp_value_update(WIFI_DPID_BRIGHT_VALUE,dp_data.BRIGHT); 
				}
				set_flash_data();
			}
		}
	}
	
	if(KEY5==0)					//ɫ��
	{
		delay1ms(100);
		if(KEY5==0)
		{
			if(dp_data.SWITCH==1&&dp_data.WORK_MODE==WHITE_MODE)
			{
				if(dp_data.COL_TEMPERATURE == 0)
				{
					temperature = 0;
				}
				else if(dp_data.COL_TEMPERATURE == 1000)
				{
					temperature = 1;
				}
				temperature++;
				switch(temperature%5)
				{
					case 1:
						dp_data.COL_TEMPERATURE=1000;
						break;
					
					case 2:
						dp_data.COL_TEMPERATURE=750;
						break;
					
					case 3:
						dp_data.COL_TEMPERATURE=500;
						break;
					
					case 4:
						dp_data.COL_TEMPERATURE=250;
						break;
					
					case 0:
						dp_data.COL_TEMPERATURE=0;
						temperature=0;
						break;
					
					default:
						break;
				}
				get_light_data();
				Timer2_init(6);
//				send_light_data(light_data.RED_VAL,light_data.GREEN_VAL,light_data.BLUE_VAL,light_data.WHITE_VAL,light_data.WARM_VAL);
				if(KEY_BT==0)
				{
					mcu_dp_value_update(BT_DPID_TEMP_VALUE,dp_data.COL_TEMPERATURE); 
				}
				else
				{
					mcu_dp_value_update(WIFI_DPID_TEMP_VALUE,dp_data.COL_TEMPERATURE); 
				}
				set_flash_data();
			}
		}
	}
	
	if(KEY1==0)					//ģʽ
	{
		delay1ms(110);
		if(KEY1==0)
		{
			if(dp_data.SWITCH==1)
			{
				color++;
				switch(color%5)
				{
					case 1:
						dp_data.WORK_MODE=COLOUR_MODE;
						dp_data.COLOUR_DATA[0]=0x0;
						dp_data.COLOUR_DATA[1]=0x0;
						dp_data.COLOUR_DATA[2]=0x0;
						dp_data.COLOUR_DATA[3]=0x0;
						dp_data.COLOUR_DATA[4]=0x0;
						dp_data.COLOUR_DATA[5]=0x3;
						dp_data.COLOUR_DATA[6]=0xe;
						dp_data.COLOUR_DATA[7]=0x8;
						dp_data.COLOUR_DATA[8]=0x0;
						dp_data.COLOUR_DATA[9]=0x3;
						dp_data.COLOUR_DATA[10]=0xe;
						dp_data.COLOUR_DATA[11]=0x8;
						break;
					case 2:
						dp_data.WORK_MODE=COLOUR_MODE;
						dp_data.COLOUR_DATA[0]=0x0;
						dp_data.COLOUR_DATA[1]=0x0;
						dp_data.COLOUR_DATA[2]=0x7;
						dp_data.COLOUR_DATA[3]=0x8;
						dp_data.COLOUR_DATA[4]=0x0;
						dp_data.COLOUR_DATA[5]=0x3;
						dp_data.COLOUR_DATA[6]=0xe;
						dp_data.COLOUR_DATA[7]=0x8;
						dp_data.COLOUR_DATA[8]=0x0;
						dp_data.COLOUR_DATA[9]=0x3;
						dp_data.COLOUR_DATA[10]=0xe;
						dp_data.COLOUR_DATA[11]=0x8;
						break;
					case 3:
						dp_data.WORK_MODE=COLOUR_MODE;
						dp_data.COLOUR_DATA[0]=0x0;
						dp_data.COLOUR_DATA[1]=0x0;
						dp_data.COLOUR_DATA[2]=0xf;
						dp_data.COLOUR_DATA[3]=0x0;
						dp_data.COLOUR_DATA[4]=0x0;
						dp_data.COLOUR_DATA[5]=0x3;
						dp_data.COLOUR_DATA[6]=0xe;
						dp_data.COLOUR_DATA[7]=0x8;
						dp_data.COLOUR_DATA[8]=0x0;
						dp_data.COLOUR_DATA[9]=0x3;
						dp_data.COLOUR_DATA[10]=0xe;
						dp_data.COLOUR_DATA[11]=0x8;					
						break;
					case 4:
						dp_data.WORK_MODE=WHITE_MODE;
						dp_data.COL_TEMPERATURE=1000;
						dp_data.BRIGHT=1000;
						break;
					case 0:
						dp_data.WORK_MODE=WHITE_MODE;
						dp_data.COL_TEMPERATURE=0;
						dp_data.BRIGHT=1000;
						color=0;
						break;
					default:
						color=0;
						break;
				}
				get_light_data();
				Timer2_init(6);
//				send_light_data(light_data.RED_VAL,light_data.GREEN_VAL,light_data.BLUE_VAL,light_data.WHITE_VAL,light_data.WARM_VAL);
				set_flash_data();
				if(dp_data.WORK_MODE==WHITE_MODE)
				{
					if(KEY_BT==0)
					{
						 mcu_dp_enum_update(BT_DPID_WORK_MODE,dp_data.WORK_MODE);
						 mcu_dp_value_update(BT_DPID_BRIGHT_VALUE,dp_data.BRIGHT);
						 mcu_dp_value_update(BT_DPID_TEMP_VALUE,dp_data.COL_TEMPERATURE);
					}
					else
					{
						 mcu_dp_enum_update(WIFI_DPID_WORK_MODE,dp_data.WORK_MODE);
						 mcu_dp_value_update(WIFI_DPID_BRIGHT_VALUE,dp_data.BRIGHT);
						 mcu_dp_value_update(WIFI_DPID_TEMP_VALUE,dp_data.COL_TEMPERATURE);
					}
				}
				else if(dp_data.WORK_MODE==COLOUR_MODE)
				{
					if(KEY_BT==0)
					{
						mcu_dp_enum_update(BT_DPID_WORK_MODE,dp_data.WORK_MODE);
						my_hex_to_str(dp_data.COLOUR_DATA,dp_data.COLOUR_DATA_UPLAOD,12);
						mcu_dp_string_update(BT_DPID_COLOUR_DATA,dp_data.COLOUR_DATA_UPLAOD,12);
					}
					else
					{
						mcu_dp_enum_update(WIFI_DPID_WORK_MODE,dp_data.WORK_MODE);
						my_hex_to_str(dp_data.COLOUR_DATA,dp_data.COLOUR_DATA_UPLAOD,12);
						mcu_dp_string_update(WIFI_DPID_COLOUR_DATA,dp_data.COLOUR_DATA_UPLAOD,12);
					}
				}
			}
		}
	}
}

