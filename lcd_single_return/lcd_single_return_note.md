# LCD課題 単一return版 コード説明

## 1. 方針

本コードでは、自作関数の戻り値を以下のマクロで統一する。

```c
#define FUNC_RET_NORMAL    (0)
#define FUNC_RET_ABNORMAL  (1)
```

`FUNC_RET_NORMAL`は自作関数の正常終了、`FUNC_RET_ABNORMAL`は自作関数の異常終了を示す。

main関数についてはC言語の通常の戻り値として、`0`を正常終了、`1`を異常終了として扱う。

---

## 2. 自作関数とpigpioライブラリ関数の戻り値判定

自作関数の戻り値判定には、`FUNC_RET_NORMAL`および`FUNC_RET_ABNORMAL`を使用する。

例：

```c
s4_callRet = s4_DisplayLcd(...);
if (s4_callRet != FUNC_RET_NORMAL) {
    s4_funcRet = FUNC_RET_ABNORMAL;
} else {
    /* DO NOTHING */
}
```

一方、pigpioライブラリ関数の戻り値判定は各関数仕様に従うため、`0`や`0未満`などで判定する。

例：

```c
s4_pi = pigpio_start(NULL, NULL);
if (s4_pi < 0) {
    s4_funcRet = FUNC_RET_ABNORMAL;
} else {
    /* DO NOTHING */
}
```

```c
s4_callRet = i2c_write_byte_data(...);
if (s4_callRet != 0) {
    s4_funcRet = FUNC_RET_ABNORMAL;
} else {
    /* DO NOTHING */
}
```

このように、自作関数の戻り値規則とpigpioライブラリ関数の戻り値仕様を分けて扱うことで、判定理由を明確にする。

---

## 3. s4_piを-1で初期化する理由

`s4_pi`は`pigpio_start`の返却値を格納する変数である。  
`pigpio_start`が失敗した場合、戻り値は0未満となる。

本コードでは関数末尾でまとめて終了処理を行うため、`pigpio_start`失敗時に`pigpio_stop`を呼び出さないよう、初期値として無効値の`-1`を設定する。

```c
s4_pi = -1;
```

終了処理では以下のように判定する。

```c
if (s4_pi >= 0) {
    pigpio_stop(s4_pi);
} else {
    /* DO NOTHING */
}
```

これにより、pigpio接続が成功している場合のみ`pigpio_stop`を呼び出す。

---

## 4. s4_lcdHandleを-1で初期化する理由

`s4_lcdHandle`は`i2c_open`の返却値を格納する変数である。  
`i2c_open`が失敗した場合、戻り値は0未満となる。

本コードでは関数末尾でまとめて終了処理を行うため、`i2c_open`失敗時に`i2c_close`を呼び出さないよう、初期値として無効値の`-1`を設定する。

```c
s4_lcdHandle = -1;
```

終了処理では以下のように判定する。

```c
if (s4_lcdHandle >= 0) {
    (VD)i2c_close(s4_pi, s4_lcdHandle);
} else {
    /* DO NOTHING */
}
```

これにより、I2Cオープンが成功している場合のみ`i2c_close`を呼び出す。

---

## 5. s4_funcRetとs4_callRetの使い分け

| 変数名 | 意味 |
|---|---|
| `s4_funcRet` | 本関数の最終戻り値 |
| `s4_callRet` | 呼び出し関数またはpigpioライブラリ関数の戻り値 |

`return`は関数末尾の1箇所に統一し、途中で異常が発生した場合は`s4_funcRet`に`FUNC_RET_ABNORMAL`を設定する。

---

## 6. 判断条件の書き方

### 異常条件のチェック

異常条件を判定し、異常の場合は`s4_funcRet`へ`FUNC_RET_ABNORMAL`を設定する。

```c
if (pu1_letterArray == NULL) {
    s4_funcRet = FUNC_RET_ABNORMAL;
} else {
    /* DO NOTHING */
}
```

### 後続処理の実行判定

前段の処理が正常な場合のみ、次の処理を実行する。

```c
if (s4_funcRet == FUNC_RET_NORMAL) {
    次の処理;
} else {
    /* DO NOTHING */
}
```

### 自作関数の呼び出し結果判定

```c
if (s4_callRet != FUNC_RET_NORMAL) {
    s4_funcRet = FUNC_RET_ABNORMAL;
} else {
    /* DO NOTHING */
}
```

### pigpioライブラリ関数の呼び出し結果判定

pigpioライブラリ関数については、各関数仕様に従い判定する。

```c
if (s4_callRet != 0) {
    s4_funcRet = FUNC_RET_ABNORMAL;
} else {
    /* DO NOTHING */
}
```

---

## 7. VDキャストを付ける箇所

`i2c_close`は戻り値を持つが、本コードでは終了処理として呼び出すため、戻り値によるエラー判定は行わない。  
そのため、戻り値未使用を明示する目的で`(VD)`キャストを付ける。

```c
(VD)i2c_close(s4_pi, s4_lcdHandle);    /* 終了処理のため、戻り値エラーチェック不要。詳細は設計書参照。 */
```

`time_sleep`および`pigpio_stop`は戻り値がvoidであるため、`(VD)`キャストは不要である。

---

## 8. ループ内の異常処理

LCD初期化命令や表示データ送信では、途中で異常が発生した場合、後続の送信を行わない。

```c
for (u4_loopCnt = 0U;
     (u4_loopCnt < LCD_INIT_CMD_NUM) && (s4_funcRet == FUNC_RET_NORMAL);
     u4_loopCnt++) {
    ...
}
```

これにより、ループ中に`s4_funcRet`が`FUNC_RET_ABNORMAL`になった場合、次回のループ条件を満たさず、繰り返しを終了する。
