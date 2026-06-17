# LCD課題 コンパイル・実行方法

## 1. pigpiod起動

```bash
sudo systemctl start pigpiod
```

## 2. I2Cデバイス確認

```bash
i2cdetect -y 1
```

AE-AQM0802が接続されている場合、`3e`が表示される。

## 3. コンパイル

```bash
gcc -Wall -std=c90 -pedantic-errors -pthread -o lcd_single_return lcd_single_return.c -lpigpiod_if2 -lrt
```

## 4. 実行

```bash
./lcd_single_return
```
