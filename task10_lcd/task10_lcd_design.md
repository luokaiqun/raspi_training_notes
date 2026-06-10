# 課題10 LCD表示 設計資料

## 1. 課題内容

AE-AQM0802をI2Cで制御し、LCDの1行目に自分の苗字、2行目に自分の名前を表示する。  
5秒後にLCD表示をクリアする。

---

## 2. 外部情報

| 名称 | 値 | 意味 |
|---|---|---|
| `LCD_I2C_BUS` | `(1)` | Raspberry Piで使用するI2Cバス番号 |
| `LCD_I2C_ADDR` | `(0x3E)` | AE-AQM0802のI2C 7bitアドレス |
| `LCD_I2C_FLAGS` | `(0)` | I2C通信時のフラグ |
| `LCD_CONTROL_COMMAND` | `(0x00)` | LCDへ命令を書き込むための制御バイト |
| `LCD_CONTROL_DATA` | `(0x40)` | LCDへ表示データを書き込むための制御バイト |
| `LCD_CMD_FUNCTION_SET_NORMAL` | `(0x38)` | 通常命令モードへ設定する命令 |
| `LCD_CMD_FUNCTION_SET_EXTEND` | `(0x39)` | 拡張命令モードへ設定する命令 |
| `LCD_CMD_INTERNAL_OSC` | `(0x14)` | 内部発振周波数を設定する命令 |
| `LCD_CMD_CONTRAST_LOW` | `(0x70)` | コントラスト下位ビットを設定する命令 |
| `LCD_CMD_POWER_ICON_CONTRAST` | `(0x56)` | 電源、アイコン、コントラスト上位ビットを設定する命令 |
| `LCD_CMD_FOLLOWER_CONTROL` | `(0x6C)` | フォロワ回路を設定する命令 |
| `LCD_CMD_DISPLAY_ON` | `(0x0C)` | LCD表示をONにする命令 |
| `LCD_CMD_CLEAR_DISPLAY` | `(0x01)` | LCD表示をクリアする命令 |
| `LCD_SET_DDRAM_BASE` | `(0x80)` | DDRAMアドレス設定命令の基準値 |
| `LCD_DDRAM_LINE1` | `(0x00)` | LCD1行目のDDRAM先頭アドレス |
| `LCD_DDRAM_LINE2` | `(0x40)` | LCD2行目のDDRAM先頭アドレス |
| `LCD_CMD_SET_LINE1` | `(LCD_SET_DDRAM_BASE + LCD_DDRAM_LINE1)` | LCD1行目の先頭へカーソルを移動する命令 |
| `LCD_CMD_SET_LINE2` | `(LCD_SET_DDRAM_BASE + LCD_DDRAM_LINE2)` | LCD2行目の先頭へカーソルを移動する命令 |
| `LCD_LINE1` | `(1)` | LCD1行目を示す値 |
| `LCD_LINE2` | `(2)` | LCD2行目を示す値 |
| `LCD_LINE_LENGTH` | `(8)` | AE-AQM0802の1行あたりの表示文字数 |
| `LCD_LINE1_MESSAGE` | `("LUO")` | LCD1行目に表示する文字列 |
| `LCD_LINE2_MESSAGE` | `("KAIQUN")` | LCD2行目に表示する文字列 |
| `MSEC_TO_SEC` | `(1000.0)` | ミリ秒から秒へ変換する値 |
| `LCD_COMMAND_WAIT_MSEC` | `(1.0)` | LCD命令送信後の待ち時間[ms] |
| `LCD_COMMAND_WAIT_SEC` | `(LCD_COMMAND_WAIT_MSEC / MSEC_TO_SEC)` | LCD命令送信後の待ち時間[s] |
| `LCD_INIT_WAIT_MSEC` | `(200.0)` | LCD初期化途中の待ち時間[ms] |
| `LCD_INIT_WAIT_SEC` | `(LCD_INIT_WAIT_MSEC / MSEC_TO_SEC)` | LCD初期化途中の待ち時間[s] |
| `LCD_DISPLAY_WAIT_MSEC` | `(5000.0)` | 文字表示後の待ち時間[ms] |
| `LCD_DISPLAY_WAIT_SEC` | `(LCD_DISPLAY_WAIT_MSEC / MSEC_TO_SEC)` | 文字表示後の待ち時間[s] |

---

## 3. 関数説明表

### main

| 項目 | 内容 |
|---|---|
| 関数名 | `main` |
| 引数 | `void` |
| 戻り値 | `int` |
| 機能概要 | pigpio daemonへ接続し、LCDを初期化した後、1行目に苗字、2行目に名前を表示する。5秒後にLCD表示をクリアし、I2C通信とpigpio接続を終了する。 |

### s4_InitLcd

| 項目 | 内容 |
|---|---|
| 関数名 | `s4_InitLcd` |
| 引数 | `S4 s4_pi`：pigpio接続番号 |
| 引数 | `S4 *ps4_handle`：I2Cハンドルを格納するポインタ |
| 戻り値 | `S4` |
| 機能概要 | AE-AQM0802をI2Cでオープンし、LCD初期化命令を順番に送信する。正常時は0、異常時は1を返す。 |

### s4_SendCommand

| 項目 | 内容 |
|---|---|
| 関数名 | `s4_SendCommand` |
| 引数 | `S4 s4_pi`：pigpio接続番号 |
| 引数 | `S4 s4_handle`：I2Cハンドル |
| 引数 | `U1 u1_command`：LCDへ送信する命令 |
| 戻り値 | `S4` |
| 機能概要 | LCDへ命令を1バイト送信する。正常時は0、異常時は1を返す。 |

### s4_SendData

| 項目 | 内容 |
|---|---|
| 関数名 | `s4_SendData` |
| 引数 | `S4 s4_pi`：pigpio接続番号 |
| 引数 | `S4 s4_handle`：I2Cハンドル |
| 引数 | `U1 u1_data`：LCDへ送信する表示データ |
| 戻り値 | `S4` |
| 機能概要 | LCDへ表示データを1バイト送信する。正常時は0、異常時は1を返す。 |

### s4_DisplayString

| 項目 | 内容 |
|---|---|
| 関数名 | `s4_DisplayString` |
| 引数 | `S4 s4_pi`：pigpio接続番号 |
| 引数 | `S4 s4_handle`：I2Cハンドル |
| 引数 | `U1 u1_line`：表示する行番号 |
| 引数 | `const char *pch_message`：表示する文字列 |
| 戻り値 | `S4` |
| 機能概要 | 指定されたLCD行の先頭へカーソルを移動し、指定された文字列を表示する。正常時は0、異常時は1を返す。 |

### s4_ClearLcd

| 項目 | 内容 |
|---|---|
| 関数名 | `s4_ClearLcd` |
| 引数 | `S4 s4_pi`：pigpio接続番号 |
| 引数 | `S4 s4_handle`：I2Cハンドル |
| 戻り値 | `S4` |
| 機能概要 | LCD表示をクリアする。正常時は0、異常時は1を返す。 |

---

## 4. フローチャート

### 4.1 main関数

```text
開始
↓
変数宣言
・pigpio接続番号を格納するため、S4型のs4_piを宣言する。
・I2Cハンドルを格納するため、S4型のs4_lcdHandleを宣言する。
・関数の戻り値を格納するため、S4型のs4_retを宣言する。
↓
s4_lcdHandleに-1を設定する。
↓
NULL、NULLを引数としてpigpio_start関数を呼び出し、戻り値をs4_piに格納する。
↓
引数チェック
・第1引数NULLはローカルホストへ接続する指定であることを確認する。
・第2引数NULLは標準ポートを使用する指定であることを確認する。
↓
s4_piが0未満であるか判定する。
├─ Yes
│  ↓
│  異常終了として1を返す。
└─ No
   ↓
   s4_pi、s4_lcdHandleのアドレスを引数としてs4_InitLcd関数を呼び出し、戻り値をs4_retに格納する。
   ↓
   引数チェック
   ・s4_piが0以上であることを確認する。
   ・s4_lcdHandleのアドレスがNULLではないことを確認する。
   ↓
   s4_retが0以外であるか判定する。
   ├─ Yes
   │  ↓
   │  s4_lcdHandleが0以上であるか判定する。
   │  ├─ Yes
   │  │  ↓
   │  │  s4_pi、s4_lcdHandleを引数としてi2c_close関数を呼び出す。
   │  │  戻り値を使用しないため、VD型へキャストする。
   │  └─ No
   │     ↓
   │     何もしない。
   │  ↓
   │  s4_piを引数としてpigpio_stop関数を呼び出す。
   │  戻り値を使用しないため、VD型へキャストする。
   │  ↓
   │  異常終了として1を返す。
   └─ No
      ↓
      s4_pi、s4_lcdHandle、LCD_LINE1、LCD_LINE1_MESSAGEを引数としてs4_DisplayString関数を呼び出し、戻り値をs4_retに格納する。
      ↓
      s4_retが0以外であるか判定する。
      ├─ Yes
      │  ↓
      │  s4_pi、s4_lcdHandleを引数としてi2c_close関数を呼び出す。
      │  戻り値を使用しないため、VD型へキャストする。
      │  ↓
      │  s4_piを引数としてpigpio_stop関数を呼び出す。
      │  戻り値を使用しないため、VD型へキャストする。
      │  ↓
      │  異常終了として1を返す。
      └─ No
         ↓
         s4_pi、s4_lcdHandle、LCD_LINE2、LCD_LINE2_MESSAGEを引数としてs4_DisplayString関数を呼び出し、戻り値をs4_retに格納する。
         ↓
         s4_retが0以外であるか判定する。
         ├─ Yes
         │  ↓
         │  s4_pi、s4_lcdHandleを引数としてi2c_close関数を呼び出す。
         │  戻り値を使用しないため、VD型へキャストする。
         │  ↓
         │  s4_piを引数としてpigpio_stop関数を呼び出す。
         │  戻り値を使用しないため、VD型へキャストする。
         │  ↓
         │  異常終了として1を返す。
         └─ No
            ↓
            LCD_DISPLAY_WAIT_SECを引数としてtime_sleep関数を呼び出す。
            戻り値を使用しないため、VD型へキャストする。
            ↓
            s4_pi、s4_lcdHandleを引数としてs4_ClearLcd関数を呼び出し、戻り値をs4_retに格納する。
            ↓
            s4_pi、s4_lcdHandleを引数としてi2c_close関数を呼び出す。
            戻り値を使用しないため、VD型へキャストする。
            ↓
            s4_piを引数としてpigpio_stop関数を呼び出す。
            戻り値を使用しないため、VD型へキャストする。
            ↓
            s4_retが0以外であるか判定する。
            ├─ Yes
            │  ↓
            │  異常終了として1を返す。
            └─ No
               ↓
               正常終了として0を返す。
```

### 4.2 s4_InitLcd関数

```text
開始
↓
変数宣言
・I2Cオープン結果を格納するため、S4型のs4_handleを宣言する。
・関数の戻り値を格納するため、S4型のs4_retを宣言する。
↓
ps4_handleがNULLであるか判定する。
├─ Yes
│  ↓
│  異常終了として1を返す。
└─ No
   ↓
   s4_pi、LCD_I2C_BUS、LCD_I2C_ADDR、LCD_I2C_FLAGSを引数としてi2c_open関数を呼び出し、戻り値をs4_handleに格納する。
   ↓
   引数チェック
   ・s4_piが0以上であることを確認する。
   ・LCD_I2C_BUSが使用するI2Cバス番号であることを確認する。
   ・LCD_I2C_ADDRがAE-AQM0802のI2Cアドレスであることを確認する。
   ・LCD_I2C_FLAGSが0であることを確認する。
   ↓
   s4_handleが0未満であるか判定する。
   ├─ Yes
   │  ↓
   │  異常終了として1を返す。
   └─ No
      ↓
      s4_handleをps4_handleの指す先へ格納する。
      ↓
      s4_pi、s4_handle、LCD_CMD_FUNCTION_SET_NORMALを引数としてs4_SendCommand関数を呼び出し、戻り値をs4_retに格納する。
      ↓
      s4_retが0以外であるか判定する。
      ├─ Yes
      │  ↓
      │  異常終了として1を返す。
      └─ No
         ↓
         s4_pi、s4_handle、LCD_CMD_FUNCTION_SET_EXTENDを引数としてs4_SendCommand関数を呼び出し、戻り値をs4_retに格納する。
         ↓
         以降も各LCD初期化命令をs4_SendCommand関数で順番に送信し、戻り値を確認する。
         ↓
         LCD_INIT_WAIT_SECを引数としてtime_sleep関数を呼び出す。
         戻り値を使用しないため、VD型へキャストする。
         ↓
         s4_pi、s4_handle、LCD_CMD_FUNCTION_SET_NORMALを引数としてs4_SendCommand関数を呼び出す。
         ↓
         s4_pi、s4_handle、LCD_CMD_DISPLAY_ONを引数としてs4_SendCommand関数を呼び出す。
         ↓
         s4_pi、s4_handle、LCD_CMD_CLEAR_DISPLAYを引数としてs4_SendCommand関数を呼び出す。
         ↓
         すべて正常終了したか判定する。
         ├─ Yes
         │  ↓
         │  正常終了として0を返す。
         └─ No
            ↓
            異常終了として1を返す。
```

### 4.3 s4_SendCommand関数

```text
開始
↓
変数宣言
・I2C書き込み結果を格納するため、S4型のs4_retを宣言する。
↓
s4_pi、s4_handle、LCD_CONTROL_COMMAND、u1_commandを引数としてi2c_write_byte_data関数を呼び出し、戻り値をs4_retに格納する。
↓
引数チェック
・s4_piが0以上であることを確認する。
・s4_handleが0以上であることを確認する。
・LCD_CONTROL_COMMANDがLCD命令送信用の制御バイトであることを確認する。
・u1_commandが送信対象のLCD命令であることを確認する。
↓
LCD_COMMAND_WAIT_SECを引数としてtime_sleep関数を呼び出す。
戻り値を使用しないため、VD型へキャストする。
↓
s4_retが0未満であるか判定する。
├─ Yes
│  ↓
│  異常終了として1を返す。
└─ No
   ↓
   正常終了として0を返す。
```

### 4.4 s4_SendData関数

```text
開始
↓
変数宣言
・I2C書き込み結果を格納するため、S4型のs4_retを宣言する。
↓
s4_pi、s4_handle、LCD_CONTROL_DATA、u1_dataを引数としてi2c_write_byte_data関数を呼び出し、戻り値をs4_retに格納する。
↓
引数チェック
・s4_piが0以上であることを確認する。
・s4_handleが0以上であることを確認する。
・LCD_CONTROL_DATAがLCD表示データ送信用の制御バイトであることを確認する。
・u1_dataが送信対象の表示データであることを確認する。
↓
LCD_COMMAND_WAIT_SECを引数としてtime_sleep関数を呼び出す。
戻り値を使用しないため、VD型へキャストする。
↓
s4_retが0未満であるか判定する。
├─ Yes
│  ↓
│  異常終了として1を返す。
└─ No
   ↓
   正常終了として0を返す。
```

### 4.5 s4_DisplayString関数

```text
開始
↓
変数宣言
・ループ回数をカウントするため、U4型のu4_loopCntを宣言する。
・文字列長を格納するため、U4型のu4_lengthを宣言する。
・LCD行設定命令を格納するため、U1型のu1_commandを宣言する。
・関数の戻り値を格納するため、S4型のs4_retを宣言する。
↓
pch_messageがNULLであるか判定する。
├─ Yes
│  ↓
│  異常終了として1を返す。
└─ No
   ↓
   u1_lineがLCD_LINE1であるか判定する。
   ├─ Yes
   │  ↓
   │  u1_commandにLCD_CMD_SET_LINE1を設定する。
   └─ No
      ↓
      u1_lineがLCD_LINE2であるか判定する。
      ├─ Yes
      │  ↓
      │  u1_commandにLCD_CMD_SET_LINE2を設定する。
      └─ No
         ↓
         異常終了として1を返す。
↓
s4_pi、s4_handle、u1_commandを引数としてs4_SendCommand関数を呼び出し、戻り値をs4_retに格納する。
↓
s4_retが0以外であるか判定する。
├─ Yes
│  ↓
│  異常終了として1を返す。
└─ No
   ↓
   pch_messageを引数としてstrlen関数を呼び出し、文字列長をu4_lengthに格納する。
   ↓
   u4_lengthがLCD_LINE_LENGTHより大きいか判定する。
   ├─ Yes
   │  ↓
   │  u4_lengthにLCD_LINE_LENGTHを設定する。
   └─ No
      ↓
      何もしない。
   ↓
   u4_loopCntに0を設定する。
   ↓
   u4_loopCntがu4_lengthより小さい間、以下を繰り返す。
   ・s4_pi、s4_handle、pch_message[u4_loopCnt]を引数としてs4_SendData関数を呼び出す。
   ・戻り値をs4_retに格納する。
   ・s4_retが0以外の場合、異常終了として1を返す。
   ・u4_loopCntを1加算する。
   ↓
   正常終了として0を返す。
```

### 4.6 s4_ClearLcd関数

```text
開始
↓
変数宣言
・関数の戻り値を格納するため、S4型のs4_retを宣言する。
↓
s4_pi、s4_handle、LCD_CMD_CLEAR_DISPLAYを引数としてs4_SendCommand関数を呼び出し、戻り値をs4_retに格納する。
↓
引数チェック
・s4_piが0以上であることを確認する。
・s4_handleが0以上であることを確認する。
・LCD_CMD_CLEAR_DISPLAYがLCDクリア命令であることを確認する。
↓
s4_retが0以外であるか判定する。
├─ Yes
│  ↓
│  異常終了として1を返す。
└─ No
   ↓
   正常終了として0を返す。
```
