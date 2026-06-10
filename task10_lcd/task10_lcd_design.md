# 課題10 LCD表示 設計資料

> 说明：本设计资料中的正式说明文字、外部情報、関数説明表、フローチャート、引数チェック表均使用日语，符合研修提交格式。

## 1. 課題内容

AE-AQM0802をI2Cで制御し、LCDの1行目に自分の苗字、2行目に自分の名前を表示する。
表示開始から5秒後にLCD表示をクリアし、プログラムを終了する。

- 制御方式 : pigpiod(`pigpiod_if2`)を使用する方法
- ピン番号 : I2C1(SDA=GPIO2, SCL=GPIO3)
- 共有ヘッダ : `type.h`(`VD` / `U1` / `U4` / `S4` 等の型を提供)
- LCD送信方式 : control byte(reg)に続けてcommand/dataを`i2c_write_byte_data`で送信する

---

## 2. 外部情報

| 名称 | 値 | 意味 |
|---|---|---|
| `LCD_I2C_BUS` | `(1)` | Raspberry Piで使用するI2Cバス番号 |
| `LCD_I2C_ADDR` | `(0x3E)` | AE-AQM0802のI2C 7bitアドレス |
| `LCD_I2C_FLAGS` | `(0)` | I2C通信時のフラグ(通常0) |
| `LCD_CONTROL_COMMAND` | `(0x00)` | LCDへ命令を書き込むためのcontrol byte |
| `LCD_CONTROL_DATA` | `(0x40)` | LCDへ表示データを書き込むためのcontrol byte |
| `LCD_CMD_FUNCTION_SET_NORMAL` | `(0x38)` | 通常命令モードへ設定する命令(8bit,2行,IS=0) |
| `LCD_CMD_FUNCTION_SET_EXTEND` | `(0x39)` | 拡張命令モードへ設定する命令(8bit,2行,IS=1) |
| `LCD_CMD_INTERNAL_OSC` | `(0x14)` | 内部発振周波数を設定する命令 |
| `LCD_CMD_CONTRAST_LOW` | `(0x70)` | コントラスト下位ビットを設定する命令 |
| `LCD_CMD_POWER_ICON_CONTRAST` | `(0x56)` | 電源・アイコン・コントラスト上位ビットを設定する命令 |
| `LCD_CMD_FOLLOWER_CONTROL` | `(0x6C)` | フォロワ回路(昇圧)を設定する命令 |
| `LCD_CMD_DISPLAY_ON` | `(0x0C)` | LCD表示をONにする命令 |
| `LCD_CMD_CLEAR_DISPLAY` | `(0x01)` | LCD表示をクリアする命令 |
| `LCD_SET_DDRAM_BASE` | `(0x80)` | DDRAMアドレス設定命令の基準値 |
| `LCD_DDRAM_LINE1` | `(0x00)` | LCD1行目のDDRAM先頭アドレス |
| `LCD_DDRAM_LINE2` | `(0x40)` | LCD2行目のDDRAM先頭アドレス |
| `LCD_CMD_SET_LINE1` | `(LCD_SET_DDRAM_BASE + LCD_DDRAM_LINE1)` | LCD1行目先頭へカーソルを移動する命令(0x80) |
| `LCD_CMD_SET_LINE2` | `(LCD_SET_DDRAM_BASE + LCD_DDRAM_LINE2)` | LCD2行目先頭へカーソルを移動する命令(0xC0) |
| `LCD_LINE1` | `(1)` | LCD1行目を示す行番号 |
| `LCD_LINE2` | `(2)` | LCD2行目を示す行番号 |
| `LCD_LINE_LENGTH` | `(8)` | AE-AQM0802の1行あたりの表示文字数 |
| `LCD_LINE1_MESSAGE` | `("LUO")` | LCD1行目に表示する文字列(苗字) |
| `LCD_LINE2_MESSAGE` | `("KAIQUN")` | LCD2行目に表示する文字列(名前) |
| `MSEC_TO_SEC` | `(1000.0)` | ミリ秒から秒へ変換する係数 |
| `LCD_COMMAND_WAIT_MSEC` | `(1.0)` | 1バイト送信後の待ち時間[ms] |
| `LCD_COMMAND_WAIT_SEC` | `(LCD_COMMAND_WAIT_MSEC / MSEC_TO_SEC)` | 1バイト送信後の待ち時間[s] |
| `LCD_INIT_WAIT_MSEC` | `(200.0)` | 初期化時の昇圧安定待ち[ms] |
| `LCD_INIT_WAIT_SEC` | `(LCD_INIT_WAIT_MSEC / MSEC_TO_SEC)` | 初期化時の昇圧安定待ち[s] |
| `LCD_CLEAR_WAIT_MSEC` | `(2.0)` | Clear Display後の追加待ち時間[ms] |
| `LCD_CLEAR_WAIT_SEC` | `(LCD_CLEAR_WAIT_MSEC / MSEC_TO_SEC)` | Clear Display後の追加待ち時間[s] |
| `LCD_DISPLAY_WAIT_MSEC` | `(5000.0)` | 文字表示後の保持時間[ms] |
| `LCD_DISPLAY_WAIT_SEC` | `(LCD_DISPLAY_WAIT_MSEC / MSEC_TO_SEC)` | 文字表示後の保持時間[s] |

> 関連する値はベース値からの式で定義し、計算で導出されることが分かるようにしている。
> 例：`LCD_CMD_SET_LINE2` は `0x80 + 0x40 = 0xC0`、`LCD_DISPLAY_WAIT_SEC` は `5000.0 / 1000.0 = 5.0`。

---

## 3. 関数説明表

### main

| 項目 | 内容 |
|---|---|
| 関数名 | `main` |
| 引数 | なし |
| 戻り値 | `int`：0 正常終了 / 1 異常終了 |
| 機能概要 | pigpio daemonへ接続し、LCDを初期化した後、1行目に苗字、2行目に名前を表示する。5秒後にLCD表示をクリアし、I2C通信とpigpio接続を終了する。 |

### s4_InitLcd

| 項目 | 内容 |
|---|---|
| 関数名 | `s4_InitLcd` |
| 引数 | `S4 s4_pi`：pigpio接続番号 |
| 引数 | `S4 *ps4_handle`：I2Cハンドルを格納するポインタ |
| 戻り値 | `S4`：0 正常終了 / 1 異常終了 |
| 機能概要 | AE-AQM0802をI2Cでオープンし、LCD初期化命令を順番に送信する。 |

### s4_SendCommand

| 項目 | 内容 |
|---|---|
| 関数名 | `s4_SendCommand` |
| 引数 | `S4 s4_pi`：pigpio接続番号 |
| 引数 | `S4 s4_handle`：I2Cハンドル |
| 引数 | `U1 u1_command`：LCDへ送信する命令 |
| 戻り値 | `S4`：0 正常終了 / 1 異常終了 |
| 機能概要 | control byte(reg)に続けて命令を`i2c_write_byte_data`でLCDへ送信する。 |

### s4_SendData

| 項目 | 内容 |
|---|---|
| 関数名 | `s4_SendData` |
| 引数 | `S4 s4_pi`：pigpio接続番号 |
| 引数 | `S4 s4_handle`：I2Cハンドル |
| 引数 | `U1 u1_data`：LCDへ送信する表示データ |
| 戻り値 | `S4`：0 正常終了 / 1 異常終了 |
| 機能概要 | control byte(reg)に続けて表示データを`i2c_write_byte_data`でLCDへ送信する。 |

### s4_DisplayString

| 項目 | 内容 |
|---|---|
| 関数名 | `s4_DisplayString` |
| 引数 | `S4 s4_pi`：pigpio接続番号 |
| 引数 | `S4 s4_handle`：I2Cハンドル |
| 引数 | `U1 u1_line`：表示する行番号 |
| 引数 | `const char *pch_message`：表示する文字列 |
| 戻り値 | `S4`：0 正常終了 / 1 異常終了 |
| 機能概要 | 指定されたLCD行の先頭へカーソルを移動し、指定された文字列を1文字ずつ表示する。 |

### s4_ClearLcd

| 項目 | 内容 |
|---|---|
| 関数名 | `s4_ClearLcd` |
| 引数 | `S4 s4_pi`：pigpio接続番号 |
| 引数 | `S4 s4_handle`：I2Cハンドル |
| 戻り値 | `S4`：0 正常終了 / 1 異常終了 |
| 機能概要 | LCD表示をクリアする。 |

---

## 4. フローチャート

> フローチャート内で外部/API関数を呼び出す箇所は、引数の詳細を「## 5. 引数チェック表」に整理している。
> フローチャートでは「詳細は引数チェック表を参照する」と記載し、流れを見やすくしている。

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
詳細は引数チェック表を参照する。
↓
s4_piが0未満であるか判定する。
├─ Yes
│  ↓
│  異常終了として1を返す。
└─ No
   ↓
   s4_pi、s4_lcdHandleのアドレスを引数としてs4_InitLcd関数を呼び出し、戻り値をs4_retに格納する。
   ↓
   s4_retが0以外であるか判定する。
   ├─ Yes
   │  ↓
   │  s4_lcdHandleが0以上であるか判定する。
   │  ├─ Yes
   │  │  ↓
   │  │  s4_pi、s4_lcdHandleを引数としてi2c_close関数を呼び出す。
   │  │  戻り値を使用しないため、VD型へキャストする。詳細は引数チェック表を参照する。
   │  └─ No
   │     ↓
   │     何もしない。
   │  ↓
   │  s4_piを引数としてpigpio_stop関数を呼び出す。
   │  戻り値を使用しないため、VD型へキャストする。詳細は引数チェック表を参照する。
   │  ↓
   │  異常終了として1を返す。
   └─ No
      ↓
      s4_pi、s4_lcdHandle、LCD_LINE1、LCD_LINE1_MESSAGEを引数としてs4_DisplayString関数を呼び出し、戻り値をs4_retに格納する。
      ↓
      s4_retが0以外であるか判定する。
      ├─ Yes
      │  ↓
      │  s4_pi、s4_lcdHandleを引数としてi2c_close関数を呼び出す。戻り値を使用しないため、VD型へキャストする。
      │  ↓
      │  s4_piを引数としてpigpio_stop関数を呼び出す。戻り値を使用しないため、VD型へキャストする。
      │  ↓
      │  異常終了として1を返す。
      └─ No
         ↓
         s4_pi、s4_lcdHandle、LCD_LINE2、LCD_LINE2_MESSAGEを引数としてs4_DisplayString関数を呼び出し、戻り値をs4_retに格納する。
         ↓
         s4_retが0以外であるか判定する。
         ├─ Yes
         │  ↓
         │  s4_pi、s4_lcdHandleを引数としてi2c_close関数を呼び出す。戻り値を使用しないため、VD型へキャストする。
         │  ↓
         │  s4_piを引数としてpigpio_stop関数を呼び出す。戻り値を使用しないため、VD型へキャストする。
         │  ↓
         │  異常終了として1を返す。
         └─ No
            ↓
            LCD_DISPLAY_WAIT_SECを引数としてtime_sleep関数を呼び出す。
            戻り値を使用しないため、VD型へキャストする。詳細は引数チェック表を参照する。
            ↓
            s4_pi、s4_lcdHandleを引数としてs4_ClearLcd関数を呼び出し、戻り値をs4_retに格納する。
            ↓
            s4_pi、s4_lcdHandleを引数としてi2c_close関数を呼び出す。戻り値を使用しないため、VD型へキャストする。
            ↓
            s4_piを引数としてpigpio_stop関数を呼び出す。戻り値を使用しないため、VD型へキャストする。
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
   詳細は引数チェック表を参照する。
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
      s4_retが0以外の場合、異常終了として1を返す。
      ↓
      同様に、LCD_CMD_FUNCTION_SET_EXTEND、LCD_CMD_INTERNAL_OSC、LCD_CMD_CONTRAST_LOW、
      LCD_CMD_POWER_ICON_CONTRAST、LCD_CMD_FOLLOWER_CONTROLを、順番にs4_SendCommand関数で送信する。
      各送信後にs4_retが0以外の場合、異常終了として1を返す。
      ↓
      LCD_INIT_WAIT_SECを引数としてtime_sleep関数を呼び出す。
      戻り値を使用しないため、VD型へキャストする。詳細は引数チェック表を参照する。
      ↓
      LCD_CMD_FUNCTION_SET_NORMAL、LCD_CMD_DISPLAY_ON、LCD_CMD_CLEAR_DISPLAYを、
      順番にs4_SendCommand関数で送信する。各送信後にs4_retが0以外の場合、異常終了として1を返す。
      ↓
      正常終了として0を返す。
```

### 4.3 s4_SendCommand関数

```text
開始
↓
変数宣言
・I2C書き込み結果を格納するため、S4型のs4_retを宣言する。
↓
s4_pi、s4_handle、LCD_CONTROL_COMMAND、u1_commandを引数として
i2c_write_byte_data関数を呼び出し、戻り値をs4_retに格納する。
詳細は引数チェック表を参照する。
↓
LCD_COMMAND_WAIT_SECを引数としてtime_sleep関数を呼び出す。
戻り値を使用しないため、VD型へキャストする。詳細は引数チェック表を参照する。
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
s4_pi、s4_handle、LCD_CONTROL_DATA、u1_dataを引数として
i2c_write_byte_data関数を呼び出し、戻り値をs4_retに格納する。
詳細は引数チェック表を参照する。
↓
LCD_COMMAND_WAIT_SECを引数としてtime_sleep関数を呼び出す。
戻り値を使用しないため、VD型へキャストする。詳細は引数チェック表を参照する。
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
・LCD行設定命令を格納するため、U1型のu1_commandを宣言する。
・文字列長を格納するため、U4型のu4_lengthを宣言する。
・ループ回数をカウントするため、U4型のu4_loopCntを宣言する。
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
   ・s4_pi、s4_handle、pch_message[u4_loopCnt]をU1型へキャストした値を引数としてs4_SendData関数を呼び出す。
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
s4_retが0以外であるか判定する。
├─ Yes
│  ↓
│  異常終了として1を返す。
└─ No
   ↓
   LCD_CLEAR_WAIT_SECを引数としてtime_sleep関数を呼び出す。
   戻り値を使用しないため、VD型へキャストする。詳細は引数チェック表を参照する。
   ↓
   正常終了として0を返す。
```

---

## 5. 引数チェック表

本課題で呼び出す外部/API関数の引数チェックと戻り値処理を整理する。

### pigpio_start

| 注記 | 関数 | 呼び出し | チェック |
|---|---|---|---|
| 引数チェック | `pigpio_start` | `pigpio_start(NULL, NULL)` | 第1引数`NULL`はローカルホストへ接続する指定であることを確認する。第2引数`NULL`は標準ポートを使用する指定であることを確認する。 |
| 戻り値確認 | `pigpio_start` | `s4_pi = pigpio_start(NULL, NULL)` | 戻り値`s4_pi`が0以上であることを確認する。0未満の場合は異常終了とする。 |

### i2c_open

| 注記 | 関数 | 呼び出し | チェック |
|---|---|---|---|
| 引数チェック | `i2c_open` | `i2c_open(s4_pi, LCD_I2C_BUS, LCD_I2C_ADDR, LCD_I2C_FLAGS)` | `s4_pi`が0以上であることを確認する。`LCD_I2C_BUS`が使用するI2Cバス番号であることを確認する。`LCD_I2C_ADDR`がAE-AQM0802のI2Cアドレスであることを確認する。`LCD_I2C_FLAGS`が0であることを確認する。 |
| 戻り値確認 | `i2c_open` | `s4_handle = i2c_open(s4_pi, LCD_I2C_BUS, LCD_I2C_ADDR, LCD_I2C_FLAGS)` | 戻り値`s4_handle`が0以上であることを確認する。0未満の場合は異常終了とする。 |

### i2c_write_byte_data

| 注記 | 関数 | 呼び出し | チェック |
|---|---|---|---|
| 引数チェック | `i2c_write_byte_data` | `i2c_write_byte_data(s4_pi, s4_handle, LCD_CONTROL_COMMAND, u1_command)` | `s4_pi`が0以上であることを確認する。`s4_handle`が0以上であることを確認する。第3引数(reg)にcontrol byte(`LCD_CONTROL_COMMAND`または`LCD_CONTROL_DATA`)を指定していることを確認する。第4引数に命令または表示データを指定していることを確認する。 |
| 戻り値確認 | `i2c_write_byte_data` | `s4_ret = i2c_write_byte_data(s4_pi, s4_handle, LCD_CONTROL_DATA, u1_data)` | 戻り値`s4_ret`が0未満でないことを確認する。0未満の場合は異常終了とする。 |

### time_sleep

| 注記 | 関数 | 呼び出し | チェック |
|---|---|---|---|
| 引数チェック | `time_sleep` | `time_sleep(LCD_DISPLAY_WAIT_SEC)` | 引数が待ち時間として妥当な秒数であることを確認する。(LCD_COMMAND_WAIT_SEC / LCD_INIT_WAIT_SEC / LCD_CLEAR_WAIT_SEC / LCD_DISPLAY_WAIT_SEC) |
| 戻り値未使用 | `time_sleep` | `(VD)time_sleep(LCD_DISPLAY_WAIT_SEC)` | 戻り値を使用しないため、VD型へキャストする。 |

### i2c_close

| 注記 | 関数 | 呼び出し | チェック |
|---|---|---|---|
| 引数チェック | `i2c_close` | `i2c_close(s4_pi, s4_handle)` | `s4_pi`が0以上であることを確認する。`s4_handle`が0以上であることを確認する。 |
| 戻り値未使用 | `i2c_close` | `(VD)i2c_close(s4_pi, s4_handle)` | 戻り値を使用しないため、VD型へキャストする。 |

### pigpio_stop

| 注記 | 関数 | 呼び出し | チェック |
|---|---|---|---|
| 引数チェック | `pigpio_stop` | `pigpio_stop(s4_pi)` | `s4_pi`が0以上であることを確認する。 |
| 戻り値未使用 | `pigpio_stop` | `(VD)pigpio_stop(s4_pi)` | 戻り値を使用しないため、VD型へキャストする。 |

### strlen

| 注記 | 関数 | 呼び出し | チェック |
|---|---|---|---|
| 引数チェック | `strlen` | `strlen(pch_message)` | `pch_message`がNULLでないことを呼び出し前に確認する。 |
| 戻り値確認 | `strlen` | `u4_length = (U4)strlen(pch_message)` | 戻り値をU4型へキャストして文字列長として使用する。LCD_LINE_LENGTHを超える場合は切り詰める。 |
