/***********************************************************
 * @file    bmp180_task11.c
 * @brief   BMP180から温度、気圧、標高を取得するプログラム
 * @version 1.1.0
 * @date    2026.06.16
 * @author  Luo Kaiqun
 ***********************************************************/

#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <pigpiod_if2.h>
#include "type.h"

/***********************************************************
 * 構造体定義
 ***********************************************************/
typedef struct {
	S2 s2_ac1;
	S2 s2_ac2;
	S2 s2_ac3;
	U2 u2_ac4;
	U2 u2_ac5;
	U2 u2_ac6;
	S2 s2_b1;
	S2 s2_b2;
	S2 s2_mb;
	S2 s2_mc;
	S2 s2_md;
} ST_BMP180_CALIB;

/***********************************************************
 * 構造体定義（校正係数読み出しテーブル用）
 ***********************************************************/
typedef struct {
	U1  u1_signType;   /* 符号種別 BMP180_CALIB_SIGNED / UNSIGNED */
	VD *pv_dst;        /* 格納先ポインタ(S2*またはU2*をvoid*として保持) */
} ST_CALIB_ENTRY;

/***********************************************************
 * マクロ定義
 ***********************************************************/
#define BMP180_I2C_BUS                    (1U)            /* Raspberry Piで使用するI2Cバス番号 */
#define BMP180_I2C_ADDR                   (0x77U)         /* BMP180のI2C 7bitアドレス */
#define BMP180_I2C_FLAGS                  (0U)            /* I2C通信時のフラグ */

#define BMP180_ADDR_WRITE_8BIT            (0xEEU)         /* データシート上の書き込み用8bitアドレス */
#define BMP180_ADDR_READ_8BIT             (0xEFU)         /* データシート上の読み込み用8bitアドレス */

#define BMP180_REG_CHIP_ID                (0xD0U)         /* Chip IDレジスタアドレス */
#define BMP180_CHIP_ID_VALUE              (0x55U)         /* BMP180のChip ID固定値 */
#define BMP180_REG_CTRL_MEAS              (0xF4U)         /* 測定制御レジスタアドレス */
#define BMP180_REG_OUT_MSB                (0xF6U)         /* 測定結果MSBレジスタアドレス */
#define BMP180_REG_OUT_LSB                (0xF7U)         /* 測定結果LSBレジスタアドレス */
#define BMP180_REG_OUT_XLSB               (0xF8U)         /* 測定結果XLSBレジスタアドレス */

#define BMP180_CMD_TEMP                   (0x2EU)         /* 温度測定開始命令 */
#define BMP180_CMD_PRESS_OSS0             (0x34U)         /* oss=0の気圧測定開始命令 */
#define BMP180_OSS                        (0U)            /* 気圧測定のoversampling setting */

#define BMP180_TEMP_WAIT_SEC              (0.005)         /* 温度測定後の待ち時間[s] */
#define BMP180_PRESS_WAIT_SEC             (0.005)         /* oss=0の気圧測定後の待ち時間[s] */

#define BMP180_CALIB_BASE_ADDR            (0xAAU)         /* 校正係数の先頭アドレス(AC1 MSB) */
#define BMP180_CALIB_STEP                 (2U)            /* 各係数のバイト数(=次アドレスへの間隔) */
#define BMP180_CALIB_COUNT                (11U)           /* 校正係数の総数 */

#define BMP180_CALIB_SIGNED               (1U)            /* 符号あり係数を表す識別子 */
#define BMP180_CALIB_UNSIGNED             (0U)            /* 符号なし係数を表す識別子 */

#define BMP180_BYTE_SHIFT                 (8U)            /* MSBを上位8bitへ移動するシフト量 */
#define BMP180_PRESS_SHIFT_BASE           (8U)            /* UP算出時の基本シフト量 */
#define BMP180_PRESS_SHIFT                (BMP180_PRESS_SHIFT_BASE - BMP180_OSS)

#define BMP180_TEMP_DIV                   (10)            /* 0.1℃単位を℃表示へ変換する値 */
#define BMP180_HPA_DIV                    (100)           /* Pa単位をhPa表示へ変換する値 */

#define BMP180_SEA_LEVEL_PRESSURE_PA      (101325.0)      /* 標高算出に使用する海面気圧[Pa] */
#define BMP180_ALTITUDE_COEFFICIENT       (44330.0)       /* 標高算出式で使用する係数 */
#define BMP180_ALTITUDE_EXPONENT          (1.0 / 5.255)   /* 標高算出式で使用する指数 */

#define BMP180_SIGN_BIT_U2                (0x8000U)       /* 16bit値の符号ビット */
#define BMP180_SIGN_EXTEND_U2             (0x10000)       /* 16bit符号拡張に使用する値 */

#define BMP180_CALC_4000                  (4000)
#define BMP180_CALC_32768                 (32768U)
#define BMP180_CALC_50000                 (50000U)
#define BMP180_CALC_0X80000000            (0x80000000U)
#define BMP180_CALC_3038                  (3038)
#define BMP180_CALC_7357                  (7357)
#define BMP180_CALC_3791                  (3791)

/***********************************************************
 * プロトタイプ宣言
 ***********************************************************/
S4 s4_OpenBmp180(S4 s4_pi, S4 *ps4_handle);
S4 s4_CheckChipId(S4 s4_pi, S4 s4_handle);
S4 s4_ReadCalibration(S4 s4_pi, S4 s4_handle, ST_BMP180_CALIB *pst_calib);
S4 s4_ReadS2Data(S4 s4_pi, S4 s4_handle, U1 u1_regAddr, S2 *ps2_data);
S4 s4_ReadU2Data(S4 s4_pi, S4 s4_handle, U1 u1_regAddr, U2 *pu2_data);
S4 s4_ReadRawTemperature(S4 s4_pi, S4 s4_handle, S4 *ps4_ut);
S4 s4_ReadRawPressure(S4 s4_pi, S4 s4_handle, S4 *ps4_up);
S4 s4_CalcTemperature(ST_BMP180_CALIB *pst_calib, S4 s4_ut, S4 *ps4_temperature, S4 *ps4_b5);
S4 s4_CalcPressure(ST_BMP180_CALIB *pst_calib, S4 s4_up, S4 s4_b5, S4 *ps4_pressure);
DL dl_CalcAltitude(S4 s4_pressure, DL dl_seaLevelPressure);
VD vd_PrintResult(S4 s4_temperature, S4 s4_pressure, DL dl_altitude);

/***********************************************************
 * @brief BMP180から温度、気圧、標高を取得し、標準出力へ表示する。
 * @param なし
 * @retval int 0 正常終了 1 異常終了
 ***********************************************************/
int main(void)
{
	S4 s4_pi;
	S4 s4_handle;
	S4 s4_ret;
	S4 s4_ut;
	S4 s4_up;
	S4 s4_temperature;
	S4 s4_pressure;
	S4 s4_b5;
	DL dl_altitude;
	ST_BMP180_CALIB st_calib;

	s4_handle = -1;

	s4_pi = pigpio_start(NULL, NULL);
	if (s4_pi < 0) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_ret = s4_OpenBmp180(s4_pi, &s4_handle);
	if (s4_ret != 0) {
		pigpio_stop(s4_pi);
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_ret = s4_CheckChipId(s4_pi, s4_handle);
	if (s4_ret != 0) {
		(VD)i2c_close(s4_pi, s4_handle);
		pigpio_stop(s4_pi);
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_ret = s4_ReadCalibration(s4_pi, s4_handle, &st_calib);
	if (s4_ret != 0) {
		(VD)i2c_close(s4_pi, s4_handle);
		pigpio_stop(s4_pi);
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_ret = s4_ReadRawTemperature(s4_pi, s4_handle, &s4_ut);
	if (s4_ret != 0) {
		(VD)i2c_close(s4_pi, s4_handle);
		pigpio_stop(s4_pi);
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_ret = s4_ReadRawPressure(s4_pi, s4_handle, &s4_up);
	if (s4_ret != 0) {
		(VD)i2c_close(s4_pi, s4_handle);
		pigpio_stop(s4_pi);
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_ret = s4_CalcTemperature(&st_calib, s4_ut, &s4_temperature, &s4_b5);
	if (s4_ret != 0) {
		(VD)i2c_close(s4_pi, s4_handle);
		pigpio_stop(s4_pi);
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_ret = s4_CalcPressure(&st_calib, s4_up, s4_b5, &s4_pressure);
	if (s4_ret != 0) {
		(VD)i2c_close(s4_pi, s4_handle);
		pigpio_stop(s4_pi);
		return 1;
	} else {
		/* DO NOTHING */
	}

	dl_altitude = dl_CalcAltitude(s4_pressure, BMP180_SEA_LEVEL_PRESSURE_PA);

	vd_PrintResult(s4_temperature, s4_pressure, dl_altitude);

	(VD)i2c_close(s4_pi, s4_handle);
	pigpio_stop(s4_pi);

	return 0;
}

/***********************************************************
 * @brief BMP180をI2Cでオープンする。
 * @param S4 s4_pi pigpio_startの返却値
 * @param S4 *ps4_handle I2Cハンドル格納先
 * @retval S4 0 正常終了 1 異常終了
 ***********************************************************/
S4 s4_OpenBmp180(S4 s4_pi, S4 *ps4_handle)
{
	S4 s4_handle;

	if (ps4_handle == NULL) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_handle = i2c_open(s4_pi, BMP180_I2C_BUS, BMP180_I2C_ADDR, BMP180_I2C_FLAGS);
	if (s4_handle < 0) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	*ps4_handle = s4_handle;

	return 0;
}

/***********************************************************
 * @brief BMP180のChip IDを確認する。
 * @param S4 s4_pi pigpio_startの返却値
 * @param S4 s4_handle I2Cハンドル
 * @retval S4 0 正常終了 1 異常終了
 ***********************************************************/
S4 s4_CheckChipId(S4 s4_pi, S4 s4_handle)
{
	S4 s4_chipId;

	s4_chipId = i2c_read_byte_data(s4_pi, s4_handle, BMP180_REG_CHIP_ID);
	if (s4_chipId < 0) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	if ((U4)s4_chipId == BMP180_CHIP_ID_VALUE) {
		return 0;
	} else {
		return 1;
	}
}

/***********************************************************
 * @brief BMP180の校正係数を読み出す。
 *        係数の符号種別と格納先をテーブル化し、ループで読み出す。
 *        アドレスは先頭アドレス + index * STEP で算出する。
 * @param S4 s4_pi pigpio_startの返却値
 * @param S4 s4_handle I2Cハンドル
 * @param ST_BMP180_CALIB *pst_calib 校正係数格納先
 * @retval S4 0 正常終了 1 異常終了
 ***********************************************************/
S4 s4_ReadCalibration(S4 s4_pi, S4 s4_handle, ST_BMP180_CALIB *pst_calib)
{
	U1 u1_idx;
	U1 u1_regAddr;
	S4 s4_ret;

	if (pst_calib == NULL) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	/* 各係数の符号種別と格納先を一覧化したテーブル。           */
	/* アドレスは BASE + idx * STEP で算出するため保持しない。 */
	/* 格納先はvoid*として保持し、符号種別に応じてキャストする。 */
	const ST_CALIB_ENTRY st_table[BMP180_CALIB_COUNT] = {
		{ BMP180_CALIB_SIGNED,   &pst_calib->s2_ac1 },
		{ BMP180_CALIB_SIGNED,   &pst_calib->s2_ac2 },
		{ BMP180_CALIB_SIGNED,   &pst_calib->s2_ac3 },
		{ BMP180_CALIB_UNSIGNED, &pst_calib->u2_ac4 },
		{ BMP180_CALIB_UNSIGNED, &pst_calib->u2_ac5 },
		{ BMP180_CALIB_UNSIGNED, &pst_calib->u2_ac6 },
		{ BMP180_CALIB_SIGNED,   &pst_calib->s2_b1  },
		{ BMP180_CALIB_SIGNED,   &pst_calib->s2_b2  },
		{ BMP180_CALIB_SIGNED,   &pst_calib->s2_mb  },
		{ BMP180_CALIB_SIGNED,   &pst_calib->s2_mc  },
		{ BMP180_CALIB_SIGNED,   &pst_calib->s2_md  }
	};

	for (u1_idx = 0U; u1_idx < BMP180_CALIB_COUNT; u1_idx++) {
		/* 先頭アドレスにidx分のオフセットを加算してアドレスを算出する。 */
		u1_regAddr = (U1)(BMP180_CALIB_BASE_ADDR + (u1_idx * BMP180_CALIB_STEP));

		if (st_table[u1_idx].u1_signType == BMP180_CALIB_SIGNED) {
			/* void*を真の型S2*へ戻して渡す。整合性はテーブル定義で保証。 */
			s4_ret = s4_ReadS2Data(s4_pi, s4_handle, u1_regAddr,
			                       (S2 *)st_table[u1_idx].pv_dst);
		} else {
			/* void*を真の型U2*へ戻して渡す。整合性はテーブル定義で保証。 */
			s4_ret = s4_ReadU2Data(s4_pi, s4_handle, u1_regAddr,
			                       (U2 *)st_table[u1_idx].pv_dst);
		}

		if (s4_ret != 0) {
			return 1;
		} else {
			/* DO NOTHING */
		}
	}

	return 0;
}

/***********************************************************
 * @brief 2バイトの符号ありデータを読み出す。
 * @param S4 s4_pi pigpio_startの返却値
 * @param S4 s4_handle I2Cハンドル
 * @param U1 u1_regAddr MSB側レジスタアドレス
 * @param S2 *ps2_data 読み出しデータ格納先
 * @retval S4 0 正常終了 1 異常終了
 ***********************************************************/
S4 s4_ReadS2Data(S4 s4_pi, S4 s4_handle, U1 u1_regAddr, S2 *ps2_data)
{
	S4 s4_msb;
	S4 s4_lsb;
	U4 u4_data;
	S4 s4_signedData;

	if (ps2_data == NULL) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_msb = i2c_read_byte_data(s4_pi, s4_handle, u1_regAddr);
	if (s4_msb < 0) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_lsb = i2c_read_byte_data(s4_pi, s4_handle, (U1)(u1_regAddr + 1U));
	if (s4_lsb < 0) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	u4_data = (((U4)s4_msb) << BMP180_BYTE_SHIFT) + (U4)s4_lsb;

	/* 16bitの符号ビット(bit15)が立っている場合は負数とみなす。      */
	/* (S2)への直接キャストは値が範囲外のとき処理系定義動作となるため、 */
	/* S4の範囲内で減算し符号拡張を明示的に行う。                     */
	if ((u4_data & BMP180_SIGN_BIT_U2) != 0U) {
		s4_signedData = (S4)u4_data - BMP180_SIGN_EXTEND_U2;
	} else {
		s4_signedData = (S4)u4_data;
	}

	*ps2_data = (S2)s4_signedData;

	return 0;
}

/***********************************************************
 * @brief 2バイトの符号なしデータを読み出す。
 * @param S4 s4_pi pigpio_startの返却値
 * @param S4 s4_handle I2Cハンドル
 * @param U1 u1_regAddr MSB側レジスタアドレス
 * @param U2 *pu2_data 読み出しデータ格納先
 * @retval S4 0 正常終了 1 異常終了
 ***********************************************************/
S4 s4_ReadU2Data(S4 s4_pi, S4 s4_handle, U1 u1_regAddr, U2 *pu2_data)
{
	S4 s4_msb;
	S4 s4_lsb;
	U4 u4_data;

	if (pu2_data == NULL) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_msb = i2c_read_byte_data(s4_pi, s4_handle, u1_regAddr);
	if (s4_msb < 0) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_lsb = i2c_read_byte_data(s4_pi, s4_handle, (U1)(u1_regAddr + 1U));
	if (s4_lsb < 0) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	u4_data = (((U4)s4_msb) << BMP180_BYTE_SHIFT) + (U4)s4_lsb;
	*pu2_data = (U2)u4_data;

	return 0;
}

/***********************************************************
 * @brief 未補正温度UTを読み出す。
 * @param S4 s4_pi pigpio_startの返却値
 * @param S4 s4_handle I2Cハンドル
 * @param S4 *ps4_ut 未補正温度UT格納先
 * @retval S4 0 正常終了 1 異常終了
 ***********************************************************/
S4 s4_ReadRawTemperature(S4 s4_pi, S4 s4_handle, S4 *ps4_ut)
{
	S4 s4_msb;
	S4 s4_lsb;
	S4 s4_ret;

	if (ps4_ut == NULL) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_ret = i2c_write_byte_data(s4_pi, s4_handle, BMP180_REG_CTRL_MEAS, BMP180_CMD_TEMP);
	if (s4_ret != 0) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	(VD)time_sleep(BMP180_TEMP_WAIT_SEC);

	s4_msb = i2c_read_byte_data(s4_pi, s4_handle, BMP180_REG_OUT_MSB);
	if (s4_msb < 0) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_lsb = i2c_read_byte_data(s4_pi, s4_handle, BMP180_REG_OUT_LSB);
	if (s4_lsb < 0) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	*ps4_ut = (S4)((((U4)s4_msb) << BMP180_BYTE_SHIFT) + (U4)s4_lsb);

	return 0;
}

/***********************************************************
 * @brief 未補正気圧UPを読み出す。
 * @param S4 s4_pi pigpio_startの返却値
 * @param S4 s4_handle I2Cハンドル
 * @param S4 *ps4_up 未補正気圧UP格納先
 * @retval S4 0 正常終了 1 異常終了
 ***********************************************************/
S4 s4_ReadRawPressure(S4 s4_pi, S4 s4_handle, S4 *ps4_up)
{
	S4 s4_msb;
	S4 s4_lsb;
	S4 s4_xlsb;
	S4 s4_ret;
	S4 s4_raw;

	if (ps4_up == NULL) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_ret = i2c_write_byte_data(s4_pi, s4_handle, BMP180_REG_CTRL_MEAS, BMP180_CMD_PRESS_OSS0);
	if (s4_ret != 0) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	(VD)time_sleep(BMP180_PRESS_WAIT_SEC);

	s4_msb = i2c_read_byte_data(s4_pi, s4_handle, BMP180_REG_OUT_MSB);
	if (s4_msb < 0) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_lsb = i2c_read_byte_data(s4_pi, s4_handle, BMP180_REG_OUT_LSB);
	if (s4_lsb < 0) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_xlsb = i2c_read_byte_data(s4_pi, s4_handle, BMP180_REG_OUT_XLSB);
	if (s4_xlsb < 0) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_raw = (S4)((((U4)s4_msb) << 16U) + (((U4)s4_lsb) << 8U) + (U4)s4_xlsb);
	*ps4_up = s4_raw >> BMP180_PRESS_SHIFT;

	return 0;
}

/***********************************************************
 * @brief 未補正温度UTから真の温度を算出する。
 * @param ST_BMP180_CALIB *pst_calib 校正係数
 * @param S4 s4_ut 未補正温度UT
 * @param S4 *ps4_temperature 0.1℃単位の温度格納先
 * @param S4 *ps4_b5 気圧計算用中間値格納先
 * @retval S4 0 正常終了 1 異常終了
 ***********************************************************/
S4 s4_CalcTemperature(ST_BMP180_CALIB *pst_calib, S4 s4_ut, S4 *ps4_temperature, S4 *ps4_b5)
{
	S4 s4_x1;
	S4 s4_x2;
	S4 s4_b5;

	if (pst_calib == NULL) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	if (ps4_temperature == NULL) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	if (ps4_b5 == NULL) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_x1 = ((s4_ut - (S4)pst_calib->u2_ac6) * (S4)pst_calib->u2_ac5) >> 15U;
	if ((s4_x1 + (S4)pst_calib->s2_md) == 0) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_x2 = ((S4)pst_calib->s2_mc << 11U) / (s4_x1 + (S4)pst_calib->s2_md);
	s4_b5 = s4_x1 + s4_x2;

	*ps4_temperature = (s4_b5 + 8) >> 4U;
	*ps4_b5 = s4_b5;

	return 0;
}

/***********************************************************
 * @brief 未補正気圧UPから真の気圧を算出する。
 * @param ST_BMP180_CALIB *pst_calib 校正係数
 * @param S4 s4_up 未補正気圧UP
 * @param S4 s4_b5 温度計算で得た中間値B5
 * @param S4 *ps4_pressure Pa単位の気圧格納先
 * @retval S4 0 正常終了 1 異常終了
 ***********************************************************/
S4 s4_CalcPressure(ST_BMP180_CALIB *pst_calib, S4 s4_up, S4 s4_b5, S4 *ps4_pressure)
{
	S4 s4_b6;
	S4 s4_x1;
	S4 s4_x2;
	S4 s4_x3;
	S4 s4_b3;
	U4 u4_b4;
	U4 u4_b7;
	S4 s4_p;

	if (pst_calib == NULL) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	if (ps4_pressure == NULL) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	s4_b6 = s4_b5 - BMP180_CALC_4000;

	s4_x1 = ((S4)pst_calib->s2_b2 * ((s4_b6 * s4_b6) >> 12U)) >> 11U;
	s4_x2 = ((S4)pst_calib->s2_ac2 * s4_b6) >> 11U;
	s4_x3 = s4_x1 + s4_x2;
	s4_b3 = (((((S4)pst_calib->s2_ac1 * 4) + s4_x3) << BMP180_OSS) + 2) >> 2U;

	s4_x1 = ((S4)pst_calib->s2_ac3 * s4_b6) >> 13U;
	s4_x2 = ((S4)pst_calib->s2_b1 * ((s4_b6 * s4_b6) >> 12U)) >> 16U;
	s4_x3 = (s4_x1 + s4_x2 + 2) >> 2U;
	u4_b4 = ((U4)pst_calib->u2_ac4 * (U4)(s4_x3 + (S4)BMP180_CALC_32768)) >> 15U;

	if (u4_b4 == 0U) {
		return 1;
	} else {
		/* DO NOTHING */
	}

	u4_b7 = ((U4)s4_up - (U4)s4_b3) * (BMP180_CALC_50000 >> BMP180_OSS);

	if (u4_b7 < BMP180_CALC_0X80000000) {
		s4_p = (S4)((u4_b7 * 2U) / u4_b4);
	} else {
		s4_p = (S4)((u4_b7 / u4_b4) * 2U);
	}

	s4_x1 = (s4_p >> 8U) * (s4_p >> 8U);
	s4_x1 = (s4_x1 * BMP180_CALC_3038) >> 16U;
	s4_x2 = (-BMP180_CALC_7357 * s4_p) >> 16U;
	s4_p = s4_p + ((s4_x1 + s4_x2 + BMP180_CALC_3791) >> 4U);

	*ps4_pressure = s4_p;

	return 0;
}

/***********************************************************
 * @brief 気圧から標高を算出する。
 * @param S4 s4_pressure Pa単位の気圧
 * @param DL dl_seaLevelPressure Pa単位の海面気圧
 * @retval DL m単位の標高
 ***********************************************************/
DL dl_CalcAltitude(S4 s4_pressure, DL dl_seaLevelPressure)
{
	DL dl_pressure;
	DL dl_ratio;
	DL dl_altitude;

	if (dl_seaLevelPressure <= 0.0) {
		return 0.0;
	} else {
		/* DO NOTHING */
	}

	if (s4_pressure <= 0) {
		return 0.0;
	} else {
		/* DO NOTHING */
	}

	dl_pressure = (DL)s4_pressure;
	dl_ratio = dl_pressure / dl_seaLevelPressure;
	dl_altitude = BMP180_ALTITUDE_COEFFICIENT * (1.0 - pow(dl_ratio, BMP180_ALTITUDE_EXPONENT));

	return dl_altitude;
}

/***********************************************************
 * @brief 温度、気圧、標高を標準出力へ表示する。
 * @param S4 s4_temperature 0.1℃単位の温度
 * @param S4 s4_pressure Pa単位の気圧
 * @param DL dl_altitude m単位の標高
 * @retval なし
 ***********************************************************/
VD vd_PrintResult(S4 s4_temperature, S4 s4_pressure, DL dl_altitude)
{
	S4 s4_tempInt;
	S4 s4_tempDec;
	S4 s4_hpaInt;
	S4 s4_hpaDec;

	s4_tempInt = s4_temperature / BMP180_TEMP_DIV;
	s4_tempDec = s4_temperature % BMP180_TEMP_DIV;
	if (s4_tempDec < 0) {
		s4_tempDec = -s4_tempDec;
	} else {
		/* DO NOTHING */
	}

	s4_hpaInt = s4_pressure / BMP180_HPA_DIV;
	s4_hpaDec = s4_pressure % BMP180_HPA_DIV;
	if (s4_hpaDec < 0) {
		s4_hpaDec = -s4_hpaDec;
	} else {
		/* DO NOTHING */
	}

	(VD)printf("Temperature: %d.%d C\n", s4_tempInt, s4_tempDec);
	(VD)printf("Pressure: %d Pa\n", s4_pressure);
	(VD)printf("Pressure: %d.%02d hPa\n", s4_hpaInt, s4_hpaDec);
	(VD)printf("Altitude: %.2f m\n", dl_altitude);
}
