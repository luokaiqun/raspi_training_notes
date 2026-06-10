# Raspberry Pi C言語研修ノート

このリポジトリは、Raspberry Piを使用したC言語研修用の個人学習ノートです。

## 構成

```text
raspi_training_notes/
├── README.md
├── .gitignore
└── task10_lcd/
    ├── lcd_task10.c
    ├── task10_lcd_design.md
    ├── task10_lcd_explanation.md
    └── compile_run.md
```

## 内容

- `lcd_task10.c`
  - AE-AQM0802に苗字と名前を表示し、5秒後にLCD表示をクリアするCプログラムです。
- `task10_lcd_design.md`
  - 外部情報、関数説明表、フローチャート、引数チェックをまとめた設計資料です。
- `task10_lcd_explanation.md`
  - I2Cアドレス、制御バイト、LCD初期化命令、DDRAMアドレスなどの説明資料です。
- `compile_run.md`
  - Raspberry Pi上でのコンパイル、実行、Gitからの取得方法です。

## 注意

会社資料、研修PDF原文、内部情報、スクリーンショットなどは入れないでください。
ここには自分で整理した学習メモと自作コードだけを置く想定です。
