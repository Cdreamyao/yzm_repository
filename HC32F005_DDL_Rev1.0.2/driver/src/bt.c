#include "bt.h"
#include "gpio.h"
#include "wifi.h"
#include "pca.h"

unsigned char scale_flag;

void light_gra_change();
void get_433_value();
extern LIGHT_DATA_DEF light_data;
extern unsigned char change_mode;
RECEIVE433 receive;
extern uint8_t bt_wifi_mode;
int ABS(int value);
unsigned int get_max_value(unsigned int a,unsigned int b,unsigned int c,unsigned int d,unsigned int e);
/**
 *******************************************************************************
 ** \addtogroup BtGroup
 ******************************************************************************/
//@{

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define IS_VALID_TIM(x)         (TIM0 == (x) ||\
                                 TIM1 == (x) ||\
                                 TIM2 == (x))

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
static func_ptr_t pfnTim0Callback = NULL;
static func_ptr_t pfnTim1Callback = NULL;
static func_ptr_t pfnTim2Callback = NULL;
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/


/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 *****************************************************************************
 ** \brief Base Timer 中断标志获取
 **
 **
 ** \param [in]  enUnit           Timer通道选择（TIM0、TIM1、TIM2）
 ** 
 ** \retval TRUE or FALSE                                      
 *****************************************************************************/
boolean_t Bt_GetIntFlag(en_bt_unit_t enUnit)
{
    boolean_t bRetVal = FALSE;
  
    ASSERT(IS_VALID_TIM(enUnit));
    
    switch (enUnit)
    {
        case TIM0:
            bRetVal = M0P_BT0->IFR_f.TF ? TRUE : FALSE;
            break;
        case TIM1:
            bRetVal = M0P_BT1->IFR_f.TF ? TRUE : FALSE;
            break;
        case TIM2:
            bRetVal = M0P_BT2->IFR_f.TF ? TRUE : FALSE;
            break;
        default:
            bRetVal = FALSE;
            break;
    }
    
    return bRetVal;
}

/**
 *****************************************************************************
 ** \brief Base Timer 中断标志清除
 **
 **
 ** \param [in]  enUnit           Timer通道选择（TIM0、TIM1、TIM2）
 ** 
 ** \retval Ok or Error                                      
 *****************************************************************************/
en_result_t Bt_ClearIntFlag(en_bt_unit_t enUnit)
{
    en_result_t enResult = Ok;
  
    ASSERT(IS_VALID_TIM(enUnit));
    
    switch (enUnit)
    {
        case TIM0:
            M0P_BT0->ICLR_f.TFC = FALSE;
            break;
        case TIM1:
            M0P_BT1->ICLR_f.TFC = FALSE;;
            break;
        case TIM2:
            M0P_BT2->ICLR_f.TFC = FALSE;;
            break;
        default:
            enResult = Error;
            break;
    }
    
    return enResult;
}

/**
 *****************************************************************************
 ** \brief Base Timer 中断使能
 **
 **
 ** \param [in]  enUnit           Timer通道选择（TIM0、TIM1、TIM2）
 ** 
 ** \retval Ok or Error                                      
 *****************************************************************************/
en_result_t Bt_EnableIrq (en_bt_unit_t enUnit)
{
    en_result_t enResult = Ok;
  
    ASSERT(IS_VALID_TIM(enUnit));
    
    switch (enUnit)
    {
        case TIM0:
            M0P_BT0->CR_f.IE = TRUE;
            break;
        case TIM1:
            M0P_BT1->CR_f.IE = TRUE;
            break;
        case TIM2:
            M0P_BT2->CR_f.IE = TRUE;
            break;
        default:
            enResult = Error;
            break;
    }
    
    return enResult;
}

/**
 *****************************************************************************
 ** \brief Base Timer 中断禁止
 **
 **
 ** \param [in]  enUnit           Timer通道选择（TIM0、TIM1、TIM2）
 ** 
 ** \retval Ok or Error                                      
 *****************************************************************************/
en_result_t Bt_DisableIrq(en_bt_unit_t enUnit)
{
    en_result_t enResult = Ok;
  
    ASSERT(IS_VALID_TIM(enUnit));
    
    switch (enUnit)
    {
        case TIM0:
            M0P_BT0->CR_f.IE = FALSE;
            break;
        case TIM1:
            M0P_BT1->CR_f.IE = FALSE;
            break;
        case TIM2:
            M0P_BT2->CR_f.IE = FALSE;
            break;
        default:
            enResult = Error;
            break;
    }
    
    return enResult;
}

/**
 *****************************************************************************
 ** \brief Base Timer 中断服务函数
 **
 **
 ** \param [in]  u8Param           Timer通道选择（0 - TIM0、1 - TIM1、2 - TIM2）
 ** 
 ** \retval NULL                                     
 *****************************************************************************/
void Tim_IRQHandler(uint8_t u8Param)
{  
    switch (u8Param)
    {
        case 0:
            pfnTim0Callback();
            break;
        case 1:
            pfnTim1Callback();
            break;
        case 2:
            pfnTim2Callback();
            break;
        default:
            ;
            break;       
    }
}



/**
 *****************************************************************************
 ** \brief Base Timer 初始化配置
 **
 **
 ** \param [in]  enUnit           Timer通道选择（TIM0、TIM1、TIM2）
 ** \param [in]  pstcConfig       初始化配置结构体指针
 ** 
 ** \retval Ok or Error                                      
 *****************************************************************************/
en_result_t Bt_Init(en_bt_unit_t enUnit, stc_bt_config_t* pstcConfig)
{
    en_result_t enResult = Ok;
  
    ASSERT(IS_VALID_TIM(enUnit));
    
    switch (enUnit)
    {
        case TIM0:
            {
                M0P_BT0->CR_f.GATE_P = pstcConfig->enGateP;
                M0P_BT0->CR_f.GATE   = pstcConfig->enGate;
                M0P_BT0->CR_f.PRS    = pstcConfig->enPRS;
                M0P_BT0->CR_f.TOG_EN = pstcConfig->enTog;
                M0P_BT0->CR_f.CT     = pstcConfig->enCT;
                M0P_BT0->CR_f.MD     = pstcConfig->enMD; 
                
                pfnTim0Callback      = pstcConfig->pfnTim0Cb;
            }
            break;
        case TIM1:
            {
                M0P_BT1->CR_f.GATE_P = pstcConfig->enGateP;
                M0P_BT1->CR_f.GATE   = pstcConfig->enGate;
                M0P_BT1->CR_f.PRS    = pstcConfig->enPRS;
                M0P_BT1->CR_f.TOG_EN = pstcConfig->enTog;
                M0P_BT1->CR_f.CT     = pstcConfig->enCT;
                M0P_BT1->CR_f.MD     = pstcConfig->enMD;
                
                pfnTim1Callback      = pstcConfig->pfnTim1Cb;
            }
            break;
        case TIM2:
            {
                M0P_BT2->CR_f.GATE_P = pstcConfig->enGateP;
                M0P_BT2->CR_f.GATE   = pstcConfig->enGate;
                M0P_BT2->CR_f.PRS    = pstcConfig->enPRS;
                M0P_BT2->CR_f.TOG_EN = pstcConfig->enTog;
                M0P_BT2->CR_f.CT     = pstcConfig->enCT;
                M0P_BT2->CR_f.MD     = pstcConfig->enMD; 
                
                pfnTim2Callback      = pstcConfig->pfnTim2Cb;
            }
            break;
        default:
            enResult = Error;
            break;
    }
    
    return enResult;
}

/**
 *****************************************************************************
 ** \brief Base Timer 启动运行
 **
 **
 ** \param [in]  enUnit           Timer通道选择（TIM0、TIM1、TIM2）
 ** 
 ** \retval Ok or Error                                      
 *****************************************************************************/
en_result_t Bt_Run(en_bt_unit_t enUnit)
{
    en_result_t enResult = Ok;
  
    ASSERT(IS_VALID_TIM(enUnit));
    
    switch (enUnit)
    {
        case TIM0:
            M0P_BT0->CR_f.TR = TRUE;
            break;
        case TIM1:
            M0P_BT1->CR_f.TR = TRUE;
            break;
        case TIM2:
            M0P_BT2->CR_f.TR = TRUE;
            break;
        default:
            enResult = Error;
            break;
    }
    
    return enResult;    
}

/**
 *****************************************************************************
 ** \brief Base Timer 停止运行
 **
 **
 ** \param [in]  enUnit           Timer通道选择（TIM0、TIM1、TIM2）
 ** 
 ** \retval Ok or Error                                      
 *****************************************************************************/
en_result_t Bt_Stop(en_bt_unit_t enUnit)
{
    en_result_t enResult = Ok;
  
    ASSERT(IS_VALID_TIM(enUnit));
    
    switch (enUnit)
    {
        case TIM0:
            M0P_BT0->CR_f.TR = FALSE;
            break;
        case TIM1:
            M0P_BT1->CR_f.TR = FALSE;
            break;
        case TIM2:
            M0P_BT2->CR_f.TR = FALSE;
            break;
        default:
            enResult = Error;
            break;
    }
    
    return enResult; 
}

/**
 *****************************************************************************
 ** \brief Base Timer 重载值设置
 **
 **
 ** \param [in]  enUnit           Timer通道选择（TIM0、TIM1、TIM2）
 ** \param [in]  u16Data          16bits重载值
 ** 
 ** \retval Ok or Error                                      
 *****************************************************************************/
en_result_t Bt_ARRSet(en_bt_unit_t enUnit, uint16_t u16Data)
{
    en_result_t enResult = Ok;
  
    ASSERT(IS_VALID_TIM(enUnit));
    
    switch (enUnit)
    {
        case TIM0:
            M0P_BT0->ARR_f.ARR = u16Data;
            break;
        case TIM1:
            M0P_BT1->ARR_f.ARR = u16Data;
            break;
        case TIM2:
            M0P_BT2->ARR_f.ARR = u16Data;
            break;
        default:
            enResult = Error;
            break;
    }
    
    return enResult; 
}

/**
 *****************************************************************************
 ** \brief Base Timer 16位计数器初值设置
 **
 **
 ** \param [in]  enUnit           Timer通道选择（TIM0、TIM1、TIM2）
 ** \param [in]  u16Data          16位初值
 ** 
 ** \retval Ok or Error                                      
 *****************************************************************************/
en_result_t Bt_Cnt16Set(en_bt_unit_t enUnit, uint16_t u16Data)
{
    en_result_t enResult = Ok;
  
    ASSERT(IS_VALID_TIM(enUnit));
    
    switch (enUnit)
    {
        case TIM0:
            M0P_BT0->CNT_f.CNT = u16Data;
            break;
        case TIM1:
            M0P_BT1->CNT_f.CNT = u16Data;
            break;
        case TIM2:
            M0P_BT2->CNT_f.CNT = u16Data;
            break;
        default:
            enResult = Error;
            break;
    }
    
    return enResult; 
}

/**
 *****************************************************************************
 ** \brief Base Timer 16位计数值获取
 **
 **
 ** \param [in]  enUnit           Timer通道选择（TIM0、TIM1、TIM2）
 ** 
 ** \retval 16bits计数值                                      
 *****************************************************************************/
uint16_t Bt_Cnt16Get(en_bt_unit_t enUnit)
{
    uint16_t    u16CntData = 0;
  
    ASSERT(IS_VALID_TIM(enUnit));
    
    switch (enUnit)
    {
        case TIM0:
            u16CntData = M0P_BT0->CNT_f.CNT;
            break;
        case TIM1:
            u16CntData = M0P_BT1->CNT_f.CNT;
            break;
        case TIM2:
            u16CntData = M0P_BT2->CNT_f.CNT;
            break;
        default:
            u16CntData = 0;
            break;
    }
    
    return u16CntData; 
}

/**
 *****************************************************************************
 ** \brief Base Timer 32位计数器初值设置
 **
 **
 ** \param [in]  enUnit           Timer通道选择（TIM0、TIM1、TIM2）
 ** \param [in]  u32Data          32位初值
 ** 
 ** \retval Ok or Error                                      
 *****************************************************************************/
en_result_t Bt_Cnt32Set(en_bt_unit_t enUnit, uint32_t u32Data)
{
    en_result_t enResult = Ok;
  
    ASSERT(IS_VALID_TIM(enUnit));
    
    switch (enUnit)
    {
        case TIM0:
            M0P_BT0->CNT32_f.CNT32 = u32Data;
            break;
        case TIM1:
            M0P_BT1->CNT32_f.CNT32 = u32Data;
            break;
        case TIM2:
            M0P_BT2->CNT32_f.CNT32 = u32Data;
            break;
        default:
            enResult = Error;
            break;
    }
    
    return enResult; 
}

/**
 *****************************************************************************
 ** \brief Base Timer 32位计数值获取
 **
 **
 ** \param [in]  enUnit           Timer通道选择（TIM0、TIM1、TIM2）
 ** 
 ** \retval 32bits计数值                                      
 *****************************************************************************/
uint32_t Bt_Cnt32Get(en_bt_unit_t enUnit)
{
    uint32_t    u32CntData = 0;
  
    ASSERT(IS_VALID_TIM(enUnit));
    
    switch (enUnit)
    {
        case TIM0:
            u32CntData = M0P_BT0->CNT32_f.CNT32;
            break;
        case TIM1:
            u32CntData = M0P_BT1->CNT32_f.CNT32;
            break;
        case TIM2:
            u32CntData = M0P_BT2->CNT32_f.CNT32;
            break;
        default:
            u32CntData = 0;
            break;
    }
    
    return u32CntData;
}

//@} // BtGroup

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/

/**************************************定时器初始化设置*********************************************/

/*******************************************************************************
 * BT0中断服务函数
 ******************************************************************************/
uint8_t control;
uint8_t receive_finish=0;
uint16_t RF433_Addr;
uint16_t RF433_Group_Value;
uint16_t RF433_Group_New_Value;
extern unsigned int irq_cnt ;
void TIMER0Int(void)
{
		static uint8_t cnt;
    if (TRUE == Bt_GetIntFlag(TIM0))
    {
			Bt_ClearIntFlag(TIM0);
			cnt++;
			if(cnt==125)
			{
				irq_cnt++;
				cnt=0;
			}
			if(receive_finish==0)
			{
				get_433_value();
			}
			if(receive_finish==1)
			{
				control=receive.Rf_Control_Data;
				RF433_Addr=(receive.Rf_Data[0]<<8)|(receive.Rf_Data[1]);
			}
    }
}

/*******************************************************************************
 * BT定时功能 （重载模式）			自定义
 ******************************************************************************/
en_result_t Timer0_init(uint16_t us)
{
    stc_bt_config_t   stcConfig;
    en_result_t       enResult = Error;
		uint16_t          u16ArrData = 		 65535-us*4*6;	//向下计数 65535-N   默认内部4M 不分频一个机器周期0.25us	8分频一个机器周期2us		24MHZ  *6
    uint16_t          u16InitCntData = 65530;
    
    stcConfig.pfnTim0Cb = TIMER0Int;
//P25设置为门控使能IO
//    Gpio_SetFunc_TIM1_GATE_P25();
//    stcConfig.enGateP = BtPositive;
    stcConfig.enGate  = BtGateDisable;
    stcConfig.enPRS   = BtPCLKDiv1;
    stcConfig.enTog   = BtTogDisable;
    stcConfig.enCT    = BtTimer;
    stcConfig.enMD    = BtMode2;
    //Bt初始化
    if (Ok != Bt_Init(TIM0, &stcConfig))
    {
        enResult = Error;
    }
    
    //TIM1中断使能
    Bt_ClearIntFlag(TIM0);
    Bt_EnableIrq(TIM0);
    EnableNvic(TIM0_IRQn, 3, TRUE);
    
    //设置重载值和计数值，启动计数
    Bt_ARRSet(TIM0, u16ArrData);
    Bt_Cnt16Set(TIM0, u16InitCntData);
    Bt_Run(TIM0);
    
    return enResult;
}



void TIMER2Int(void)
{
    if (TRUE == Bt_GetIntFlag(TIM2))
    {
        Bt_ClearIntFlag(TIM2);
				light_gra_change();
    }
}

en_result_t Timer2_init(uint16_t ms)
{
    stc_bt_config_t   stcConfig;
    en_result_t       enResult = Error;
		uint16_t          u16ArrData = 65535-ms/0.64*5;	//向下计数 65535-N   默认内部4M 不分频一个机器周期0.25us	256分频一个机器周期64us		24MHZ  *6
    uint16_t          u16InitCntData = 65530;
    
    stcConfig.pfnTim2Cb = TIMER2Int;
//P25设置为门控使能IO
//    Gpio_SetFunc_TIM1_GATE_P25();
//    stcConfig.enGateP = BtPositive;
    stcConfig.enGate  = BtGateDisable;
    stcConfig.enPRS   = BtPCLKDiv256;
    stcConfig.enTog   = BtTogDisable;
    stcConfig.enCT    = BtTimer;
    stcConfig.enMD    = BtMode2;
    //Bt初始化
    if (Ok != Bt_Init(TIM2, &stcConfig))
    {
        enResult = Error;
    }
    
    //TIM1中断使能
    Bt_ClearIntFlag(TIM2);
    Bt_EnableIrq(TIM2);
    EnableNvic(TIM2_IRQn, 3, TRUE);
    
    //设置重载值和计数值，启动计数
    Bt_ARRSet(TIM2, u16ArrData);
    Bt_Cnt16Set(TIM2, u16InitCntData);
    Bt_Run(TIM2);
    
    return enResult;
}


int ABS(int value)
{
	if(value < 0){
		return 0-value;
	}else{
		return value;
	}
}

unsigned int get_max_value(unsigned int a,unsigned int b,unsigned int c,unsigned int d,unsigned int e)
{
	int x,y,z;
	x = a > b ? a : b; //1次比较，1次赋值
	y = c > d ? c : d; //1次比较，1次赋值
	z = x > y ? x : y;
	return z > e ? z : e;  //1次比较
}

static float r_scale;
static float g_scale;
static float b_scale;
static float w_scale;
static float ww_scale;
void light_gra_change()
{
  int delata_red = 0;
	int delata_green = 0;
  int delata_blue = 0;
  int delata_white = 0;
	int delata_warm = 0;
  int MAX_VALUE;
	unsigned int RED_GRA_STEP = 1;
	unsigned int GREEN_GRA_STEP = 1;
	unsigned int BLUE_GRA_STEP = 1;
	unsigned int WHITE_GRA_STEP = 1;
	unsigned int WARM_GRA_STEP = 1;

	 if(light_data.WHITE_VAL != light_data.LAST_WHITE_VAL || light_data.WARM_VAL != light_data.LAST_WARM_VAL || light_data.RED_VAL != light_data.LAST_RED_VAL ||light_data.GREEN_VAL != light_data.LAST_GREEN_VAL || light_data.BLUE_VAL != light_data.LAST_BLUE_VAL)
	 {
			delata_red = light_data.RED_VAL - light_data.LAST_RED_VAL;
			delata_green = light_data.GREEN_VAL - light_data.LAST_GREEN_VAL;
			delata_blue = light_data.BLUE_VAL - light_data.LAST_BLUE_VAL;
			delata_white = light_data.WHITE_VAL - light_data.LAST_WHITE_VAL;
			delata_warm = light_data.WARM_VAL - light_data.LAST_WARM_VAL;
			MAX_VALUE = get_max_value(ABS(delata_red), ABS(delata_green), ABS(delata_blue), ABS(delata_white), ABS(delata_warm));

			if(scale_flag == 0){
				r_scale = ABS(delata_red)/1.0/MAX_VALUE;
				g_scale = ABS(delata_green)/1.0/MAX_VALUE;
				b_scale = ABS(delata_blue)/1.0/MAX_VALUE;
				w_scale = ABS(delata_white)/1.0/MAX_VALUE;
				ww_scale = ABS(delata_warm)/1.0/MAX_VALUE;
				scale_flag = 1;
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
			if(MAX_VALUE == ABS(delata_warm)){
				WARM_GRA_STEP = 1;
			}else{
				WARM_GRA_STEP =  ABS(delata_warm) - MAX_VALUE*ww_scale;
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
			if(delata_warm != 0){
			    if(ABS(delata_warm) < WARM_GRA_STEP)
			    {
					 light_data.LAST_WARM_VAL += delata_warm;
				}else{
					if(delata_warm < 0)
						light_data.LAST_WARM_VAL -= WARM_GRA_STEP;
					else
						light_data.LAST_WARM_VAL += WARM_GRA_STEP;
				}
			}

			if((dp_data.SWITCH == 0) && ((dp_data.WORK_MODE==SCENE_MODE)&&(change_mode==gradual))){
				;
			}else{
				if(dp_data.WORK_MODE!=SCENE_MODE||(dp_data.WORK_MODE==SCENE_MODE&&change_mode!=jump))
				{
//					send_light_data(light_data.LAST_RED_VAL*RESO_VAL, light_data.LAST_GREEN_VAL*RESO_VAL, light_data.LAST_BLUE_VAL*RESO_VAL, light_data.LAST_WHITE_VAL*RESO_VAL, light_data.LAST_WARM_VAL*RESO_VAL);
					send_light_data(light_data.LAST_RED_VAL, light_data.LAST_GREEN_VAL, light_data.LAST_BLUE_VAL, light_data.LAST_WHITE_VAL, light_data.LAST_WARM_VAL);
				}
			}
		}else{
			if(dp_data.WORK_MODE!=SCENE_MODE||(dp_data.WORK_MODE==SCENE_MODE&&change_mode!=gradual)){
				send_light_data(light_data.RED_VAL, light_data.GREEN_VAL, light_data.BLUE_VAL, light_data.WHITE_VAL, light_data.WARM_VAL);
				Bt_Stop(TIM2);
			}
		}
	}

	
void get_433_value()
{
	switch(receive.Rf_Cnt)
	{
		case 0 :                                                           //12.4ms起始头
			if(Gpio_GetIO(3,4)==1)                                                
			{
				receive.Count_Lead_Hi++; 																			//累计高电平持续时间
				if(receive.Count_Lead_Lo != 0)																//电平由低电平变为高电平
				{																																											
					if((receive.Count_Lead_Lo >= 70) && (receive.Count_Lead_Lo <= 200))           //5.6ms - 16ms，判断是否为起始头
					{
						receive.Rf_Flag = RF_FLAG_LO;
						receive.Count_Lead_Hi=0;
						receive.Count_Lead_Lo=0;
						receive.Recv_Data_Buf = 0x00;                              //数据清零准备接受数据
						receive.Count_Data_Hi = 0;
						receive.Count_Data_Lo = 0;
						receive.Recv_Bit_Cnt  = 0;
						receive.Recv_Byte_Cnt = 0;
						receive.Rf_Cnt=1;
					}
					else                                                     //不是起始头，为干扰噪声，数据清零重新计数
					{
						receive.Count_Lead_Lo=0;
						receive.Rf_Cnt=0;
					}
				}
					
			}
			else if(Gpio_GetIO(3,4)==0)                                                     
			{
				receive.Count_Lead_Lo++;																						//累计低电平持续时间		
				if(receive.Count_Lead_Hi != 0)																			//电平由高变低
				{
					if((receive.Count_Lead_Hi >= 70) && (receive.Count_Lead_Hi <= 200))       //5.6ms - 16ms，判断是否为起始头
					{
						receive.Rf_Flag = RF_FLAG_HI;
						receive.Count_Lead_Hi=0;
						receive.Count_Lead_Lo=0;
						receive.Recv_Data_Buf = 0x00;                              //数据清零准备接受数据
						receive.Count_Data_Hi = 0;
						receive.Count_Data_Lo = 0;
						receive.Recv_Bit_Cnt  = 0;
						receive.Recv_Byte_Cnt = 0;
						receive.Rf_Cnt=1;
					}
					else                                                     //不是起始头，为干扰噪声，数据清零重新计数
					{
						receive.Count_Lead_Hi=0;
						receive.Rf_Cnt=0;
					}
				}


			}
		break;

		case 1 :																																
			if(receive.Rf_Flag == RF_FLAG_HI)																				//起始头为高电平的编码规则
			{
				if(Gpio_GetIO(3,4)==0)                                                  //累计低电平时间
				{
					receive.Count_Data_Lo++;
				}
				else                                                       //低电平结束
				{
				 if((receive.Count_Data_Lo >= 1) && (receive.Count_Data_Lo <= 30))       // 80us - 2.4ms
					{
					receive.Lo_Cnt = receive.Count_Data_Lo;                          
					receive.Count_Data_Lo = 0;
					receive.Rf_Cnt=2;
					}
				 else
					{
					receive.Rf_Cnt=0;
					}
				}
			}
			else if(receive.Rf_Flag == RF_FLAG_LO)																	//起始头为低电平的编码规则
			{
				if(Gpio_GetIO(3,4)==1)                                                  
				{
					receive.Count_Data_Hi++;
				}
				else                                                       
				{
				 if((receive.Count_Data_Hi >= 1) && (receive.Count_Data_Hi <= 30))       // 80us - 2.4ms
					{
						receive.Hi_Cnt = receive.Count_Data_Hi;                         
						receive.Count_Data_Hi = 0;
						receive.Rf_Cnt=2;
						}
				 else
					{
						receive.Rf_Cnt=0;
					}
				}
			}

		break;

		case 2:                                                           
			if(receive.Rf_Flag == RF_FLAG_HI)
			{
				if(Gpio_GetIO(3,4)==1)                                                   
				{
					receive.Count_Data_Hi++;
				}
				else                                                        
				{
					if((receive.Count_Data_Hi >= 1) && (receive.Count_Data_Hi <= 30))       // 80us - 2.4ms
					{
						receive.Hi_Cnt = receive.Count_Data_Hi;                        
						receive.Count_Data_Hi = 0;
						receive.Rf_Cnt=3;
					}
					else
					{
						receive.Rf_Cnt=0;
					}
				}
			}
			else if(receive.Rf_Flag == RF_FLAG_LO)
			{
				if(Gpio_GetIO(3,4)==0)                                                   
				{
					receive.Count_Data_Lo++;
				}
				else                                                        
				{
					if((receive.Count_Data_Lo >= 1) && (receive.Count_Data_Lo <= 30))       // 80us - 2.4ms
						{
							receive.Lo_Cnt = receive.Count_Data_Lo;                          
							receive.Count_Data_Lo = 0;
							receive.Rf_Cnt=3;
						}
					else
					{
							receive.Rf_Cnt=0;
					}
				}
			}
			
		break;

		case 3 :
			if(receive.Rf_Flag == RF_FLAG_HI)
			{
				receive.Recv_Data_Buf <<= 1;                                      //读数据
				if(receive.Hi_Cnt<receive.Lo_Cnt)                                         //低电平持续时间长，则为‘1’
				{
					receive.Recv_Data_Buf|=0x01;
				}
				else
				{
					receive.Recv_Data_Buf&=0xFE;
				}
					receive.Recv_Bit_Cnt ++;

				if(receive.Recv_Bit_Cnt>7)                                        //读下个字节
				{
					receive.Rf_Data[receive.Recv_Byte_Cnt]=receive.Recv_Data_Buf;                  //存入数组
					receive.Recv_Bit_Cnt = 0;
					receive.Recv_Byte_Cnt++;
					receive.Recv_Data_Buf = 0x00;
				}

				if(receive.Recv_Byte_Cnt>2)                                     //所有数据接受完成
				{
					receive.Rf_Control_Data=receive.Rf_Data[2]&0xFF;                      //读取键值
					receive.Rf_Cnt = 4;                                           
				}
				else
				{
					receive.Rf_Cnt = 1;                                          
				}
			}
			else if(receive.Rf_Flag == RF_FLAG_LO)
			{
				receive.Recv_Data_Buf <<= 1;                                     
				if(receive.Hi_Cnt>receive.Lo_Cnt)                                         
				{
					receive.Recv_Data_Buf|=0x01;
				}
				else
				{
					receive.Recv_Data_Buf&=0xFE;
				}
				receive.Recv_Bit_Cnt ++;

				if(receive.Recv_Bit_Cnt>7)                                        
				{
					receive.Rf_Data[receive.Recv_Byte_Cnt]=receive.Recv_Data_Buf;                  
					receive.Recv_Bit_Cnt = 0;
					receive.Recv_Byte_Cnt++;
					receive.Recv_Data_Buf = 0x00;
				}

				if(receive.Recv_Byte_Cnt>2)                                     
				{
					receive.Rf_Control_Data=receive.Rf_Data[2]&0xFF;                     
					receive.Rf_Cnt = 4;                                           
				}
				else
				{
					receive.Rf_Cnt = 1;                                           
				}
			}

		break;

		case 4 :																														//数据接受结束
			receive.Rf_Flag = 0;
			receive.Rf_Cnt = 0;                                              
			receive_finish =1;
			bt_wifi_mode=0;
			break;

		default:
			receive.Rf_Flag = 0;
			receive.Rf_Cnt = 0;
		break;
	}
}