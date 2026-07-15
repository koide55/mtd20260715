# 演習2: システムコールレベルMTD（システムコール番号シャッフリング）
# Exercise 2: System-call-level MTD (System Call Number Shuffling)

**e9patch v1.0.1（2026年6月リリース）向けに全面刷新。**
旧演習が前提としていた AWS EC2 の事前構築済み環境（`~/work/mtd`, Rust製tracer, カーネル
リコンフィグ）を廃し、**任意のx86_64 Linux上で `./setup.sh` により再現できる自己完結型**
に作り直した。

*Fully reworked for **e9patch v1.0.1** (June 2026). The old AWS-EC2 / Rust-tracer / kernel-
reconfig setup is replaced by a self-contained lab that builds everything from source on any
x86_64 Linux via `./setup.sh`.*

---

## 前提 / Prerequisites

- **x86_64 の Linux**（e9patchはx86_64 ELF専用 / e9patch rewrites x86_64 ELF only）
- **推奨: Docker Compose**（リポジトリ同梱。Apple Silicon でも Rosetta 2 で動作）
- ローカル実行の場合: `git`, `gcc`, `g++`, `make`, `python3`
- （応用デモのみ / advanced demo only）`pwntools`（Dockerイメージには導入済み）

## クイックスタート（Docker 推奨）/ Quick start (Docker, recommended)

リポジトリのルートで（e9patch はイメージに構築済み、`setup.sh` 不要）：

```console
$ docker compose up -d --build
$ docker compose exec lab ./exercise2-syscall-mtd/run_demo.sh
```

> Apple Silicon では Docker Desktop の「Use Rosetta for x86/amd64 emulation」を有効に。
> ptrace 用の `SYS_PTRACE` / `seccomp:unconfined` は compose 側で付与済み。

## クイックスタート（ローカル）/ Quick start (local, without Docker)

```console
$ ./setup.sh        # e9patch v1.0.1 を取得・ビルド（初回のみ / first time only）
$ make              # tracer・デモプログラム・instrumented バイナリを作成
$ ./run_demo.sh     # デモ実行 / run the demonstration
```

期待される出力 / Expected output:

```
----------------------------------------------------------------------
1) Trusted program through the MTD tracer (should run normally):
----------------------------------------------------------------------
[mtd] MTD_KEY = 0x5f3a0000 (per-run random)
Hello, world
[mtd] 23 syscalls translated, 0 intrusions detected

----------------------------------------------------------------------
2a) Un-instrumented 'shellcode' WITHOUT the tracer (attack succeeds):
----------------------------------------------------------------------
### SHELL OBTAINED (uid=1000)

----------------------------------------------------------------------
2b) Same 'shellcode' WITH the MTD tracer (attack detected & blocked):
----------------------------------------------------------------------
[mtd] MTD_KEY = 0x2c110000 (per-run random)
[mtd] Invalid system call detected (raw execve, rax=59): INTRUSION BLOCKED
[mtd] target stopped -- shell denied
```

> 注: `MTD_KEY` と翻訳された syscall 数は実行ごと・環境ごとに変わる。
> Note: `MTD_KEY` and the translated syscall count vary per run / per environment.

---

## 構成要素 / Components

| ファイル / File | 役割 / Role |
|---|---|
| `setup.sh` | e9patch v1.0.1 をソースからビルド / build e9patch v1.0.1 from source |
| `syscall_mtd.c` | e9patchフック。各 `syscall` 命令の直前で `rax` にKEYを加算 / e9patch hook, KEY-shifts `rax` |
| `mtd_tracer.c` | ptrace監視プロセス。KEYを引いて実番号に戻し、生の危険syscallを検知 / ptrace supervisor |
| `hello.c` | 正規プログラム / benign baseline |
| `evil.c` | 生の `execve` を行う「シェルコード相当」 / raw-execve stand-in for shellcode |
| `smashme.c` + `exploit.py` | 応用: 実際のスタックオーバーフロー攻撃 / advanced: real overflow exploit |
| `Makefile`, `run_demo.sh` | ビルド・デモ / build & demo orchestration |

---

## 仕組み / How it works

```
                 per-run random KEY  (環境変数 MTD_KEY で共有 / shared via env)
                          │
  ┌────────────────┐   ptrace   ┌────────────────────────────────────────┐
  │ mtd_tracer      │◀─(監視)──▶│ target (e9patchでsyscallをフック済み)     │
  │ ・KEYを生成/配布 │           │  正規syscall:  rax += KEY  (フックが実行)   │
  │ ・entry で:      │           │  注入コード:   rax は生の実番号のまま       │
  │   rax>=KEY→翻訳  │           └────────────────────────────────────────┘
  │   rax<KEY & 危険 │
  │        →侵入遮断 │
  └────────────────┘
```

1. **フック (`syscall_mtd.c`)** … e9tool が正規バイナリの各 `syscall` 命令の直前に挿入。
   実行時に per-run のKEYを `rax`（syscall番号）へ加算し、番号を「シャッフル」する。
   フック自身は一切syscallを呼ばない（`getenv` は環境配列を読むだけ）。
2. **tracer (`mtd_tracer.c`)** … ptraceでKEYを引いて実番号に戻してからカーネルへ渡す。
   KEYでシフトされていない生の syscall（＝instrumentされていないコード）のうち、
   `execve`/`execveat` のような危険なものを **侵入として遮断**する。
   起動時のローダ等の良性な生syscallは許可するため、誤検知に強い。

**なぜ注入シェルコードだけ捕まるのか / Why only injected code is caught:**
正規バイナリの `syscall` 命令は**書き換え時に既知**なので全てフック済み（＝シフトされる）。
注入シェルコードは**実行時にスタック上に現れる**ためフックされず、番号は生のまま。
この差を tracer が検出する。

---

## e9patch コマンド（v1.0.1 構文）/ e9patch commands (v1.0.1 syntax)

`Makefile` が実行する中核コマンド：

```console
# フックをe9patch同梱の stdlib.c に対してコンパイル
$ e9patch/e9compile.sh syscall_mtd.c -I e9patch/examples

# 各 syscall 命令の直前にフックを挿入
$ e9patch/e9tool -M 'asm=/syscall/' \
      -P 'before entry(state)@syscall_mtd' hello -o hello.mtd
```

**旧版（〜2021）からの構文変更 / Syntax changes from the old slides:**

| | 旧版 / Old | 新版 / New (v1.0.1) |
|---|---|---|
| マッチ | `--match 'asm=sys(?:enter\|call)'` | `-M 'asm=/syscall/'` |
| パッチ | `--action 'call [before] func(&rax)@func'` | `-P 'before entry(state)@syscall_mtd'` |
| 引数 | `&rax`（ポインタ渡し） | `state`（全レジスタ構造体、`state->rax` を直接変更）|
| ヘッダ | 手書き `stdlib.c` | 同梱 `examples/stdlib.c`（`SYS_*`,`STATE`,`getenv` 等完備）|

---

## 応用デモ: 本物のバッファオーバーフロー / Advanced: real buffer overflow

`smashme` は `gets()` による古典的スタックオーバーフローを持つ。`exploit.py` は
`jmp rsp` ガジェット経由でスタック上のシェルコード（生の `execve`）を実行する。

```console
$ make smashme smashme.mtd
$ pip install pwntools
$ setarch $(uname -m) -R python3 exploit.py --no-mtd   # → シェル奪取 / shell
$ setarch $(uname -m) -R python3 exploit.py --mtd      # → 検知・遮断 / blocked
```

> 本物のメモリ破壊攻撃はガジェットアドレスやASLR無効化に依存し**環境依存**。
> 検出メカニズムを確実に見せるだけなら `./run_demo.sh`（`evil` 使用）で十分。
> A real exploit is environment-specific; `run_demo.sh` reliably shows the mechanism.

---

## グループ演習 / Group discussion

1. Webサービスに外部からバイナリが送り込まれ実行される未知の脆弱性を仮定する。
   システムコールレベルMTDが組み込まれていれば攻撃を検出・防御できるか。
2. 演習1（URL等）のMTDと同時に組み合わせた場合、「平均攻撃成功時間間隔」はどう変化するか。
3. 本フレームワークは syscall 番号をランダム化する。どのような効果があるか。
   （syscall命令ごと・時間ごとにKEYを変える拡張の是非も議論せよ。）
4. **旧版との相違点**: 旧演習はLinuxカーネルのリコンフィグを伴った。本版
   （e9patch v1.0.1 + ptraceによるユーザ空間実装）の相違点・利点・欠点は何か。

---

## 検証状況 / Verification status（重要 / important）

本リポジトリの作成環境は **aarch64（ARM）** であり、e9patchはx86_64専用のため、
x86_64バイナリの書き換え・実行によるエンドツーエンドの動作確認は**この環境では実施できていない**。
実施済みの検証は次のとおり：

*This repository was authored on an **aarch64 (ARM)** host. Since e9patch is x86_64-only, the
full x86_64 rewrite/run pipeline could NOT be executed here. What was verified:*

- e9patch **v1.0.1 のソースを実際に取得**し、`e9compile.sh` / `e9tool` の構文、
  同梱 `examples/stdlib.c` のAPI（`init(argc,argv,envp)`, `environ`, `getenv`, `atoll`,
  `struct STATE`{`int64_t rax`}）を照合済み。フックはこのAPIに厳密に準拠。
- E9Tool User's Guide のパッチ文法（`PATCH ::= [before|replace|after] TRAMPOLINE`）で
  `-M`/`-P` 構文を確認済み。
- `hello.c` / `evil.c` はネイティブに `-Wall` で警告なくコンパイル確認済み。
- `mtd_tracer.c` は標準的なptrace実装（x86_64ガード付き）。
- **演習1 (`mtdnet.py`) はサンドボックスで完全に実行・動作確認済み**。

受講生環境（Docker Desktop + Rosetta 2、または x86_64 Linux）で
`docker compose up -d --build && docker compose exec lab ./exercise2-syscall-mtd/run_demo.sh`
を実行して最終確認することを推奨。`docker-compose.yml` は Dockerfile / compose 構文とも
検証済みだが、Dockerビルド自体は本サンドボックス（docker不在）では未実行。
