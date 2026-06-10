# 課題10 LCD表示 説明資料

> 本资料用于帮助理解设计意图：解释性文字用中文，但代码、宏、命令名等保持与日语设计资料一致。

## 1. 全体の考え方（整体思路）

LCD不能把字符串直接发出去就显示。
对 LCD 必须发送两类内容：「命令(command)」和「表示数据(data)」。

```text
命令(command)：
・初期化する
・表示をONにする
・表示をクリアする
・カーソル位置(DDRAMアドレス)を設定する

表示データ(data)：
・'L' 'U' 'O'
・'K' 'A' 'I' 'Q' 'U' 'N'
```

因此代码中把发送分成两个函数：

```c
S4 s4_SendCommand(S4 s4_pi, S4 s4_handle, U1 u1_command);
S4 s4_SendData(S4 s4_pi, S4 s4_handle, U1 u1_data);
```

两者的区别只是第 1 字节的 control byte 不同（命令用 `0x00`，数据用 `0x40`），第 2 字节才是真正的命令值或字符码。

---

## 2. なぜ`i2c_write_byte_data`で送るか（为什么用 i2c_write_byte_data）

AQM0802 的 I2C 协议本身就是：

```text
control byte + command/data byte
```

也就是说，每次写入都是「先 1 个 control byte，再 1 个命令/数据字节」。
`i2c_write_byte_data` 正好是「reg(1字节) + data(1字节)」的写入形式，
把 control byte 放到 reg 参数，把命令/数据放到 data 参数，正好对应这个协议：

```c
/* 命令送信 */
s4_ret = i2c_write_byte_data(s4_pi, s4_handle, LCD_CONTROL_COMMAND, u1_command);

/* 表示データ送信 */
s4_ret = i2c_write_byte_data(s4_pi, s4_handle, LCD_CONTROL_DATA, u1_data);
```

- 第 3 引数(reg)：control byte（命令用 `0x00`，数据用 `0x40`）。
- 第 4 引数(data)：实际的命令值或字符码。

这样不需要自己准备 2 字节缓冲区，`i2c_write_byte_data` 内部会按
「control byte → command/data」的顺序发送 2 个字节。

---

## 3. なぜI2Cアドレスは0x3Eか（地址 0x3E 与 0x7C 的关系）

コードでは以下のように定義している。

```c
#define LCD_I2C_ADDR (0x3E)
```

これはAE-AQM0802のI2C **7bitアドレス** である。

数据手册或资料里有时把 AQM0802 的从机地址写成 `0x7C`。
那是 **8bit 表记**，等于 7bit 地址左移 1 位：

```text
0x3E << 1 = 0x7C
```

pigpio 的 `i2c_open` 需要传入 **7bit 地址**，所以代码用 `0x3E`：

```c
s4_handle = i2c_open(s4_pi, LCD_I2C_BUS, LCD_I2C_ADDR, LCD_I2C_FLAGS);
```

这样就能与挂在 Raspberry Pi I2C バス1 上、地址为 0x3E 的 LCD 通信。

---

## 4. なぜLCD_CONTROL_COMMANDは0x00か（命令用 control byte）

```c
#define LCD_CONTROL_COMMAND (0x00)
```

向 AQM0802 写数据时，第 1 字节是 control byte，用来告诉 LCD「接下来这 1 字节是命令还是数据」。
`0x00` 表示后面的 1 字节是 **命令**。

```c
/* reg=0x00(次は命令)、data=実際のLCD命令 */
s4_ret = i2c_write_byte_data(s4_pi, s4_handle, LCD_CONTROL_COMMAND, u1_command);
```

> 注意：`0x00` 不是普通寄存器地址，而是 LCD 协议规定的 control byte。

---

## 5. なぜLCD_CONTROL_DATAは0x40か（数据用 control byte）

```c
#define LCD_CONTROL_DATA (0x40)
```

`0x40` 表示后面的 1 字节是 **表示数据**。

```c
/* reg=0x40(次は表示データ)、data=文字コード */
s4_ret = i2c_write_byte_data(s4_pi, s4_handle, LCD_CONTROL_DATA, u1_data);
```

例如 `u1_data` 为 `'L'` 时：

```text
reg  = 0x40  → 次は表示データ
data = 'L'   → 文字コード
```

> 同样，`0x40` 也不是普通寄存器地址，而是 control byte。

---

## 6. なぜLCD初期化命令が多いか（为什么初始化命令很多）

课题要求只是「显示苗字和名字，5 秒后清除」，但 LCD 上电后不一定立即处于可显示状态。
所以必须按数据手册发送一串初始化命令，让 LCD 进入可显示状态。

> 重要：这些初始化命令不是题目直接给出的，而是依据 AE-AQM0802 / ST7032 数据手册
> 或常见初始化流程补充的。

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

| 命令 | 値 | 意味 |
|---|---|---|
| `LCD_CMD_FUNCTION_SET_NORMAL` | `0x38` | 通常命令モード(8bit,2行,IS=0)へ設定する。 |
| `LCD_CMD_FUNCTION_SET_EXTEND` | `0x39` | 拡張命令モード(IS=1)へ設定する。内部発振・コントラスト・フォロワ設定に使う。 |
| `LCD_CMD_INTERNAL_OSC` | `0x14` | 内部発振周波数を設定する。 |
| `LCD_CMD_CONTRAST_LOW` | `0x70` | コントラスト下位ビットを設定する。表示が薄い場合はここを調整する。 |
| `LCD_CMD_POWER_ICON_CONTRAST` | `0x56` | 電源・アイコン・コントラスト上位ビットを設定する。 |
| `LCD_CMD_FOLLOWER_CONTROL` | `0x6C` | フォロワ回路(昇圧)を設定する。送信後は安定待ちが必要。 |
| `LCD_CMD_DISPLAY_ON` | `0x0C` | 表示ON(カーソルOFF・点滅OFF)。 |
| `LCD_CMD_CLEAR_DISPLAY` | `0x01` | 表示クリア。 |

フォロワ制御の後は昇圧回路が安定するまで待つ。

```c
(VD)time_sleep(LCD_INIT_WAIT_SEC);
```

---

## 7. なぜDDRAMアドレスを指定するか（为什么要指定 DDRAM 地址）

LCD 把要显示的字符保存在内部的 **DDRAM**（显示用内存）里。
写字符前，必须先指定从哪个位置开始写。

AE-AQM0802 是 8 文字 2 行的 LCD，一般使用以下地址：

| 行 | DDRAM先頭アドレス |
|---|---|
| 1行目 | `0x00` |
| 2行目 | `0x40` |

设置 DDRAM 地址的命令是「`0x80` + DDRAM 地址」，所以宏用表达式定义：

```c
#define LCD_SET_DDRAM_BASE  (0x80)
#define LCD_DDRAM_LINE1     (0x00)
#define LCD_DDRAM_LINE2     (0x40)
#define LCD_CMD_SET_LINE1   (LCD_SET_DDRAM_BASE + LCD_DDRAM_LINE1)
#define LCD_CMD_SET_LINE2   (LCD_SET_DDRAM_BASE + LCD_DDRAM_LINE2)
```

计算结果：

```text
LCD_CMD_SET_LINE1 = 0x80 + 0x00 = 0x80
LCD_CMD_SET_LINE2 = 0x80 + 0x40 = 0xC0
```

即：要写 1 行目就发 `0x80`，要写 2 行目就发 `0xC0`（作为命令发送）。

---

## 8. 具体的にLUOを表示するとき（以显示 LUO 为例）

```c
s4_ret = s4_DisplayString(s4_pi, s4_lcdHandle, LCD_LINE1, LCD_LINE1_MESSAGE);
```

展开后等价于：

```c
s4_DisplayString(s4_pi, s4_lcdHandle, 1, "LUO");
```

函数内部因为 `LCD_LINE1`，先发送行设置命令把光标移到 1 行目先頭：

```c
u1_command = LCD_CMD_SET_LINE1;   /* 0x80 */
s4_ret = s4_SendCommand(s4_pi, s4_handle, u1_command);
```

然后把 `"LUO"` 逐字符作为表示数据发送：

```text
'L' → s4_SendData
'U' → s4_SendData
'O' → s4_SendData
```

每个字符都用 `LCD_CONTROL_DATA`(0x40) 作为 control byte，所以 LCD 把它当成表示数据，而不是命令。

---

## 9. まとめ（整体流程）

```text
pigpio_startでpigpio daemonへ接続する。
↓
i2c_openでLCDとのI2C通信を開始する。
↓
LCD初期化命令を順番に送信し、LCDを表示可能な状態にする。
↓
DDRAMアドレス(0x80)を指定し、1行目の先頭へ移動する。
↓
苗字"LUO"を表示データとして1文字ずつ送信する。
↓
DDRAMアドレス(0xC0)を指定し、2行目の先頭へ移動する。
↓
名前"KAIQUN"を表示データとして1文字ずつ送信する。
↓
5秒待つ(LCD_DISPLAY_WAIT_SEC)。
↓
LCDクリア命令(0x01)を送信する。
↓
i2c_closeとpigpio_stopで後処理を行う。
```
