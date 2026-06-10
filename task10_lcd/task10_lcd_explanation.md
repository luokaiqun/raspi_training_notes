# 課題10 LCD表示 説明資料

## 1. 全体の考え方

LCDは、文字列をそのまま送るだけでは表示できない。  
LCDには「命令」と「表示データ」の2種類を送る必要がある。

```text
命令：
・初期化する
・表示をONにする
・表示をクリアする
・カーソル位置を設定する

表示データ：
・'L'
・'U'
・'O'
・'K'
・'A'
・'I'
・'Q'
・'U'
・'N'
```

そのため、コードではLCDへの1バイト送信を最小関数`s4_WriteLcd`に集約し、
制御バイト（コマンド/データ）を引数で切り替えている。

```c
S4 s4_WriteLcd(S4 s4_pi, S4 s4_handle, U1 u1_control, U1 u1_value);
```

`u1_control`に`LCD_CONTROL_COMMAND`を渡せば命令、`LCD_CONTROL_DATA`を渡せば表示データとして送信される。
コマンド用・データ用に関数を分けず、重複を排除している。

---

## 2. なぜI2Cアドレスは0x3Eか

コードでは以下のように定義している。

```c
#define LCD_I2C_ADDR (0x3E)
```

これはAE-AQM0802のI2C 7bitアドレスである。

データシートや資料によっては、AQM0802のスレーブアドレスが`0x7C`と書かれることがある。  
これは8bit表記であり、7bitアドレスを1bit左シフトした値である。

```text
0x3E << 1 = 0x7C
```

pigpioの`i2c_open`関数には7bitアドレスを渡すため、コードでは`0x3E`を使用する。

```c
s4_handle = i2c_open(s4_pi, LCD_I2C_BUS, LCD_I2C_ADDR, LCD_I2C_FLAGS);
```

この処理により、Raspberry PiのI2Cバス1に接続された、アドレス0x3EのLCDと通信できる状態になる。

---

## 3. なぜLCD_CONTROL_COMMANDは0x00か

コードでは以下のように定義している。

```c
#define LCD_CONTROL_COMMAND (0x00)
```

AQM0802へI2Cでデータを送るときは、まず制御バイトを送る。  
この制御バイトにより、次の1バイトが「命令」なのか「表示データ」なのかをLCDへ伝える。

`0x00`は、後ろの1バイトが命令であることを示す。

```c
s4_ret = i2c_write_byte_data(s4_pi, s4_handle, LCD_CONTROL_COMMAND, u1_command);
```

このコードは、LCDへ以下のような意味のデータを送っている。

```text
LCD_CONTROL_COMMAND = 0x00
u1_command          = LCDへ送る命令
```

つまり、「次の1バイトは命令である」と指定してから、LCD命令を送っている。

---

## 4. なぜLCD_CONTROL_DATAは0x40か

コードでは以下のように定義している。

```c
#define LCD_CONTROL_DATA (0x40)
```

`0x40`は、後ろの1バイトが表示データであることを示す制御バイトである。

```c
s4_ret = i2c_write_byte_data(s4_pi, s4_handle, LCD_CONTROL_DATA, u1_data);
```

例えば`u1_data`が`'L'`の場合、LCDへ以下のような意味で送信される。

```text
LCD_CONTROL_DATA = 0x40
u1_data          = 'L'
```

つまり、「次の1バイトは表示データである」と指定してから、文字コード`'L'`を送っている。

---

## 5. なぜLCD初期化命令が多いか

課題の要求は、LCDへ苗字と名前を表示して、5秒後にクリアすることである。  
しかしLCDは、電源投入直後から必ず表示可能な状態になっているとは限らない。

そのため、LCDを文字表示できる状態にするため、データシートに従って初期化命令を送る必要がある。

コードでは以下の命令を使用している。

```c
#define LCD_CMD_FUNCTION_SET_NORMAL       (0x38)
#define LCD_CMD_FUNCTION_SET_EXTEND       (0x39)
#define LCD_CMD_INTERNAL_OSC              (0x14)
#define LCD_CMD_CONTRAST_LOW              (0x70)
#define LCD_CMD_POWER_ICON_CONTRAST       (0x56)
#define LCD_CMD_FOLLOWER_CONTROL          (0x6C)
#define LCD_CMD_DISPLAY_ON                (0x0C)
#define LCD_CMD_CLEAR_DISPLAY             (0x01)
```

### 5.1 LCD_CMD_FUNCTION_SET_NORMAL

```c
#define LCD_CMD_FUNCTION_SET_NORMAL (0x38)
```

LCDを通常命令モードに設定する。  
8bit、2行表示など、LCDの基本動作モードを設定する命令である。

### 5.2 LCD_CMD_FUNCTION_SET_EXTEND

```c
#define LCD_CMD_FUNCTION_SET_EXTEND (0x39)
```

LCDを拡張命令モードに設定する。  
内部発振、コントラスト、フォロワ回路などを設定するために使用する。

### 5.3 LCD_CMD_INTERNAL_OSC

```c
#define LCD_CMD_INTERNAL_OSC (0x14)
```

LCD内部の発振設定を行う命令である。

### 5.4 LCD_CMD_CONTRAST_LOW

```c
#define LCD_CMD_CONTRAST_LOW (0x70)
```

LCDのコントラスト下位ビットを設定する命令である。  
表示が薄い場合は、この値を変更することで改善できる場合がある。

### 5.5 LCD_CMD_POWER_ICON_CONTRAST

```c
#define LCD_CMD_POWER_ICON_CONTRAST (0x56)
```

電源制御、アイコン表示、コントラスト上位ビットを設定する命令である。

### 5.6 LCD_CMD_FOLLOWER_CONTROL

```c
#define LCD_CMD_FOLLOWER_CONTROL (0x6C)
```

LCD内部のフォロワ回路を設定する命令である。  
この命令の後、内部回路が安定するまで待つ必要がある。

```c
(VD)time_sleep(LCD_INIT_WAIT_SEC);
```

### 5.7 LCD_CMD_DISPLAY_ON

```c
#define LCD_CMD_DISPLAY_ON (0x0C)
```

LCD表示をONにする命令である。  
カーソルOFF、点滅OFFの設定も含まれる。

### 5.8 LCD_CMD_CLEAR_DISPLAY

```c
#define LCD_CMD_CLEAR_DISPLAY (0x01)
```

LCD表示をクリアする命令である。

---

## 6. なぜDDRAMアドレスを指定するか

LCDは、表示する文字を内部のDDRAMという表示用メモリに保存している。  
文字を表示する前に、どの位置から文字を書き込むかを指定する必要がある。

AE-AQM0802は8文字2行のLCDであり、一般的に以下のアドレスを使用する。

| 行 | DDRAM先頭アドレス |
|---|---|
| 1行目 | `0x00` |
| 2行目 | `0x40` |

DDRAMアドレスを設定する命令は、以下のように作る。

```text
0x80 + DDRAMアドレス
```

そのため、コードでは以下のようにマクロを定義している。

```c
#define LCD_SET_DDRAM_BASE  (0x80)
#define LCD_DDRAM_LINE1     (0x00)
#define LCD_DDRAM_LINE2     (0x40)
```

コードでは、行番号からDDRAMアドレスを求め、`0x80 + アドレス`でアドレスセットコマンドを生成する。

```c
u1_command = (U1)(LCD_SET_DDRAM_BASE + u1_ddramAddr);
```

計算結果は以下になる。

```text
1行目: 0x80 + 0x00 = 0x80
2行目: 0x80 + 0x40 = 0xC0
```

つまり、1行目に表示したい場合は`0x80`を命令として送り、2行目に表示したい場合は`0xC0`を命令として送る。

---

## 7. 具体的にLUOを表示するとき

コードでは以下のように呼び出している。

```c
s4_ret = s4_DisplayString(s4_pi, s4_lcdHandle, LCD_LINE1, LCD_LINE1_MESSAGE);
```

マクロを展開すると、意味は以下になる。

```c
s4_DisplayString(s4_pi, s4_lcdHandle, 1, "LUO");
```

関数内では、`LCD_LINE1`が指定されているため、1行目先頭のDDRAMアドレスからアドレスセットコマンドを生成し、命令として送る。

```c
u1_ddramAddr = LCD_DDRAM_LINE1;
u1_command   = (U1)(LCD_SET_DDRAM_BASE + u1_ddramAddr);
s4_ret = s4_WriteLcd(s4_pi, s4_handle, LCD_CONTROL_COMMAND, u1_command);
```

これはLCDのカーソルを1行目の先頭に移動する処理である。

その後、文字列`"LUO"`を1文字ずつ送信する。

```text
'L'
'U'
'O'
```

それぞれの文字は`LCD_CONTROL_DATA`を使用して送信されるため、LCDは命令ではなく表示データとして受け取る。

---

## 8. まとめ

このプログラムの流れは以下である。

```text
pigpio_startでpigpio daemonへ接続する。
↓
i2c_openでLCDとのI2C通信を開始する。
↓
LCD初期化命令を送信し、LCDを表示可能な状態にする。
↓
DDRAMアドレスを指定し、1行目の先頭へ移動する。
↓
苗字を表示データとして送信する。
↓
DDRAMアドレスを指定し、2行目の先頭へ移動する。
↓
名前を表示データとして送信する。
↓
5秒待つ。
↓
LCDクリア命令を送信する。
↓
i2c_closeとpigpio_stopで後処理を行う。
```
