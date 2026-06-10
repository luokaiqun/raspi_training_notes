/***********************************************************
 * @file    lcd.c
 * @brief   AE-AQM0802に苗字と名前を表示するプログラム
 * @version 1.0.0
 * @date    2026.06.11
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

#define LCD_LINE1                         (1)
#define LCD_LINE2                         (2)
#define LCD_LINE_LENGTH                   (8)

#define LCD_INIT_CMD_NUM                  (9)
#define LCD_INIT_WAIT_INDEX               (5)

#define LCD_LINE1_MESSAGE                 ("LUO")
#define LCD_LINE2_MESSAGE                 ("KAIQUN")

#define MSEC_TO_SEC                       (1000.0)
#define LCD_COMMAND_WAIT_MSEC             (1.0)
#define LCD_COMMAND_WAIT_SEC              (LCD_COMMAND_WAIT_MSEC / MSEC_TO_SEC)
#define LCD_CLEAR_WAIT_MSEC               (2.0)
#define LCD_CLEAR_WAIT_SEC                (LCD_CLEAR_WAIT_MSEC / MSEC_TO_SEC)
#define LCD_INIT_WAIT_MSEC                (200.0)
#define LCD_INIT_WAIT_SEC                 (LCD_INIT_WAIT_MSEC / MSEC_TO_SEC)
#define LCD_DISPLAY_WAIT_MSEC             (5000.0)
#define LCD_DISPLAY_WAIT_SEC              (LCD_DISPLAY_WAIT_MSEC / MSEC_TO_SEC)

/***********************************************************
 * プロトタイプ宣言
 ***********************************************************/
S4 s4_WriteLcd(S4 s4_pi, S4 s4_handle, U1 u1_control, U1 u1_value);
S4 s4_InitLcd(S4 s4_pi, S4 *ps4_handle);
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
 * @brief 制御バイトと値を1バイトLCDへI2C送信する。
 * @param[in] S4 s4_pi pigpio接続番号
 * @param[in] S4 s4_handle I2Cハンドル
 * @param[in] U1 u1_control 制御バイト(コマンド/データ選択)
 * @param[in] U1 u1_value 送信する値
 * @retval S4 0 正常終了
 * @retval S4 1 異常終了
 ***********************************************************/
S4 s4_WriteLcd(S4 s4_pi, S4 s4_handle, U1 u1_control, U1 u1_value)
{
	S4 s4_ret;

	s4_ret = i2c_write_byte_data(s4_pi, s4_handle, u1_control, u1_value);

	(VD)time_sleep(LCD_COMMAND_WAIT_SEC);

	if (s4_ret < 0) {
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
	static const U1 u1a_initCmd[LCD_INIT_CMD_NUM] = {
		LCD_CMD_FUNCTION_SET_NORMAL,    /* 0:機能設定(IS=0)            */
		LCD_CMD_FUNCTION_SET_EXTEND,    /* 1:機能設定(IS=1 拡張命令)   */
		LCD_CMD_INTERNAL_OSC,           /* 2:内部OSC周波数             */
		LCD_CMD_CONTRAST_LOW,           /* 3:コントラスト下位          */
		LCD_CMD_POWER_ICON_CONTRAST,    /* 4:Power/ICON/Contrast上位   */
		LCD_CMD_FOLLOWER_CONTROL,       /* 5:フォロワ制御(昇圧ON)      */
		LCD_CMD_FUNCTION_SET_NORMAL,    /* 6:機能設定(IS=0 通常命令へ) */
		LCD_CMD_DISPLAY_ON,             /* 7:表示ON                    */
		LCD_CMD_CLEAR_DISPLAY           /* 8:表示クリア                */
	};
	S4 s4_handle;
	S4 s4_ret;
	U4 u4_loopCnt;

	if (ps4_handle == NULL) {
		return 1;
	}

	s4_handle = i2c_open(s4_pi, LCD_I2C_BUS, LCD_I2C_ADDR, LCD_I2C_FLAGS);
	if (s4_handle < 0) {
		return 1;
	}

	*ps4_handle = s4_handle;

	for (u4_loopCnt = 0; u4_loopCnt < LCD_INIT_CMD_NUM; u4_loopCnt++) {
		s4_ret = s4_WriteLcd(s4_pi, s4_handle, LCD_CONTROL_COMMAND,
		                     u1a_initCmd[u4_loopCnt]);
		if (s4_ret != 0) {
			return 1;
		} else {
			/* DO NOTHING */
		}

		/* フォロワ制御送信後は昇圧回路の安定を待つ */
		if (u4_loopCnt == LCD_INIT_WAIT_INDEX) {
			(VD)time_sleep(LCD_INIT_WAIT_SEC);
		} else {
			/* DO NOTHING */
		}
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
	U1 u1_ddramAddr;
	U1 u1_command;
	U4 u4_length;
	U4 u4_loopCnt;
	S4 s4_ret;

	if (pch_message == NULL) {
		return 1;
	}

	if (u1_line == LCD_LINE1) {
		u1_ddramAddr = LCD_DDRAM_LINE1;
	} else if (u1_line == LCD_LINE2) {
		u1_ddramAddr = LCD_DDRAM_LINE2;
	} else {
		return 1;
	}

	/* DDRAMアドレスセットコマンド(0x80 + アドレス)で書き込み位置を指定する */
	u1_command = (U1)(LCD_SET_DDRAM_BASE + u1_ddramAddr);
	s4_ret = s4_WriteLcd(s4_pi, s4_handle, LCD_CONTROL_COMMAND, u1_command);
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
		s4_ret = s4_WriteLcd(s4_pi, s4_handle, LCD_CONTROL_DATA,
		                     (U1)pch_message[u4_loopCnt]);
		if (s4_ret != 0) {
			return 1;
		} else {
			/* DO NOTHING */
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

	s4_ret = s4_WriteLcd(s4_pi, s4_handle, LCD_CONTROL_COMMAND, LCD_CMD_CLEAR_DISPLAY);
	if (s4_ret != 0) {
		return 1;
	}

	/* Clear Display は実行に時間がかかるため追加で待機する */
	(VD)time_sleep(LCD_CLEAR_WAIT_SEC);

	return 0;
}
