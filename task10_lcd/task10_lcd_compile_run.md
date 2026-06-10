# 課題10 コンパイル・実行・Git取得手順

## 1. Raspberry Pi側の準備

pigpiodを起動する。

```bash
sudo systemctl start pigpiod
```

状態確認をする。

```bash
sudo systemctl status pigpiod
```

I2Cデバイスが見えているか確認する。

```bash
i2cdetect -y 1
```

`3e`が表示されていれば、AQM0802が認識されている可能性が高い。

---

## 2. コンパイル

`task10_lcd.c`があるディレクトリで以下を実行する。

```bash
gcc -Wall -std=c90 -pedantic-errors -pthread -o task10_lcd task10_lcd.c -lpigpiod_if2 -lrt
```

---

## 3. 実行

```bash
./task10_lcd
```

LCDの1行目に`LUO`、2行目に`KAIQUN`が表示され、5秒後に表示が消える。

---

## 4. GitHubからRaspberry Piへ取得する場合

初回だけ以下を実行する。

```bash
git clone git@github.com:GitHubユーザー名/raspi_training_notes.git
```

取得したディレクトリへ移動する。

```bash
cd raspi_training_notes/task10_lcd
```

2回目以降、更新を取得する場合は以下を実行する。

```bash
cd ~/raspi_training_notes
git pull
```

---

## 5. type.hについて

このコードは`type.h`が同じディレクトリ、またはコンパイラが参照できる場所にある前提である。

```c
#include "type.h"
```

`type.h`が別ディレクトリにある場合は、コンパイル時に`-I`オプションで指定する。

例：

```bash
gcc -Wall -std=c90 -pedantic-errors -pthread -I../include -o task10_lcd task10_lcd.c -lpigpiod_if2 -lrt
```
