# Raspberry Pi C言語研修ノート

このリポジトリは、Raspberry Piを使用したC言語研修用の個人学習ノートです。

## 構成

```text
raspi_training_notes/
├── README.md
├── .gitignore
└── task10_lcd/
    ├── task10_lcd.c
    ├── task10_lcd_design.md
    ├── task10_lcd_explanation.md
    └── task10_lcd_compile_run.md
```

## 内容

- `task10_lcd.c`
  - AE-AQM0802に苗字と名前を表示し、5秒後にLCD表示をクリアするCプログラムです。
- `task10_lcd_design.md`
  - 概要、ファイル構成、設計方針、外部情報（マクロ定義）一覧、関数一覧、各関数の説明・引数チェック・フローチャートをまとめた設計書です。
- `task10_lcd_explanation.md`
  - I2Cアドレス、制御バイト、LCD初期化命令、DDRAMアドレスなどの意味を補足説明した資料です。
- `task10_lcd_compile_run.md`
  - Raspberry Pi上でのコンパイル、実行、Gitからの取得手順です。

## 注意

会社資料、研修PDF原文、内部情報、スクリーンショットなどは入れないでください。
ここには自分で整理した学習メモと自作コードだけを置く想定です。
