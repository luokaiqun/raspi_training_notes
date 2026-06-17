/***********************************************************
 * @file    lcd.c
 * @brief   AE-AQM0802に苗字と名前を表示するプログラム
 * @version 1.0.0
 * @date    2026.06.11
 * @author  Luo Kaiqun
 ***********************************************************/

#include <stddef.h>
#include <pigpiod_if2.h>
#include "type.h"

/***********************************************************
 * マクロ定義
 ***********************************************************/
#define FUNC_RET_NORMAL                   (0)        /* 自作関数の正常終了 */
#define FUNC_RET_ABNORMAL                 (1)        /* 自作関数の異常終了 */

/*
 * 自作関数の戻り値判定にはFUNC_RET_NORMAL/FUNC_RET_ABNORMALを使用する。
 * pigpioライブラリ関数の戻り値判定は各関数仕様に従い、0、0未満などの値で判定する。
 */

#define LCD_I2C_BUS                       (1U)       /* Raspberry Piで使用するI2Cバス番号 */
#define LCD_I2C_ADDR                      (0x3EU)    /* AE-AQM0802のI2C 7bitアドレス */
#define LCD_I2C_FLAGS                     (0U)       /* I2C通信時のフラグ */

#define LCD_CONTROL_COMMAND               (0x00U)    /* LCDへ命令を書き込むためのコントロールバイト */
#define LCD_CONTROL_DATA                  (0x40U)    /* LCDへ表示データを書き込むためのコントロールバイト */

#define LCD_CMD_FUNCTION_SET_IS0          (0x38U)    /* 通常命令モードでFunction Setを行う命令 */
#define LCD_CMD_FUNCTION_SET_IS1          (0x39U)    /* 拡張命令モードでFunction Setを行う命令 */
#define LCD_CMD_INTERNAL_OSC              (0x14U)    /* 内部発振周波数を設定する命令 */
#define LCD_CMD_CONTRAST_SET              (0x70U)    /* コントラスト下位ビットを設定する命令 */
#define LCD_CMD_POWER_ICON_CONTRAST       (0x56U)    /* 電源、アイコン、コントラスト上位ビットを設定する命令 */
#define LCD_CMD_FOLLOWER_CONTROL          (0x6CU)    /* フォロワ回路を設定する命令 */
#define LCD_CMD_DISPLAY_ON                (0x0CU)    /* LCD表示をONにする命令 */
#define LCD_CMD_CLEAR_DISPLAY             (0x01U)    /* LCD表示をクリアする命令 */

#define LCD_SET_DDRAM_BASE                (0x80U)    /* DDRAMアドレス設定命令の基準値 */
#define LCD_DDRAM_LINE1                   (0x00U)    /* LCD1行目先頭のDDRAMアドレス */
#define LCD_DDRAM_LINE2                   (0x40U)    /* LCD2行目先頭のDDRAMアドレス */

#define LCD_LINE1                         (1U)       /* LCD1行目を示す値 */
#define LCD_LINE2                         (2U)       /* LCD2行目を示す値 */
#define LCD_LINE_LENGTH                   (8U)       /* LCD1行あたりの表示可能文字数 */

#define LCD_INIT_CMD_NUM                  (9U)       /* LCD初期化命令の個数 */

#define LCD_POWER_WAIT_SEC                (0.04)     /* LCD電源投入後の待ち時間[s] */
#define LCD_COMMAND_WAIT_SEC              (0.00003)  /* 通常命令送信後の待ち時間[s] */
#define LCD_CLEAR_WAIT_SEC                (0.002)    /* クリア命令送信後の待ち時間[s] */
#define LCD_INIT_WAIT_SEC                 (0.2)      /* フォロワ回路設定後の待ち時間[s] */
#define LCD_DISPLAY_WAIT_SEC              (5.0)      /* LCD表示後の待ち時間[s] */

#define FIRST_NAME_LENGTH                 (6U)       /* 名前の文字数 */
#define LAST_NAME_LENGTH                  (3U)       /* 苗字の文字数 */

#define LETTER_L                          (0x4CU)    /* 文字'L'の文字コード */
#define LETTER_U                          (0x55U)    /* 文字'U'の文字コード */
#define LETTER_O                          (0x4FU)    /* 文字'O'の文字コード */
#define LETTER_K                          (0x4BU)    /* 文字'K'の文字コード */
#define LETTER_A                          (0x41U)    /* 文字'A'の文字コード */
#define LETTER_I                          (0x49U)    /* 文字'I'の文字コード */
#define LETTER_Q                          (0x51U)    /* 文字'Q'の文字コード */
#define LETTER_N                          (0x4EU)    /* 文字'N'の文字コード */

/***********************************************************
 * プロトタイプ宣言
 ***********************************************************/
S4 s4_SendToLcd(S4 s4_pi, S4 s4_handle, U1 u1_control, U1 u1_value);
S4 s4_InitLcd(S4 s4_pi, S4 *ps4_handle);
S4 s4_DisplayLcd(S4 s4_pi, S4 s4_handle, U1 u1_line, U1 *pu1_letterArray, U4 u4_length);
S4 s4_ClearLcd(S4 s4_pi, S4 s4_handle);

/***********************************************************
 * @brief LCDに苗字と名前を表示し、5秒後に表示をクリアする。
 * @param なし
 * @retval int 0 正常終了 1 異常終了
 ***********************************************************/
int main(void)
{
	S4 s4_pi;
	S4 s4_lcdHandle;
	S4 s4_funcRet;
	S4 s4_callRet;
	U1 u1_firstNameArray[FIRST_NAME_LENGTH] = {
		LETTER_K,
		LETTER_A,
		LETTER_I,
		LETTER_Q,
		LETTER_U,
		LETTER_N
	};
	U1 u1_lastNameArray[LAST_NAME_LENGTH] = {
		LETTER_L,
		LETTER_U,
		LETTER_O
	};

	/* pigpio_start失敗時に終了処理でpigpio_stopを呼ばないため、無効値として-1を設定する。 */
	s4_pi = -1;

	/* i2c_open失敗時に終了処理でi2c_closeを呼ばないため、無効値として-1を設定する。 */
	s4_lcdHandle = -1;

	s4_funcRet = FUNC_RET_NORMAL;
	s4_callRet = FUNC_RET_NORMAL;

	s4_pi = pigpio_start(NULL, NULL);
	if (s4_pi < 0) {
		s4_funcRet = FUNC_RET_ABNORMAL;
	} else {
		/* DO NOTHING */
	}

	if (s4_funcRet == FUNC_RET_NORMAL) {
		s4_callRet = s4_InitLcd(s4_pi, &s4_lcdHandle);
		if (s4_callRet != FUNC_RET_NORMAL) {
			s4_funcRet = FUNC_RET_ABNORMAL;
		} else {
			/* DO NOTHING */
		}
	} else {
		/* DO NOTHING */
	}

	if (s4_funcRet == FUNC_RET_NORMAL) {
		s4_callRet = s4_DisplayLcd(s4_pi, s4_lcdHandle, LCD_LINE1, u1_lastNameArray, LAST_NAME_LENGTH);
		if (s4_callRet != FUNC_RET_NORMAL) {
			s4_funcRet = FUNC_RET_ABNORMAL;
		} else {
			/* DO NOTHING */
		}
	} else {
		/* DO NOTHING */
	}

	if (s4_funcRet == FUNC_RET_NORMAL) {
		s4_callRet = s4_DisplayLcd(s4_pi, s4_lcdHandle, LCD_LINE2, u1_firstNameArray, FIRST_NAME_LENGTH);
		if (s4_callRet != FUNC_RET_NORMAL) {
			s4_funcRet = FUNC_RET_ABNORMAL;
		} else {
			/* DO NOTHING */
		}
	} else {
		/* DO NOTHING */
	}

	if (s4_funcRet == FUNC_RET_NORMAL) {
		time_sleep(LCD_DISPLAY_WAIT_SEC);

		s4_callRet = s4_ClearLcd(s4_pi, s4_lcdHandle);
		if (s4_callRet != FUNC_RET_NORMAL) {
			s4_funcRet = FUNC_RET_ABNORMAL;
		} else {
			/* DO NOTHING */
		}
	} else {
		/* DO NOTHING */
	}

	if (s4_lcdHandle >= 0) {
		(VD)i2c_close(s4_pi, s4_lcdHandle);    /* 終了処理のため、戻り値エラーチェック不要。詳細は設計書参照。 */
	} else {
		/* DO NOTHING */
	}

	if (s4_pi >= 0) {
		pigpio_stop(s4_pi);
	} else {
		/* DO NOTHING */
	}

	return (int)s4_funcRet;
}

/***********************************************************
 * @brief コントロールバイトと値をLCDへI2C送信する。
 * @param S4 s4_pi pigpio_startの返却値
 * @param S4 s4_handle I2Cハンドル
 * @param U1 u1_control コントロールバイト
 * @param U1 u1_value 送信する値
 * @retval S4 FUNC_RET_NORMAL 正常終了  FUNC_RET_ABNORMAL 異常終了
 ***********************************************************/
S4 s4_SendToLcd(S4 s4_pi, S4 s4_handle, U1 u1_control, U1 u1_value)
{
	S4 s4_funcRet;
	S4 s4_callRet;
	DL dl_waitSec;

	s4_funcRet = FUNC_RET_NORMAL;
	s4_callRet = 0;  
	dl_waitSec = LCD_COMMAND_WAIT_SEC;

	s4_callRet = i2c_write_byte_data(s4_pi, s4_handle, u1_control, u1_value);
	if (s4_callRet != 0) {   /* pigpioライブラリ仕様に従い、0以外を異常と判定する。 */
		s4_funcRet = FUNC_RET_ABNORMAL;
	} else {
		/* DO NOTHING */
	}

	if (s4_funcRet == FUNC_RET_NORMAL) {
		if ((u1_control == LCD_CONTROL_COMMAND) && (u1_value == LCD_CMD_CLEAR_DISPLAY)) {
			dl_waitSec = LCD_CLEAR_WAIT_SEC;
		} else if ((u1_control == LCD_CONTROL_COMMAND) && (u1_value == LCD_CMD_FOLLOWER_CONTROL)) {
			dl_waitSec = LCD_INIT_WAIT_SEC;
		} else {
			dl_waitSec = LCD_COMMAND_WAIT_SEC;
		}

		time_sleep(dl_waitSec);
	} else {
		/* DO NOTHING */
	}

	return s4_funcRet;
}

/***********************************************************
 * @brief 引数で指定されたLCDを初期化する。
 * @param S4 s4_pi pigpio_startの返却値
 * @param S4 *ps4_handle I2Cハンドル格納先
 * @retval S4 FUNC_RET_NORMAL 正常終了  FUNC_RET_ABNORMAL 異常終了
 ***********************************************************/
S4 s4_InitLcd(S4 s4_pi, S4 *ps4_handle)
{
	S4 s4_handle;
	S4 s4_funcRet;
	S4 s4_callRet;
	U4 u4_loopCnt;
	U1 u1_initCmd[LCD_INIT_CMD_NUM] = {
		LCD_CMD_FUNCTION_SET_IS0,
		LCD_CMD_FUNCTION_SET_IS1,
		LCD_CMD_INTERNAL_OSC,
		LCD_CMD_CONTRAST_SET,
		LCD_CMD_POWER_ICON_CONTRAST,
		LCD_CMD_FOLLOWER_CONTROL,
		LCD_CMD_FUNCTION_SET_IS0,
		LCD_CMD_DISPLAY_ON,
		LCD_CMD_CLEAR_DISPLAY
	};

	s4_handle = -1;
	s4_funcRet = FUNC_RET_NORMAL;
	s4_callRet = FUNC_RET_NORMAL;

	if (ps4_handle == NULL) {
		s4_funcRet = FUNC_RET_ABNORMAL;
	} else {
		/* DO NOTHING */
	}

	if (s4_funcRet == FUNC_RET_NORMAL) {
		time_sleep(LCD_POWER_WAIT_SEC);

		s4_handle = i2c_open(s4_pi, LCD_I2C_BUS, LCD_I2C_ADDR, LCD_I2C_FLAGS);
		if (s4_handle < 0) {    /* pigpioライブラリ仕様に従い、0未満はエラーコードのため異常と判定する。 */
			s4_funcRet = FUNC_RET_ABNORMAL;
		} else {
			*ps4_handle = s4_handle;
		}
	} else {
		/* DO NOTHING */
	}

	if (s4_funcRet == FUNC_RET_NORMAL) {
		for (u4_loopCnt = 0U;
		     (u4_loopCnt < LCD_INIT_CMD_NUM) && (s4_funcRet == FUNC_RET_NORMAL);
		     u4_loopCnt++) {

			s4_callRet = s4_SendToLcd(s4_pi, s4_handle, LCD_CONTROL_COMMAND, u1_initCmd[u4_loopCnt]);
			if (s4_callRet != FUNC_RET_NORMAL) {
				s4_funcRet = FUNC_RET_ABNORMAL;
			} else {
				/* DO NOTHING */
			}
		}
	} else {
		/* DO NOTHING */
	}

	return s4_funcRet;
}

/***********************************************************
 * @brief LCDの指定行に配列の文字を表示する。
 * @param S4 s4_pi pigpio_startの返却値
 * @param S4 s4_handle I2Cハンドル
 * @param U1 u1_line 表示するLCD行番号
 * @param U1 *pu1_letterArray 文字配列の先頭要素を指すポインタ
 * @param U4 u4_length 表示する文字数
 * @retval S4 FUNC_RET_NORMAL 正常終了  FUNC_RET_ABNORMAL 異常終了
 ***********************************************************/
S4 s4_DisplayLcd(S4 s4_pi, S4 s4_handle, U1 u1_line, U1 *pu1_letterArray, U4 u4_length)
{
	U1 u1_ddramAddr;
	U1 u1_command;
	U4 u4_loopCnt;
	S4 s4_funcRet;
	S4 s4_callRet;

	u1_ddramAddr = LCD_DDRAM_LINE1;
	u1_command = 0U;
	s4_funcRet = FUNC_RET_NORMAL;
	s4_callRet = FUNC_RET_NORMAL;

	if (pu1_letterArray == NULL) {
		s4_funcRet = FUNC_RET_ABNORMAL;
	} else {
		/* DO NOTHING */
	}

	if (s4_funcRet == FUNC_RET_NORMAL) {
		if ((u4_length == 0U) || (u4_length > LCD_LINE_LENGTH)) {
			s4_funcRet = FUNC_RET_ABNORMAL;
		} else {
			/* DO NOTHING */
		}
	} else {
		/* DO NOTHING */
	}

	if (s4_funcRet == FUNC_RET_NORMAL) {
		if (u1_line == LCD_LINE1) {
			u1_ddramAddr = LCD_DDRAM_LINE1;
		} else if (u1_line == LCD_LINE2) {
			u1_ddramAddr = LCD_DDRAM_LINE2;
		} else {
			s4_funcRet = FUNC_RET_ABNORMAL;
		}
	} else {
		/* DO NOTHING */
	}

	if (s4_funcRet == FUNC_RET_NORMAL) {
		/* DDRAMアドレスセットコマンドにより、書き込み開始位置を指定する。 */
		u1_command = (U1)(LCD_SET_DDRAM_BASE + u1_ddramAddr);

		s4_callRet = s4_SendToLcd(s4_pi, s4_handle, LCD_CONTROL_COMMAND, u1_command);
		if (s4_callRet != FUNC_RET_NORMAL) {
			s4_funcRet = FUNC_RET_ABNORMAL;
		} else {
			/* DO NOTHING */
		}
	} else {
		/* DO NOTHING */
	}

	if (s4_funcRet == FUNC_RET_NORMAL) {
		for (u4_loopCnt = 0U;
		     (u4_loopCnt < u4_length) && (s4_funcRet == FUNC_RET_NORMAL);
		     u4_loopCnt++) {

			s4_callRet = s4_SendToLcd(s4_pi, s4_handle, LCD_CONTROL_DATA, pu1_letterArray[u4_loopCnt]);
			if (s4_callRet != FUNC_RET_NORMAL) {
				s4_funcRet = FUNC_RET_ABNORMAL;
			} else {
				/* DO NOTHING */
			}
		}
	} else {
		/* DO NOTHING */
	}

	return s4_funcRet;
}

/***********************************************************
 * @brief LCD表示をクリアする。
 * @param S4 s4_pi pigpio_startの返却値
 * @param S4 s4_handle I2Cハンドル
 * @retval S4 FUNC_RET_NORMAL 正常終了  FUNC_RET_ABNORMAL 異常終了
 ***********************************************************/
S4 s4_ClearLcd(S4 s4_pi, S4 s4_handle)
{
	S4 s4_funcRet;
	S4 s4_callRet;

	s4_funcRet = FUNC_RET_NORMAL;
	s4_callRet = FUNC_RET_NORMAL;

	s4_callRet = s4_SendToLcd(s4_pi, s4_handle, LCD_CONTROL_COMMAND, LCD_CMD_CLEAR_DISPLAY);
	if (s4_callRet != FUNC_RET_NORMAL) {
		s4_funcRet = FUNC_RET_ABNORMAL;
	} else {
		/* DO NOTHING */
	}

	return s4_funcRet;
}
