/*============================================================*/
/* kadai10.c                                                  */
/*  課題 10: AE-AQM0802(+PCA9515) に苗字と名前を表示し、       */
/*           5 秒後に表示をクリアするプログラム               */
/*  方式  : pigpiod を使用する方法                            */
/*============================================================*/
#include <pigpiod_if2.h>   /* pigpiod I/F 用ヘッダ            */
#include "type.h"          /* 型定義の共有ヘッダ              */

/*---- I2C 接続設定 ------------------------------------------*/
#define I2C_BUS         (1)        /* I2C 1(一般用途,SDA=GPIO2,SCL=GPIO3) */
#define I2C_ADDR_LCD    (0x3E)     /* AE-AQM0802 のスレーブアドレス       */
#define I2C_FLAGS       (0)        /* I2C フラグ。通常 0                  */

/*---- 制御バイト(i2c_write_byte_data の reg に指定) ---------*/
/*  reg はスレーブアドレス直後に送出される 1 バイト目であり、 */
/*  AQM0802(ST7032)では制御バイト(Co/RS)として扱われる        */
#define CTRL_CMD        (0x00)     /* コマンド書き込み(Co=0,RS=0)         */
#define CTRL_DATA       (0x40)     /* DDRAM データ書き込み(Co=0,RS=1)     */

/*---- LCD コマンド(AQM0802 データシート 初期化設定例) -------*/
#define LCD_CMD_FUNC_NORMAL (0x38) /* Function set:8bit,2行,IS=0          */
#define LCD_CMD_FUNC_EXTEND (0x39) /* Function set:8bit,2行,IS=1(拡張)    */
#define LCD_CMD_OSC_FREQ    (0x14) /* 内部 OSC 周波数調整                 */
#define LCD_CMD_CONTRAST_L  (0x70) /* コントラスト下位 C3-C0 ※要調整     */
#define LCD_CMD_POWER_ICON  (0x56) /* Power/ICON/Contrast上位(3.3V)※要調整*/
#define LCD_CMD_FOLLOWER    (0x6C) /* Follower control(昇圧ON)            */
#define LCD_CMD_DISPLAY_ON  (0x0C) /* Display ON(D=1,C=0,B=0)             */
#define LCD_CMD_CLEAR       (0x01) /* Clear Display                       */

/*---- DDRAM アドレス ----------------------------------------*/
#define DDRAM_SET_CMD   (0x80)     /* DDRAM アドレスセットコマンド上位bit */
#define DDRAM_LINE1     (0x00)     /* 1 行目先頭アドレス                  */
#define DDRAM_LINE2     (0x40)     /* 2 行目先頭アドレス                  */

/*---- 時間設定(time_sleep の引数は秒単位) -------------------*/
/*  ミリ秒 → 秒 の単位換算を明示する                          */
#define MS_PER_SEC       (1000)                              /* 1 秒=1000ms */
#define DISPLAY_TIME_MS  (5000)                              /* 表示保持[ms]*/
#define DISPLAY_TIME_SEC (DISPLAY_TIME_MS / (double)MS_PER_SEC)
#define POWER_WAIT_MS    (200)                               /* 昇圧安定[ms]*/
#define POWER_WAIT_SEC   (POWER_WAIT_MS  / (double)MS_PER_SEC)
#define CLEAR_WAIT_MS    (2)                                 /* Clear後[ms] */
#define CLEAR_WAIT_SEC   (CLEAR_WAIT_MS  / (double)MS_PER_SEC)

/*---- 表示文字列(半角英数字、各行 8 文字以内に置き換える) ---*/
#define STR_SURNAME     ("YAMADA")  /* 苗字(例)                           */
#define STR_NAME        ("TARO")    /* 名前(例)                           */

/*---- 関数プロトタイプ宣言 ----------------------------------*/
int main(void);
s4  s4_WriteLcd(int s4_pi, int s4_handle, u1 u1_ctrl, u1 u1_val);
vd  vd_InitLcd(int s4_pi, int s4_handle);
vd  vd_PutString(int s4_pi, int s4_handle, u1 u1_ddramAddr, const char *pc_str);
vd  vd_ClearLcd(int s4_pi, int s4_handle);

/*============================================================*/
/* main                                                       */
/*  pigpiod 接続 → I2C オープン → LCD 初期化 → 苗字・名前表示 */
/*  → 5 秒待機 → クリア → クローズ の順に処理する             */
/*  自作関数へ渡す引数の妥当性確認は main で行う(引数チェック表参照)*/
/*============================================================*/
int main(void)
{
    int s4_pi;      /* pigpio_start の返却値(pigpiod 接続) */
    int s4_handle;  /* i2c_open の返却値(I2C ハンドル)     */

    /* pigpiod デーモンとの接続を開始する */
    if ((s4_pi = pigpio_start(NULL, NULL)) < 0) {
        /* 接続失敗の場合には負数が返る */
        return 1;
    } else {
        /* do nothing */
    }

    /* AE-AQM0802 を I2C で開きハンドルを取得する */
    if ((s4_handle = i2c_open(s4_pi, I2C_BUS, I2C_ADDR_LCD, I2C_FLAGS)) < 0) {
        /* オープン失敗の場合には負数が返る */
        pigpio_stop(s4_pi);   /* pigpiod との接続を終了する */
        return 1;
    } else {
        /* do nothing */
    }

    vd_InitLcd(s4_pi, s4_handle);                             /* LCD を初期化する        */
    vd_PutString(s4_pi, s4_handle, DDRAM_LINE1, STR_SURNAME); /* 1 行目に苗字を表示する  */
    vd_PutString(s4_pi, s4_handle, DDRAM_LINE2, STR_NAME);    /* 2 行目に名前を表示する  */
    time_sleep(DISPLAY_TIME_SEC);                            /* 表示保持(5 秒)待機する  */
    vd_ClearLcd(s4_pi, s4_handle);                          /* LCD の表示をクリアする  */

    /* I2C ハンドルをクローズする */
    (void)i2c_close(s4_pi, (unsigned)s4_handle);
    /* 戻り値のエラーチェック不要。理由は設計を参照 */

    pigpio_stop(s4_pi);   /* pigpiod との接続を終了する */

    return 0;
}

/*------------------------------------------------------------*/
/* s4_WriteLcd                                                */
/*  制御バイト u1_ctrl と値 u1_val を AQM0802 へ I2C 送信する */
/*  引数は呼び出し元で確認済みのため本関数では確認しない      */
/*------------------------------------------------------------*/
s4 s4_WriteLcd(int s4_pi, int s4_handle, u1 u1_ctrl, u1 u1_val)
{
    s4 s4_ret;   /* i2c_write_byte_data の戻り値 */

    /* reg=u1_ctrl(制御バイト)、val=u1_val として 1 バイト送信する */
    s4_ret = i2c_write_byte_data(s4_pi, (unsigned)s4_handle, u1_ctrl, u1_val);

    return s4_ret;
}

/*------------------------------------------------------------*/
/* vd_InitLcd                                                 */
/*  AQM0802 データシートの初期化設定例にしたがい LCD を初期化 */
/*  本関数内の s4_WriteLcd の戻り値はすべてエラーチェック不要 */
/*  (理由は設計を参照)。引数は呼び出し元で確認済み            */
/*------------------------------------------------------------*/
vd vd_InitLcd(int s4_pi, int s4_handle)
{
    (void)s4_WriteLcd(s4_pi, s4_handle, CTRL_CMD, LCD_CMD_FUNC_NORMAL); /* 機能設定(IS=0)   */
    (void)s4_WriteLcd(s4_pi, s4_handle, CTRL_CMD, LCD_CMD_FUNC_EXTEND); /* 機能設定(IS=1)   */
    (void)s4_WriteLcd(s4_pi, s4_handle, CTRL_CMD, LCD_CMD_OSC_FREQ);    /* 内部OSC周波数    */
    (void)s4_WriteLcd(s4_pi, s4_handle, CTRL_CMD, LCD_CMD_CONTRAST_L);  /* コントラスト下位 */
    (void)s4_WriteLcd(s4_pi, s4_handle, CTRL_CMD, LCD_CMD_POWER_ICON);  /* 昇圧+上位        */
    (void)s4_WriteLcd(s4_pi, s4_handle, CTRL_CMD, LCD_CMD_FOLLOWER);    /* ボルテージ追従   */
    time_sleep(POWER_WAIT_SEC);                                         /* 昇圧回路安定待ち */
    (void)s4_WriteLcd(s4_pi, s4_handle, CTRL_CMD, LCD_CMD_FUNC_NORMAL); /* 通常命令へ復帰   */
    (void)s4_WriteLcd(s4_pi, s4_handle, CTRL_CMD, LCD_CMD_DISPLAY_ON);  /* 表示ON           */
    (void)s4_WriteLcd(s4_pi, s4_handle, CTRL_CMD, LCD_CMD_CLEAR);       /* 表示クリア       */
    time_sleep(CLEAR_WAIT_SEC);                                         /* Clear完了待ち    */
}

/*------------------------------------------------------------*/
/* vd_PutString                                               */
/*  u1_ddramAddr で指定した位置から文字列 pc_str を表示する   */
/*  s4_WriteLcd の戻り値はエラーチェック不要(理由は設計を参照)*/
/*  引数は呼び出し元で確認済み。文字列は 8 文字以内を保証     */
/*------------------------------------------------------------*/
vd vd_PutString(int s4_pi, int s4_handle, u1 u1_ddramAddr, const char *pc_str)
{
    s4 s4_idx;   /* 文字列走査用インデックス */

    /* DDRAM アドレスセットコマンドを送信し書き込み位置を指定する */
    (void)s4_WriteLcd(s4_pi, s4_handle, CTRL_CMD,
                      (u1)(DDRAM_SET_CMD | u1_ddramAddr));

    /* 文字列終端まで 1 文字ずつ DDRAM へ書き込み LCD に表示する */
    for (s4_idx = 0; pc_str[s4_idx] != '\0'; s4_idx++) {
        (void)s4_WriteLcd(s4_pi, s4_handle, CTRL_DATA, (u1)pc_str[s4_idx]);
    }
}

/*------------------------------------------------------------*/
/* vd_ClearLcd                                                */
/*  Clear Display コマンドを送信し全表示を消去する            */
/*  s4_WriteLcd の戻り値はエラーチェック不要(理由は設計を参照)*/
/*  引数は呼び出し元で確認済み                                */
/*------------------------------------------------------------*/
vd vd_ClearLcd(int s4_pi, int s4_handle)
{
    /* Clear Display コマンド(0x01)を送信し全表示を消去する */
    (void)s4_WriteLcd(s4_pi, s4_handle, CTRL_CMD, LCD_CMD_CLEAR);

    /* Clear Display は実行に時間がかかるため待機する */
    time_sleep(CLEAR_WAIT_SEC);
}
