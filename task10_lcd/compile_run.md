# 課題10 コンパイル・実行・確認手順

> 命令・コマンドは実機(Raspberry Pi)で使用する形のまま記載する。解説は中文。

## 1. pigpiod の起動

本课题使用 `pigpiod_if2`，必须先启动 pigpio daemon。

```bash
sudo systemctl start pigpiod
```

确认运行状态：

```bash
sudo systemctl status pigpiod
```

> 如果显示 `active (running)` 则正常。
> 如果未安装 pigpio，先安装：`sudo apt-get install pigpio`。

---

## 2. I2C デバイスの確認

确认 LCD 是否被识别（バス1）：

```bash
i2cdetect -y 1
```

如果在 `3e` 处显示地址，说明 AQM0802(0x3E) 已被识别。

> 如果 `i2cdetect` 不存在：`sudo apt-get install i2c-tools`。
> 如果 `3e` 不显示：确认 I2C 已启用(`sudo raspi-config` → Interface Options → I2C)、接线、电源。

---

## 3. コンパイル

在 `lcd_task10.c` 所在目录执行（C90 准拠）：

```bash
gcc -Wall -std=c90 -pedantic-errors -pthread -o lcd_task10 lcd_task10.c -lpigpiod_if2 -lrt
```

- `-std=c90 -pedantic-errors`：强制 C90，不允许 C99 语法。
- `-pthread` / `-lpigpiod_if2` / `-lrt`：pigpiod_if2 所需。

---

## 4. 実行

```bash
./lcd_task10
```

LCD の1行目に `LUO`、2行目に `KAIQUN` が表示され、5秒後に表示が消える。

---

## 5. type.h について

本代码以 `type.h` 与源文件位于同一目录，或编译器能引用到的位置为前提。

```c
#include "type.h"
```

如果 `type.h` 在其他目录，用 `-I` 指定包含路径：

```bash
gcc -Wall -std=c90 -pedantic-errors -pthread -I../include -o lcd_task10 lcd_task10.c -lpigpiod_if2 -lrt
```

---

## 6. GitHub から Raspberry Pi へ取得する場合

初回だけ clone する：

```bash
git clone git@github.com:GitHubユーザー名/raspi_training_notes.git
```

移动到目录：

```bash
cd raspi_training_notes/task10_lcd
```

2回目以降、更新を取得する：

```bash
cd ~/raspi_training_notes
git pull
```

---

## 7. うまくいかない場合の確認点（排查清单）

| 症状 | 確認点 |
|---|---|
| `pigpio_start` が失败(返回0未满) | `sudo systemctl status pigpiod` で daemon が起動しているか確認する。 |
| `i2cdetect` で `3e` が出ない | I2C 有効化・配線・電源・PCA9515 経路を確認する。 |
| 表示が薄い | `LCD_CMD_CONTRAST_LOW` / `LCD_CMD_POWER_ICON_CONTRAST` の値を調整する。 |
| `pigpiod_if2.h` が見つからない | pigpio がインストールされているか確認する(`sudo apt-get install pigpio`)。 |
| `git` コマンドが無い | `sudo apt-get install git` でインストールする。あるいは clone 済みフォルダを直接コピーする。 |
| リンクエラー(`undefined reference`) | `-lpigpiod_if2 -lrt` がコンパイルコマンドに含まれているか確認する。 |
