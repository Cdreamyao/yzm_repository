/*
 * ir_code.h
 *
 *  Created on: 2020年6月27日
 *      Author: SEO
 */

#ifndef APP_TUYA_USER_FARYLINK_SOC_LIGHT_STRIP_FAKE4_RGB_INCLUDE_IR_CODE_H_
#define APP_TUYA_USER_FARYLINK_SOC_LIGHT_STRIP_FAKE4_RGB_INCLUDE_IR_CODE_H_

#define IR_CODE_HEAD1				0x00
#define IR_CODE_HEAD2				0xEF

#define IR_CODE_BRIGHT_UP			0x00
#define IR_CODE_BRIGHT_DOWN			0x01

#define IR_CODE_POWER_OFF			0x02
#define IR_CODE_POWER_ON			0x03

#define IR_CODE_R					0x04
#define IR_CODE_G					0x05
#define IR_CODE_B					0x06
#define IR_CODE_W					0x07

#define IR_CODE_COLOR1				0x08
#define IR_CODE_COLOR2				0x09
#define IR_CODE_COLOR3				0x0A
#define IR_CODE_COLOR4				0x0C
#define IR_CODE_COLOR5				0x0D
#define IR_CODE_COLOR6				0x0E
#define IR_CODE_COLOR7				0x10
#define IR_CODE_COLOR8				0x11
#define IR_CODE_COLOR9				0x12
#define IR_CODE_COLOR10				0x14
#define IR_CODE_COLOR11				0x15
#define IR_CODE_COLOR12				0x16

#define IR_CODE_FLASH				0x0B
#define IR_CODE_STROBE				0x0F
#define IR_CODE_FADE				0x13
#define IR_CODE_SMOOTH				0x17

#endif /* APP_TUYA_USER_FARYLINK_SOC_LIGHT_STRIP_FAKE4_RGB_INCLUDE_IR_CODE_H_ */
