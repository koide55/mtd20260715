# MTD 演習 / Moving Target Defense Hands-on (2026-07-15)

Moving Target Defense (MTD, ムービング・ターゲット・ディフェンス) の講義スライドと
ハンズオン演習一式。2025-07-27 版のスライド (`MTD-prosecit-20250727.pdf`) を
Markdown化し、演習2（システムコールMTD）を **e9patch v1.0.1**（2026年6月リリース）
向けに全面刷新したもの。

*Lecture slides and hands-on exercises for Moving Target Defense. The 2025-07-27 slide deck
was converted to Markdown, and Exercise 2 (system-call MTD) was fully reworked for
**e9patch v1.0.1** (June 2026).*

## 構成 / Layout

```
mtd20260715/
├── README.md                      ← このファイル / this file
├── Dockerfile                     ← 演習用イメージ（amd64, e9patch同梱）
├── docker-compose.yml             ← Docker Compose 定義
├── slides/
│   └── MTD-slides.md              ← 全スライド（日英併記）/ full deck (JP/EN)
├── exercise1-network-mtd/         ← 演習1: ネットワークレベルMTD（URL/ポート）
│   ├── README.md
│   ├── mtdnet.py                  ← MTD機能付きWebアプリ / MTD web app
│   └── demo.sh
├── exercise2-syscall-mtd/         ← 演習2: システムコールレベルMTD（プロセス内）
│   ├── README.md
│   ├── setup.sh                   ← ローカル用: e9patch v1.0.1 をビルド（Docker不要）
│   ├── syscall_mtd.c              ← e9patchフック（syscallゲート監視）/ in-process MTD monitor
│   ├── victim.c                   ← 自前インラインsyscallでシェル起動パスを持つ被害プログラム
│   ├── Makefile
│   └── run_demo.sh
└── exercise3-combined/            ← 演習3: 多層防御（演習1×演習2）
    ├── README.md
    └── scenario.sh                ← ネットワークMTD × syscall MTD の統合シナリオ
```

## はじめかた（Docker Compose 推奨）/ Getting started (Docker Compose, recommended)

受講生は同じ環境で演習できるよう **Docker Compose** で実行する。イメージビルド時に
e9patch v1.0.1 がビルド済みになるので、起動後すぐ演習できる。

*Run everything with **Docker Compose** so all students share one environment.
e9patch v1.0.1 is built into the image, so the lab is ready right after startup.*

```console
# リポジトリのルートで / at the repository root
$ docker compose up -d --build          # イメージ構築（e9patchビルド含む）＆起動
$ docker compose exec lab ./exercise2-syscall-mtd/run_demo.sh   # 演習2デモ
$ docker compose exec lab bash          # 対話シェル / interactive shell
$ docker compose down                    # 停止・削除 / stop & remove
```

演習1（ネットワークMTD）をコンテナ内で起動すると、ホストの `localhost:8123` 等から
アクセスできる（ポートは compose で公開済み）：

```console
$ docker compose exec lab ./exercise2-syscall-mtd/run_demo.sh   # 演習2デモ
$ docker compose exec lab ./exercise3-combined/scenario.sh      # 演習3（多層防御）

# 演習1をコンテナ内で起動するとホストの localhost:8123 等からアクセス可能
$ docker compose exec lab bash -lc "cd exercise1-network-mtd && ./demo.sh"
```

### Apple Silicon (M1/M2/M3…) について / On Apple Silicon

e9patch は x86_64 専用だが、本イメージは `platform: linux/amd64` 指定のため
**Docker Desktop の Rosetta 2 エミュレーション**で動作する。Docker Desktop の
設定 →「Use Rosetta for x86/amd64 emulation on Apple Silicon」を有効にすること。
演習2はptraceを使わない**プロセス内方式**なので、特別な権限（`SYS_PTRACE` 等）は不要。

*e9patch is x86_64-only, but the image is `linux/amd64` and runs under Docker Desktop's
Rosetta 2. Enable "Use Rosetta for x86/amd64 emulation" in Docker Desktop settings.
Exercise 2 is fully in-process (no ptrace), so no special capabilities are required.*

## ローカル（Dockerを使わない）/ Running locally without Docker

**スライド / Slides:** `slides/MTD-slides.md` を任意のMarkdownビューアで開く。
（VS Code, `grip`, `pandoc`, Marp などで表示・PDF化可能。）

**演習1 / Exercise 1**（Python3のみ / Python3 only）:

```console
$ cd exercise1-network-mtd && ./demo.sh
```

**演習2 / Exercise 2**（x86_64 Linux + git/gcc/make が必要）:

```console
$ cd exercise2-syscall-mtd
$ ./setup.sh && make && ./run_demo.sh
```

**演習3 / Exercise 3**（演習1・2を組み合わせた多層防御シナリオ）:

```console
$ cd exercise3-combined && ./scenario.sh      # 事前に演習2の ./setup.sh を実施
```

## 主な更新点 / Key updates in this revision

- スライド全35ページをMarkdown化（日英併記を維持）。
- 演習2を **e9patch v1.0.1** の新しい `-M`/`-P` 構文と同梱 `stdlib.c` API に更新。
- 演習2からAWS EC2・カーネルリコンフィグ前提を排除。さらに **ptrace 非依存の
  プロセス内方式**に再設計し、**Apple Silicon の Docker (Rosetta 2) でも動作**するように。
- **演習3（多層防御）を新設** — ネットワークMTD × システムコールMTD の統合シナリオ。
- **Docker Compose** で受講生が同一環境を再現できるように（`Dockerfile` に e9patch 同梱）。
- 演習1のWebアプリ (`mtdnet.py`) をPython 3.12+対応・レース排除版に更新。

## 検証 / Verification

- 演習1 (`mtdnet.py`, `demo.sh`) はサンドボックスで**実行・動作確認済み**。
- 演習2の **e9patch v1.0.1 ビルド・計装は Docker (amd64/Rosetta) 上で成功を確認済み**。
  ptrace 方式が Rosetta で不可であることを実測（`orig_rax=0`）した上で、プロセス内方式へ移行。
  コードは e9patch v1.0.1 の API に照合済み。デモの最終実行は受講生環境で確認のこと。
  詳細は `exercise2-syscall-mtd/README.md` の「検証状況」を参照。

## ライセンス / License

演習コードはMIT。`exercise2-syscall-mtd/setup.sh` が取得する e9patch 本体は
GPLv3（<https://github.com/GJDuck/e9patch>）。
