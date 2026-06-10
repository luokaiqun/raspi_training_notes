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
/* I2C接続設定 */
#define LCD_I2C_BUS                       (1)
#define LCD_I2C_ADDR                      (0x3E)
#define LCD_I2C_FLAGS                     (0)

/* control byte(I2C送信1バイト目) */
#define LCD_CONTROL_COMMAND               (0x00)
#define LCD_CONTROL_DATA                  (0x40)

/* LCD初期化コマンド(AE-AQM0802 / ST7032 データシート準拠) */
#define LCD_CMD_FUNCTION_SET_NORMAL       (0x38)
#define LCD_CMD_FUNCTION_SET_EXTEND       (0x39)
#define LCD_CMD_INTERNAL_OSC              (0x14)
#define LCD_CMD_CONTRAST_LOW              (0x70)
#define LCD_CMD_POWER_ICON_CONTRAST       (0x56)
#define LCD_CMD_FOLLOWER_CONTROL          (0x6C)
#define LCD_CMD_DISPLAY_ON                (0x0C)
#define LCD_CMD_CLEAR_DISPLAY             (0x01)

/* DDRAMアドレス設定コマンド(0x80 + アドレス) */
#define LCD_SET_DDRAM_BASE                (0x80)
#define LCD_DDRAM_LINE1                   (0x00)
#define LCD_DDRAM_LINE2                   (0x40)
#define LCD_CMD_SET_LINE1                 (LCD_SET_DDRAM_BASE + LCD_DDRAM_LINE1)
#define LCD_CMD_SET_LINE2                 (LCD_SET_DDRAM_BASE + LCD_DDRAM_LINE2)

/* 行番号・文字数 */
#define LCD_LINE1                         (1)
#define LCD_LINE2                         (2)
#define LCD_LINE_LENGTH                   (8)

/* 表示メッセージ */
#define LCD_LINE1_MESSAGE                 ("LUO")
#define LCD_LINE2_MESSAGE                 ("KAIQUN")

/* 待ち時間(time_sleepの引数は秒単位) */
#define MSEC_TO_SEC                       (1000.0)
#define LCD_COMMAND_WAIT_MSEC             (1.0)
#define LCD_COMMAND_WAIT_SEC              (LCD_COMMAND_WAIT_MSEC / MSEC_TO_SEC)
#define LCD_INIT_WAIT_MSEC                (200.0)
#define LCD_INIT_WAIT_SEC                 (LCD_INIT_WAIT_MSEC / MSEC_TO_SEC)
#define LCD_CLEAR_WAIT_MSEC               (2.0)
#define LCD_CLEAR_WAIT_SEC                (LCD_CLEAR_WAIT_MSEC / MSEC_TO_SEC)
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

	/* pigpio daemonへ接続する */
	s4_pi = pigpio_start(NULL, NULL);
	if (s4_pi < 0) {
		return 1;
	}

	/* LCDを初期化する */
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

	/* 1行目に苗字を表示する */
	s4_ret = s4_DisplayString(s4_pi, s4_lcdHandle, LCD_LINE1, LCD_LINE1_MESSAGE);
	if (s4_ret != 0) {
		(VD)i2c_close(s4_pi, s4_lcdHandle);
		(VD)pigpio_stop(s4_pi);
		return 1;
	}

	/* 2行目に名前を表示する */
	s4_ret = s4_DisplayString(s4_pi, s4_lcdHandle, LCD_LINE2, LCD_LINE2_MESSAGE);
	if (s4_ret != 0) {
		(VD)i2c_close(s4_pi, s4_lcdHandle);
		(VD)pigpio_stop(s4_pi);
		return 1;
	}

	/* 5秒間表示を保持する */
	(VD)time_sleep(LCD_DISPLAY_WAIT_SEC);

	/* LCD表示をクリアする */
	s4_ret = s4_ClearLcd(s4_pi, s4_lcdHandle);

	/* I2C通信とpigpio接続を終了する */
	(VD)i2c_close(s4_pi, s4_lcdHandle);
	(VD)pigpio_stop(s4_pi);

	if (s4_ret != 0) {
		return 1;
	}

	return 0;
}

/***********************************************************
 * @brief 引数で指定されたLCDをI2Cでオープンし初期化する。
 * @param[in]  S4 s4_pi pigpio接続番号
 * @param[out] S4 *ps4_handle I2Cハンドル格納先
 * @retval S4 0 正常終了
 * @retval S4 1 異常終了
 ***********************************************************/
S4 s4_InitLcd(S4 s4_pi, S4 *ps4_handle)
{
	S4 s4_handle;
	S4 s4_ret;

	/* 出力先ポインタの正当性を確認する */
	if (ps4_handle == NULL) {
		return 1;
	}

	/* LCDとのI2C通信を開始する */
	s4_handle = i2c_open(s4_pi, LCD_I2C_BUS, LCD_I2C_ADDR, LCD_I2C_FLAGS);
	if (s4_handle < 0) {
		return 1;
	}

	*ps4_handle = s4_handle;

	/* 機能設定(通常命令モード) */
	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_FUNCTION_SET_NORMAL);
	if (s4_ret != 0) {
		return 1;
	}

	/* 機能設定(拡張命令モード) */
	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_FUNCTION_SET_EXTEND);
	if (s4_ret != 0) {
		return 1;
	}

	/* 内部OSC周波数設定 */
	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_INTERNAL_OSC);
	if (s4_ret != 0) {
		return 1;
	}

	/* コントラスト下位設定 */
	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_CONTRAST_LOW);
	if (s4_ret != 0) {
		return 1;
	}

	/* Power/ICON/Contrast上位設定 */
	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_POWER_ICON_CONTRAST);
	if (s4_ret != 0) {
		return 1;
	}

	/* フォロワ制御(昇圧ON) */
	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_FOLLOWER_CONTROL);
	if (s4_ret != 0) {
		return 1;
	}

	/* 昇圧回路が安定するまで待つ */
	(VD)time_sleep(LCD_INIT_WAIT_SEC);

	/* 機能設定(通常命令モードへ戻す) */
	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_FUNCTION_SET_NORMAL);
	if (s4_ret != 0) {
		return 1;
	}

	/* 表示ON */
	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_DISPLAY_ON);
	if (s4_ret != 0) {
		return 1;
	}

	/* 表示クリア */
	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_CLEAR_DISPLAY);
	if (s4_ret != 0) {
		return 1;
	}

	return 0;
}

/***********************************************************
 * @brief 引数で指定された命令をLCDへI2Cで送信する。
 * @param[in] S4 s4_pi pigpio接続番号
 * @param[in] S4 s4_handle I2Cハンドル
 * @param[in] U1 u1_command LCDへ送信する命令
 * @retval S4 0 正常終了
 * @retval S4 1 異常終了
 ***********************************************************/
S4 s4_SendCommand(S4 s4_pi, S4 s4_handle, U1 u1_command)
{
	S4 s4_ret;

	/* control byte(LCD_CONTROL_COMMAND)に続けて命令を1バイト送信する */
	s4_ret = i2c_write_byte_data(s4_pi, s4_handle, LCD_CONTROL_COMMAND, u1_command);

	/* 命令実行完了を待つ */
	(VD)time_sleep(LCD_COMMAND_WAIT_SEC);

	if (s4_ret < 0) {
		return 1;
	}

	return 0;
}

/***********************************************************
 * @brief 引数で指定された表示データをLCDへI2Cで送信する。
 * @param[in] S4 s4_pi pigpio接続番号
 * @param[in] S4 s4_handle I2Cハンドル
 * @param[in] U1 u1_data LCDへ送信する表示データ
 * @retval S4 0 正常終了
 * @retval S4 1 異常終了
 ***********************************************************/
S4 s4_SendData(S4 s4_pi, S4 s4_handle, U1 u1_data)
{
	S4 s4_ret;

	/* control byte(LCD_CONTROL_DATA)に続けて表示データを1バイト送信する */
	s4_ret = i2c_write_byte_data(s4_pi, s4_handle, LCD_CONTROL_DATA, u1_data);

	/* 書き込み完了を待つ */
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
	U1 u1_command;
	U4 u4_length;
	U4 u4_loopCnt;
	S4 s4_ret;

	/* 文字列ポインタの正当性を確認する */
	if (pch_message == NULL) {
		return 1;
	}

	/* 行番号からDDRAMアドレス設定コマンドを決定する */
	if (u1_line == LCD_LINE1) {
		u1_command = LCD_CMD_SET_LINE1;
	} else if (u1_line == LCD_LINE2) {
		u1_command = LCD_CMD_SET_LINE2;
	} else {
		return 1;
	}

	/* 書き込み位置(行先頭)を指定する */
	s4_ret = s4_SendCommand(s4_pi, s4_handle, u1_command);
	if (s4_ret != 0) {
		return 1;
	}

	/* 表示文字数を1行の最大文字数に制限する */
	u4_length = (U4)strlen(pch_message);
	if (u4_length > LCD_LINE_LENGTH) {
		u4_length = LCD_LINE_LENGTH;
	} else {
		/* DO NOTHING */
	}

	/* 1文字ずつ表示データとして送信する */
	for (u4_loopCnt = 0; u4_loopCnt < u4_length; u4_loopCnt++) {
		s4_ret = s4_SendData(s4_pi, s4_handle, (U1)pch_message[u4_loopCnt]);
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

	/* 表示クリア命令を送信する */
	s4_ret = s4_SendCommand(s4_pi, s4_handle, LCD_CMD_CLEAR_DISPLAY);
	if (s4_ret != 0) {
		return 1;
	}

	/* Clear Displayは実行に時間がかかるため追加で待機する */
	(VD)time_sleep(LCD_CLEAR_WAIT_SEC);

	return 0;
}
