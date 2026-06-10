/***********************************************************
 * @file    lcd_task10.c
 * @brief   AE-AQM0802に苗字と名前を表示するプログラム
 * @version 1.0.0
 * @date    2026.06.10
 * @author  Luo Kaiqun
 ***********************************************************/

#include <stddef.h>
#include <string.h>
#include <pigpiod_if2.h>
#include "type.h"

/***********************************************************
 * マクロ定義
 ***********************************************************/
#define LCD_I2C_BUS                       (1)
#define LCD_I2C_ADDR                      (0x3E)
#define LCD_I2C_FLAGS                     (0)

#define LCD_CONTROL_COMMAND               (0x00)
#define LCD_CONTROL_DATA                  (0x40)

#define LCD_CMD_FUNCTION_SET_NORMAL       (0x38)
#define LCD_CMD_FUNCTION_SET_EXTEND       (0x39)
#define LCD_CMD_INTERNAL_OSC              (0x14)
#define LCD_CMD_CONTRAST_LOW              (0x70)
#define LCD_CMD_POWER_ICON_CONTRAST       (0x56)
#define LCD_CMD_FOLLOWER_CONTROL          (0x6C)
#define LCD_CMD_DISPLAY_ON                (0x0C)
#define LCD_CMD_CLEAR_DISPLAY             (0x01)

#define LCD_SET_DDRAM_BASE                (0x80)
#define LCD_DDRAM_LINE1                   (0x00)
#define LCD_DDRAM_LINE2                   (0x40)
#define LCD_CMD_SET_LINE1                 (LCD_SET_DDRAM_BASE + LCD_DDRAM_LINE1)
#define LCD_CMD_SET_LINE2                 (LCD_SET_DDRAM_BASE + LCD_DDRAM_LINE2)

#define LCD_LINE1                         (1)
#define LCD_LINE2                         (2)
#define LCD_LINE_LENGTH                   (8)

#define LCD_LINE1_MESSAGE                 ("LUO")
#define LCD_LINE2_MESSAGE                 ("KAIQUN")

#define MSEC_TO_SEC                       (1000.0)
#define LCD_COMMAND_WAIT_MSEC             (1.0)
#define LCD_COMMAND_WAIT_SEC              (LCD_COMMAND_WAIT_MSEC / MSEC_TO_SEC)
#define LCD_INIT_WAIT_MSEC                (200.0)
#define LCD_INIT_WAIT_SEC                 (LCD_INIT_WAIT_MSEC / MSEC_TO_SEC)
#define LCD_DISPLAY_WAIT_MSEC             (5000.0)
#define LCD_DISPLAY_WAIT_SEC              (LCD_DISPLAY_WAIT_MSEC / MSEC_TO_SEC)

/***********************************************************
 * プロトタイプ宣言
 ***********************************************************/
S4 s4_InitLcd(S4 s4_pi, S4 *ps4_handle);
S4 s4_SendCommand(S4 s4_pi, S4 s4_handle, U1 u1_command);
S4 s4_SendData(S4 s4_pi, S4 s4_handle, U1 u1_data);
S4 s4_DisplayString(S4 s4_pi, S4 s4_handle, U1 u1_line, const char *pch_message);
S4 s4_ClearLcd(S4 s4_pi, S4 s4_handle);

/***********************************************************
 * @brief LCDに苗字と名前を表示し、5秒後に表示をクリアする。
 * @param なし
 * @retval int 0 正常終了
 * @retval int 1 異常終了
 ***********************************************************/
int main(void)
{
	S4 s4_pi;
	S4 s4_lcdHandle;
	S4 s4_ret;

	s4_lcdHandle = -1;

	s4_pi = pigpio_start(NULL, NULL);
	if (s4_pi < 0) {
		return 1;
	}

	s4_ret = s4_InitLcd(s4_pi, &s4_lcdHandle);
	if (s4_ret != 0) {
		if (s4_lcdHandle >= 0) {
			(VD)i2c_close(s4_pi, s4_lcdHandle);
		} else {
			/* DO NOTHING */
		}
		(VD)pigpio_stop(s4_pi);
		return 1;
	}

	s4_ret = s4_DisplayString(s4_pi, s4_lcdHandle, LCD_LINE1, LCD_LINE1_MESSAGE);
	if (s4_ret != 0) {
		(VD)i2c_close(s4_pi, s4_lcdHandle);
		(VD)pigpio_stop(s4_pi);
		return 1;
	}

	s4_ret = s4_DisplayString(s4_pi, s4_lcdHandle, LCD_LINE2, LCD_LINE2_MESSAGE);
	if (s4_ret != 0) {
		(VD)i2c_close(s4_pi, s4_lcdHandle);
		(VD)pigpio_stop(s4_pi);
		return 1;
	}

	(VD)time_sleep(LCD_DISPLAY_WAIT_SEC);

	s4_ret = s4_ClearLcd(s4_pi, s4_lcdHandle);

	(VD)i2c_close(s4_pi, s4_lcdHandle);
	(VD)pigpio_stop(s4_pi);

	if (s4_ret != 0) {
		return 1;
	}

	return 0;
}

/***********************************************************
 * @brief 引数で指定されたLCDを初期化する。
 * @param[in]  S4 s4_pi pigpio接続番号
 * @param[out] S4 *ps4_handle I2Cハンドル格納先
 * @retval S4 0 正常終了
 * @retval S4 1 異常終了
 ***********************************************************/
S4 s4_InitLcd(S4 s4_pi, S4 *ps4_handle)
{
	S4 s4_handle;
	S4 s4_ret;

	if (ps4_handle == NULL) {
		return 1;
	}

	s4_handle = i2c_open(s4_pi, LCD_I2C_BUS, LCD_I2C_ADDR, LCD_I2C_FLAGS);
	if (s4_handle < 0) {
		return 1;
	}

	*ps4_handle = s4_handle;

	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_FUNCTION_SET_NORMAL);
	if (s4_ret != 0) {
		return 1;
	}

	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_FUNCTION_SET_EXTEND);
	if (s4_ret != 0) {
		return 1;
	}

	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_INTERNAL_OSC);
	if (s4_ret != 0) {
		return 1;
	}

	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_CONTRAST_LOW);
	if (s4_ret != 0) {
		return 1;
	}

	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_POWER_ICON_CONTRAST);
	if (s4_ret != 0) {
		return 1;
	}

	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_FOLLOWER_CONTROL);
	if (s4_ret != 0) {
		return 1;
	}

	(VD)time_sleep(LCD_INIT_WAIT_SEC);

	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_FUNCTION_SET_NORMAL);
	if (s4_ret != 0) {
		return 1;
	}

	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_DISPLAY_ON);
	if (s4_ret != 0) {
		return 1;
	}

	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_CLEAR_DISPLAY);
	if (s4_ret != 0) {
		return 1;
	}

	return 0;
}

/***********************************************************
 * @brief 引数で指定された命令をLCDへ送信する。
 * @param[in] S4 s4_pi pigpio接続番号
 * @param[in] S4 s4_handle I2Cハンドル
 * @param[in] U1 u1_command LCDへ送信する命令
 * @retval S4 0 正常終了
 * @retval S4 1 異常終了
 ***********************************************************/
S4 s4_SendCommand(S4 s4_pi, S4 s4_handle, U1 u1_command)
{
	S4 s4_ret;

	s4_ret = i2c_write_byte_data(s4_pi, s4_handle, LCD_CONTROL_COMMAND, u1_command);

	(VD)time_sleep(LCD_COMMAND_WAIT_SEC);

	if (s4_ret < 0) {
		return 1;
	}

	return 0;
}

/***********************************************************
 * @brief 引数で指定された表示データをLCDへ送信する。
 * @param[in] S4 s4_pi pigpio接続番号
 * @param[in] S4 s4_handle I2Cハンドル
 * @param[in] U1 u1_data LCDへ送信する表示データ
 * @retval S4 0 正常終了
 * @retval S4 1 異常終了
 ***********************************************************/
S4 s4_SendData(S4 s4_pi, S4 s4_handle, U1 u1_data)
{
	S4 s4_ret;

	s4_ret = i2c_write_byte_data(s4_pi, s4_handle, LCD_CONTROL_DATA, u1_data);

	(VD)time_sleep(LCD_COMMAND_WAIT_SEC);

	if (s4_ret < 0) {
		return 1;
	}

	return 0;
}

/***********************************************************
 * @brief 引数で指定されたLCD行に文字列を表示する。
 * @param[in] S4 s4_pi pigpio接続番号
 * @param[in] S4 s4_handle I2Cハンドル
 * @param[in] U1 u1_line 表示するLCD行番号
 * @param[in] const char *pch_message 表示する文字列
 * @retval S4 0 正常終了
 * @retval S4 1 異常終了
 ***********************************************************/
S4 s4_DisplayString(S4 s4_pi, S4 s4_handle, U1 u1_line, const char *pch_message)
{
	U4 u4_loopCnt;
	U4 u4_length;
	U1 u1_command;
	S4 s4_ret;

	if (pch_message == NULL) {
		return 1;
	}

	if (u1_line == LCD_LINE1) {
		u1_command = LCD_CMD_SET_LINE1;
	} else if (u1_line == LCD_LINE2) {
		u1_command = LCD_CMD_SET_LINE2;
	} else {
		return 1;
	}

	s4_ret = s4_SendCommand(s4_pi, s4_handle, u1_command);
	if (s4_ret != 0) {
		return 1;
	}

	u4_length = (U4)strlen(pch_message);
	if (u4_length > LCD_LINE_LENGTH) {
		u4_length = LCD_LINE_LENGTH;
	} else {
		/* DO NOTHING */
	}

	for (u4_loopCnt = 0; u4_loopCnt < u4_length; u4_loopCnt++) {
		s4_ret = s4_SendData(s4_pi, s4_handle, (U1)pch_message[u4_loopCnt]);
		if (s4_ret != 0) {
			return 1;
		}
	}

	return 0;
}

/***********************************************************
 * @brief LCD表示をクリアする。
 * @param[in] S4 s4_pi pigpio接続番号
 * @param[in] S4 s4_handle I2Cハンドル
 * @retval S4 0 正常終了
 * @retval S4 1 異常終了
 ***********************************************************/
S4 s4_ClearLcd(S4 s4_pi, S4 s4_handle)
{
	S4 s4_ret;

	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_CLEAR_DISPLAY);
	if (s4_ret != 0) {
		return 1;
	}

	return 0;
}
