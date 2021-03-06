﻿/***********************************************************
*  File: device.h 
*  Author: nzy
*  Date: 20150605
***********************************************************/
#ifndef _DEVICE_H
    #define _DEVICE_H

    #include "wmtime.h"
    #include "error_code.h"
    
#ifdef __cplusplus
	extern "C" {
#endif

#ifdef  __DEVICE_GLOBALS
    #define __DEVICE_EXT
#else
    #define __DEVICE_EXT extern
#endif

/***********************************************************
*************************product define*********************
***********************************************************/
/***********************************************************
*************************micro define***********************
***********************************************************/
// device information define
#define SW_VER USER_SW_VER
//#define PRODECT_KEY "fcif4xpjdblzifhk"
#define PRODECT_KEY "mg6sgtufiwt9lmwl"

#define DEF_DEV_ABI DEV_SINGLE
/***********************************************************
*************************variable define********************
***********************************************************/

#define USE_NEW_PRODTEST
/***********************************************************
*************************function define********************
***********************************************************/
/***********************************************************
*  Function: device_init
*  Input: 
*  Output: 
*  Return: 
***********************************************************/
__DEVICE_EXT \
OPERATE_RET device_init(VOID);


#ifdef __cplusplus
}
#endif
#endif

