# 課題10 LCD表示プログラム (AE-AQM0802) 設計書

## 1. 概要（ゴール）

AE-AQM0802(+PCA9515) を I2C で制御し、LCD の **1 行目に苗字 `LUO`**、**2 行目に名前 `KAIQUN`** を表示する。
表示開始から **5 秒後に表示をクリア** し、プログラムを終了する。

- 制御方式 : pigpiod を使用する方法
- ピン番号 : BCM（I2C1 : SDA=GPIO2, SCL=GPIO3）
- 共有ヘッダ : `type.h`（`VD` / `U1` / `U4` / `S4` 等の型を提供）

## 2. ファイル構成

| ファイル名 | 内容 |
|---|---|
| `task10_lcd.c` | 本課題のソースコード |
| `type.h` | 研修コーディング規約に基づく型定義の共有ヘッダ（別途提供） |
| `task10_lcd_design.md` | 本設計書 |
| `task10_lcd_explanation.md` | 各マクロ・処理の意味を説明した補足資料 |
| `task10_lcd_compile_run.md` | コンパイル・実行・Git取得手順 |

## 3. 設計方針

- LCD への 1 バイト I2C 送信は最小関数 `s4_WriteLcd` に集約し、制御バイト（コマンド/データ）を引数で切り替える。コマンド送信用・データ送信用に関数を分けず重複を排除する。
- 初期化コマンド列は配列 `u1a_initCmd` に定義し、ループで順次送信する。送信順や命令の追加・削除は配列の変更のみで対応できる。
- 行番号から DDRAM アドレスへの変換は分岐で行い、`0x80 + アドレス` でアドレスセットコマンドを生成する。
- 各関数は `S4` 型で 0:正常 / 1:異常 を返し、呼び出し元で戻り値を判定する。`main` で取得したハンドルは初期化失敗時も必ずクローズする。

## 4. 外部情報（マクロ定義）一覧

### 4.1 I2C 設定

| マクロ名 | 値 | 説明 |
|---|---|---|
| `LCD_I2C_BUS` | `1` | I2C バス番号（一般用途 I2C1） |
| `LCD_I2C_ADDR` | `0x3E` | AE-AQM0802 の I2C スレーブアドレス |
| `LCD_I2C_FLAGS` | `0` | I2C フラグ（通常 0） |

### 4.2 制御バイト（`i2c_write_byte_data` の reg に指定）

| マクロ名 | 値 | 説明 |
|---|---|---|
| `LCD_CONTROL_COMMAND` | `0x00` | コマンド書き込み（Co=0, RS=0） |
| `LCD_CONTROL_DATA` | `0x40` | DDRAM データ書き込み（Co=0, RS=1） |

### 4.3 LCD コマンド（AQM0802 データシート 初期化設定例）

| マクロ名 | 値 | 説明 |
|---|---|---|
| `LCD_CMD_FUNCTION_SET_NORMAL` | `0x38` | Function set（8bit, 2行, IS=0 通常命令） |
| `LCD_CMD_FUNCTION_SET_EXTEND` | `0x39` | Function set（8bit, 2行, IS=1 拡張命令） |
| `LCD_CMD_INTERNAL_OSC` | `0x14` | 内部 OSC 周波数調整 |
| `LCD_CMD_CONTRAST_LOW` | `0x70` | コントラスト下位（C3-C0）※要調整 |
| `LCD_CMD_POWER_ICON_CONTRAST` | `0x56` | Power/ICON/Contrast 上位（3.3V 用）※要調整 |
| `LCD_CMD_FOLLOWER_CONTROL` | `0x6C` | フォロワ制御（昇圧 ON） |
| `LCD_CMD_DISPLAY_ON` | `0x0C` | 表示 ON（D=1, C=0, B=0） |
| `LCD_CMD_CLEAR_DISPLAY` | `0x01` | 表示クリア |

### 4.4 DDRAM アドレス

| マクロ名 | 値 | 説明 |
|---|---|---|
| `LCD_SET_DDRAM_BASE` | `0x80` | DDRAM アドレスセットコマンドの基底値 |
| `LCD_DDRAM_LINE1` | `0x00` | 1 行目先頭 DDRAM アドレス |
| `LCD_DDRAM_LINE2` | `0x40` | 2 行目先頭 DDRAM アドレス |

### 4.5 行・文字数・初期化制御

| マクロ名 | 値 | 説明 |
|---|---|---|
| `LCD_LINE1` | `1` | 1 行目を示す行番号 |
| `LCD_LINE2` | `2` | 2 行目を示す行番号 |
| `LCD_LINE_LENGTH` | `8` | 1 行あたりの最大表示文字数 |
| `LCD_INIT_CMD_NUM` | `9` | 初期化コマンド配列の要素数 |
| `LCD_INIT_WAIT_INDEX` | `5` | 昇圧安定待ちを挿入するコマンド配列の添字（フォロワ制御の直後） |

### 4.6 表示メッセージ

| マクロ名 | 値 | 説明 |
|---|---|---|
| `LCD_LINE1_MESSAGE` | `"LUO"` | 1 行目表示メッセージ（苗字） |
| `LCD_LINE2_MESSAGE` | `"KAIQUN"` | 2 行目表示メッセージ（名前） |

### 4.7 時間設定（`time_sleep` の引数は秒単位）

ミリ秒 → 秒 の単位換算をマクロで明示する。

| マクロ名 | 値 / 計算式 | 説明 |
|---|---|---|
| `MSEC_TO_SEC` | `1000.0` | ミリ秒 → 秒 の換算係数 |
| `LCD_COMMAND_WAIT_MSEC` | `1.0` | 1 バイト送信後の待ち時間 [ms] |
| `LCD_COMMAND_WAIT_SEC` | `1.0 / 1000.0` | 1 バイト送信後の待ち時間 [s] |
| `LCD_CLEAR_WAIT_MSEC` | `2.0` | Clear Display 後の追加待ち時間 [ms] |
| `LCD_CLEAR_WAIT_SEC` | `2.0 / 1000.0` | Clear Display 後の追加待ち時間 [s] |
| `LCD_INIT_WAIT_MSEC` | `200.0` | 初期化時の昇圧安定待ち [ms] |
| `LCD_INIT_WAIT_SEC` | `200.0 / 1000.0` | 初期化時の昇圧安定待ち [s] |
| `LCD_DISPLAY_WAIT_MSEC` | `5000.0` | 表示保持時間 [ms] |
| `LCD_DISPLAY_WAIT_SEC` | `5000.0 / 1000.0` | 表示保持時間 [s] |

## 5. 関数一覧

| 関数名 | 戻り値型 | 機能 |
|---|---|---|
| `main` | `int` | LCD に苗字と名前を表示し 5 秒後にクリアする |
| `s4_WriteLcd` | `S4` | 制御バイトと値を 1 バイト LCD へ I2C 送信する |
| `s4_InitLcd` | `S4` | LCD を初期化する |
| `s4_DisplayString` | `S4` | 指定行に文字列を表示する |
| `s4_ClearLcd` | `S4` | LCD 表示をクリアする |

---

## 6. 各関数の詳細

### 6.1 main

#### 関数説明

| 項目 | 内容 |
|---|---|
| 関数名 | `main` |
| 機能 | LCD に苗字と名前を表示し、5 秒後に表示をクリアする |
| 引数 | なし |
| 戻り値 | `int` 0:正常終了 / 1:異常終了 |

#### 引数チェック

| 引数 | チェック内容 |
|---|---|
| （なし） | 引数なし |

#### フローチャート

```text
開始
↓
変数宣言
・pigpio接続番号を格納するため、S4型のs4_piを宣言する。
・I2Cハンドルを格納するため、S4型のs4_lcdHandleを宣言する。
・関数の戻り値を格納するため、S4型のs4_retを宣言する。
↓
s4_lcdHandleを-1で初期化する。
↓
pigpio_start(NULL, NULL)を呼び出し、戻り値をs4_piに格納する。
↓
s4_piが0未満であるか判定する。
├─ Yes → 異常終了として1を返す。
└─ No
↓
s4_pi、&s4_lcdHandleを引数としてs4_InitLcd関数を呼び出し、戻り値をs4_retに格納する。
↓
s4_retが0以外であるか判定する。
├─ Yes
│   ↓
│   s4_lcdHandleが0以上であるか判定する。
│   ├─ Yes → s4_pi、s4_lcdHandleを引数としてi2c_close関数を呼び出す。
│   └─ No  → 何もしない。
│   ↓
│   s4_piを引数としてpigpio_stop関数を呼び出す。
│   ↓
│   異常終了として1を返す。
└─ No
↓
s4_pi、s4_lcdHandle、LCD_LINE1、LCD_LINE1_MESSAGEを引数として
s4_DisplayString関数を呼び出し、戻り値をs4_retに格納する。
↓
s4_retが0以外であるか判定する。
├─ Yes → i2c_close関数、pigpio_stop関数を呼び出し、異常終了として1を返す。
└─ No
↓
s4_pi、s4_lcdHandle、LCD_LINE2、LCD_LINE2_MESSAGEを引数として
s4_DisplayString関数を呼び出し、戻り値をs4_retに格納する。
↓
s4_retが0以外であるか判定する。
├─ Yes → i2c_close関数、pigpio_stop関数を呼び出し、異常終了として1を返す。
└─ No
↓
LCD_DISPLAY_WAIT_SECを引数としてtime_sleep関数を呼び出す。
↓
s4_pi、s4_lcdHandleを引数としてs4_ClearLcd関数を呼び出し、戻り値をs4_retに格納する。
↓
s4_pi、s4_lcdHandleを引数としてi2c_close関数を呼び出す。
↓
s4_piを引数としてpigpio_stop関数を呼び出す。
↓
s4_retが0以外であるか判定する。
├─ Yes → 異常終了として1を返す。
└─ No  → 正常終了として0を返す。
```

---

### 6.2 s4_WriteLcd

#### 関数説明

| 項目 | 内容 |
|---|---|
| 関数名 | `s4_WriteLcd` |
| 機能 | 制御バイトと値を 1 バイト LCD へ I2C 送信する |
| 引数 | `S4 s4_pi` : pigpio接続番号<br>`S4 s4_handle` : I2Cハンドル<br>`U1 u1_control` : 制御バイト(コマンド/データ選択)<br>`U1 u1_value` : 送信する値 |
| 戻り値 | `S4` 0:正常終了 / 1:異常終了 |

#### 引数チェック

| 引数 | チェック内容 |
|---|---|
| `s4_pi` | 0以上であることを確認する |
| `s4_handle` | 0以上であることを確認する |
| `u1_control` | LCD_CONTROL_COMMAND または LCD_CONTROL_DATA であることを確認する |
| `u1_value` | 送信する有効な値(コマンドまたは文字コード)であることを確認する |

#### フローチャート

```text
開始
↓
変数宣言
・関数の戻り値を格納するため、S4型のs4_retを宣言する。
↓
s4_pi、s4_handle、u1_control、u1_valueを引数として
i2c_write_byte_data関数を呼び出し、戻り値をs4_retに格納する。
↓
LCD_COMMAND_WAIT_SECを引数としてtime_sleep関数を呼び出す。
↓
s4_retが0未満であるか判定する。
├─ Yes → 異常終了として1を返す。
└─ No  → 正常終了として0を返す。
```

---

### 6.3 s4_InitLcd

#### 関数説明

| 項目 | 内容 |
|---|---|
| 関数名 | `s4_InitLcd` |
| 機能 | 引数で指定された LCD を初期化する |
| 引数 | `S4 s4_pi` : pigpio接続番号<br>`S4 *ps4_handle` : I2Cハンドル格納先 |
| 戻り値 | `S4` 0:正常終了 / 1:異常終了 |

#### 引数チェック

| 引数 | チェック内容 |
|---|---|
| `s4_pi` | 0以上（呼び出し元 main で pigpio_start 成功を確認済み）であることを確認する |
| `ps4_handle` | NULL でないことを確認する（関数内で NULL 判定を実施） |

#### フローチャート

```text
開始
↓
変数宣言
・初期化コマンド配列 u1a_initCmd を定義する（要素数 LCD_INIT_CMD_NUM）。
・I2Cハンドルを格納するため、S4型のs4_handleを宣言する。
・関数の戻り値を格納するため、S4型のs4_retを宣言する。
・ループカウンタとして、U4型のu4_loopCntを宣言する。
↓
ps4_handleがNULLであるか判定する。
├─ Yes → 異常終了として1を返す。
└─ No
↓
s4_pi、LCD_I2C_BUS、LCD_I2C_ADDR、LCD_I2C_FLAGSを引数として
i2c_open関数を呼び出し、戻り値をs4_handleに格納する。
↓
s4_handleが0未満であるか判定する。
├─ Yes → 異常終了として1を返す。
└─ No
↓
s4_handleをps4_handleの指す先に格納する。
↓
u4_loopCntを0で初期化する。
↓
┌──< ループ：u4_loopCnt < LCD_INIT_CMD_NUM の間繰り返す >
│   ↓
│   s4_pi、s4_handle、LCD_CONTROL_COMMAND、u1a_initCmd[u4_loopCnt]を引数として
│   s4_WriteLcd関数を呼び出し、戻り値をs4_retに格納する。
│   ↓
│   s4_retが0以外であるか判定する。
│   ├─ Yes → 異常終了として1を返す。
│   └─ No
│   ↓
│   u4_loopCntがLCD_INIT_WAIT_INDEXと等しいか判定する。
│   ├─ Yes → LCD_INIT_WAIT_SECを引数としてtime_sleep関数を呼び出す。
│   └─ No  → 何もしない。
│   ↓
│   u4_loopCntに1を加算する。
└──< ループ終端 >
↓
正常終了として0を返す。
```

---

### 6.4 s4_DisplayString

#### 関数説明

| 項目 | 内容 |
|---|---|
| 関数名 | `s4_DisplayString` |
| 機能 | 引数で指定された LCD 行に文字列を表示する |
| 引数 | `S4 s4_pi` : pigpio接続番号<br>`S4 s4_handle` : I2Cハンドル<br>`U1 u1_line` : 表示するLCD行番号<br>`const char *pch_message` : 表示する文字列 |
| 戻り値 | `S4` 0:正常終了 / 1:異常終了 |

#### 引数チェック

| 引数 | チェック内容 |
|---|---|
| `s4_pi` | 0以上であることを確認する |
| `s4_handle` | 0以上であることを確認する |
| `u1_line` | LCD_LINE1 または LCD_LINE2 であることを確認する（関数内で判定） |
| `pch_message` | NULL でないことを確認する（関数内で NULL 判定を実施） |

#### フローチャート

```text
開始
↓
変数宣言
・DDRAMアドレスを格納するため、U1型のu1_ddramAddrを宣言する。
・アドレスセットコマンドを格納するため、U1型のu1_commandを宣言する。
・文字列長を格納するため、U4型のu4_lengthを宣言する。
・ループカウンタとして、U4型のu4_loopCntを宣言する。
・関数の戻り値を格納するため、S4型のs4_retを宣言する。
↓
pch_messageがNULLであるか判定する。
├─ Yes → 異常終了として1を返す。
└─ No
↓
u1_lineがLCD_LINE1であるか判定する。
├─ Yes → u1_ddramAddrにLCD_DDRAM_LINE1を格納する。
└─ No
    ↓
    u1_lineがLCD_LINE2であるか判定する。
    ├─ Yes → u1_ddramAddrにLCD_DDRAM_LINE2を格納する。
    └─ No  → 異常終了として1を返す。
↓
u1_commandに(LCD_SET_DDRAM_BASE + u1_ddramAddr)を格納する。
↓
s4_pi、s4_handle、LCD_CONTROL_COMMAND、u1_commandを引数として
s4_WriteLcd関数を呼び出し、戻り値をs4_retに格納する。
↓
s4_retが0以外であるか判定する。
├─ Yes → 異常終了として1を返す。
└─ No
↓
strlen関数でpch_messageの長さを取得し、U4型に変換してu4_lengthに格納する。
↓
u4_lengthがLCD_LINE_LENGTHより大きいか判定する。
├─ Yes → u4_lengthにLCD_LINE_LENGTHを格納する。
└─ No  → 何もしない。
↓
u4_loopCntを0で初期化する。
↓
┌──< ループ：u4_loopCnt < u4_length の間繰り返す >
│   ↓
│   s4_pi、s4_handle、LCD_CONTROL_DATA、pch_message[u4_loopCnt]をU1型に変換した値を
│   引数としてs4_WriteLcd関数を呼び出し、戻り値をs4_retに格納する。
│   ↓
│   s4_retが0以外であるか判定する。
│   ├─ Yes → 異常終了として1を返す。
│   └─ No
│   ↓
│   u4_loopCntに1を加算する。
└──< ループ終端 >
↓
正常終了として0を返す。
```

---

### 6.5 s4_ClearLcd

#### 関数説明

| 項目 | 内容 |
|---|---|
| 関数名 | `s4_ClearLcd` |
| 機能 | LCD 表示をクリアする |
| 引数 | `S4 s4_pi` : pigpio接続番号<br>`S4 s4_handle` : I2Cハンドル |
| 戻り値 | `S4` 0:正常終了 / 1:異常終了 |

#### 引数チェック

| 引数 | チェック内容 |
|---|---|
| `s4_pi` | 0以上であることを確認する |
| `s4_handle` | 0以上であることを確認する |

#### フローチャート

```text
開始
↓
変数宣言
・関数の戻り値を格納するため、S4型のs4_retを宣言する。
↓
s4_pi、s4_handle、LCD_CONTROL_COMMAND、LCD_CMD_CLEAR_DISPLAYを引数として
s4_WriteLcd関数を呼び出し、戻り値をs4_retに格納する。
↓
s4_retが0以外であるか判定する。
├─ Yes → 異常終了として1を返す。
└─ No
↓
LCD_CLEAR_WAIT_SECを引数としてtime_sleep関数を呼び出す。
↓
正常終了として0を返す。
```

---

## 7. ビルド・実行方法

```sh
# 事前に pigpiod デーモンを起動しておく（pigpiod を使用する方法の前提）
$ sudo pigpiod

# ビルド（type.h と同一ディレクトリで実行）
$ gcc -Wall -std=c90 -pedantic-errors -pthread -o task10_lcd task10_lcd.c -lpigpiod_if2 -lrt

# 実行（pigpiod 方式のため sudo 不要）
$ ./task10_lcd
```
