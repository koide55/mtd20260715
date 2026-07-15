# 演習2: システムコールレベルMTD（プロセス内・syscallゲート監視）
# Exercise 2: System-call-level MTD (in-process syscall-gate monitor)

**e9patch v1.0.1 向け・プロセス内方式。**
旧演習の AWS EC2 / カーネルリコンフィグ前提を廃し、さらに ptrace にも依存しない
**プロセス内方式**に作り直した。これにより **Apple Silicon の Docker Desktop
(Rosetta 2) でもそのまま動作**する。

*In-process design for e9patch v1.0.1. It removes the old AWS-EC2 / kernel-reconfig
setup AND the ptrace dependency, so it runs as-is under Docker Desktop's Rosetta 2
on Apple Silicon.*

---

## なぜプロセス内方式なのか / Why in-process

「システムコール番号を実行時に監視・シャッフルする」古典的なMTDは、プロセスが実際に
発行するシステムコール番号を観測する実行時コンポーネントを必要とする。native x86_64 では
これを ptrace で実装できるが、**Docker Desktop の Rosetta 2（Apple Silicon）では成立しない**：

- Rosetta は **ARM64 の Linux カーネル**上で x86_64 ユーザコードを翻訳実行する。
- x86_64 の `syscall` 命令に来ると、Rosetta が対応する **ARM64 のシステムコール**を発行する。
- そのため ptrace / seccomp がカーネル境界で見るのは ARM64 のシステムコールで、
  x86_64 のシステムコール番号（`orig_rax`）は存在しない（実測で `orig_rax=0`、さらに
  Rosetta 由来の SIGTRAP で ptrace 監視が破綻することを確認済み）。

一方 **e9patch の計装コードは完全にユーザ空間で動く**（Rosetta が他のx86_64コードと同様に翻訳）。
そこで MTD の判定点を**プロセス内**へ移す：フックが**信頼バイナリの全 `syscall` ゲート**に座り、
per-deployment のシステムコールポリシーを強制する。e9patch は syscall 命令を100%計装するため、
非実行スタック環境で **バイナリ自身の `syscall` ガジェットを再利用してシェルを起動する
コード再利用攻撃（ret2syscall / ROP）**もこのゲートを通り、検知・遮断できる。

*e9patch instrumentation runs entirely in user space, so we move the MTD enforcement point
INTO the process. The hook sits on every syscall gate of the trusted binary and enforces a
per-deployment policy. Since e9patch patches 100% of syscall sites, a code-reuse payload
(ret2syscall/ROP on an NX stack) that spawns a shell through the program's own syscall
gadget passes through the gate and is blocked.*

> native x86_64 環境向けの「番号ランダム化 + ptrace で外部syscallを検知」する版に関心が
> あれば別途用意可能（Rosettaでは不可）。本演習はプロセス内方式に統一している。

---

## 前提 / Prerequisites

- **推奨: Docker Compose**（同梱。Apple Silicon でも Rosetta 2 で動作）
- ローカル実行の場合: **x86_64 Linux** + `git`, `gcc`, `g++`, `make`

## クイックスタート（Docker 推奨）/ Quick start (Docker, recommended)

リポジトリのルートで（e9patch はイメージに構築済み）：

```console
$ docker compose up -d --build
$ docker compose exec lab ./exercise2-syscall-mtd/run_demo.sh
```

> Apple Silicon では Docker Desktop の「Use Rosetta for x86/amd64 emulation」を有効に。
> ptrace を使わないため特別な権限（SYS_PTRACE 等）は不要。

## クイックスタート（ローカル / x86_64 Linux）

```console
$ ./setup.sh          # e9patch v1.0.1 をソースからビルド（初回のみ）
$ ./run_demo.sh
```

---

## 期待される出力 / Expected output

```
1) Trusted program with the MTD gate active (runs normally):
[mtd] syscall gate active (instance 0x....); blocking: execve(59) execveat(322)
Hello, world

2a) Trusted 'victim' program, benign run:
[mtd] syscall gate active (instance 0x....); blocking: execve(59) execveat(322)
victim: doing benign work (this printf is a write syscall)

2b) 'victim pwn' WITHOUT MTD (shell-spawning path succeeds):
victim: doing benign work (this printf is a write syscall)
### SHELL OBTAINED (uid=0)

2c) 'victim pwn' WITH the MTD syscall gate (execve blocked):
[mtd] syscall gate active (instance 0x....); blocking: execve(59) execveat(322)
victim: doing benign work (this printf is a write syscall)
[mtd] INTRUSION BLOCKED: disallowed system call execve (rax=59) at syscall gate 0x...
(exit 42 from victim.mtd: 42 means the MTD gate blocked the shell.)

3) Diversified policy (MTD_BLOCK): same binary, different gate per run.
[mtd] syscall gate active (instance 0x....); blocking: write(1) execve(59) execveat(322)
[mtd] INTRUSION BLOCKED: disallowed system call write (rax=1) at syscall gate 0x...
```

`instance` の値と `gate` アドレスは実行ごとに変わる。

---

## 構成要素 / Components

| ファイル / File | 役割 / Role |
|---|---|
| `syscall_mtd.c` | e9patchフック。全 `syscall` ゲートでポリシーを強制し、禁止syscallを検知・遮断 |
| `hello.c` | 正規プログラム（計装しても普通に動く）/ benign baseline |
| `victim.c` | 正規プログラムだが `pwn` 指定でシェル起動パスに入る（攻撃をシミュレート）|
| `setup.sh` | ローカル用: e9patch v1.0.1 をビルド（Dockerでは不要）|
| `Makefile`, `run_demo.sh` | ビルド・デモ |

## e9patch コマンド（v1.0.1 構文）

```console
# フックをコンパイル（静的リンク対象のため -DNO_GLIBC=1 が必須）
$ e9patch/e9compile.sh syscall_mtd.c -I e9patch/examples -DNO_GLIBC=1

# 各 syscall 命令の直前にMTDゲートを挿入
$ e9patch/e9tool -M 'asm=/syscall/' \
      -P 'before entry(state)@syscall_mtd' victim -o victim.mtd
```

**旧版（〜2021）からの構文変更:**

| | 旧版 / Old | 新版 / New (v1.0.1) |
|---|---|---|
| マッチ | `--match 'asm=sys(?:enter\|call)'` | `-M 'asm=/syscall/'` |
| パッチ | `--action 'call [before] func(&rax)@func'` | `-P 'before entry(state)@syscall_mtd'` |
| 引数 | `&rax` | `state`（`state->rax`, `state->rip` を参照）|
| ヘッダ | 手書き `stdlib.c` | 同梱 `examples/stdlib.c` + `-DNO_GLIBC=1` |

## ポリシーのカスタマイズ / Customizing the policy

環境変数 `MTD_BLOCK` に遮断したいsyscall番号をカンマ区切りで指定（既定: `59,322` = execve/execveat）。

```console
$ MTD_BLOCK="59,322,257" ./victim.mtd     # openat も遮断
```

デプロイやサーバごとに監視・遮断するsyscall集合を変える／時間で回転させることで、
攻撃者が前提にできるsyscall ABIを不確実にする（＝moving target）。

---

## グループ演習 / Group discussion

1. Webサービスに外部からバイナリが送り込まれ実行される未知の脆弱性を仮定する。
   このプロセス内syscallゲート監視で攻撃を検出・防御できるか。できない攻撃形態は何か。
   （ヒント: 実行可能スタック上の独自 `syscall` 命令は計装対象外。NXスタック＋ret2syscallは対象内。）
2. 演習1（URL等）のMTDと同時に組み合わせた場合、「平均攻撃成功時間間隔」はどう変化するか。
3. 遮断集合をデプロイ／時間ごとに多様化・回転させることの効果と、正規利用者への影響。
4. **旧版との相違点**: 旧演習はLinuxカーネルのリコンフィグ、その後の版は ptrace を伴った。
   本版（e9patch v1.0.1・プロセス内・カーネル非依存）の相違点・利点・欠点は何か。
5. Rosetta のような翻訳環境では、なぜカーネル境界（ptrace/seccomp）での
   syscall番号MTDが成立しないのか。プロセス内方式はその制約をどう回避しているか。

---

## 検証状況 / Verification status

- **e9patch v1.0.1 のビルドと計装は Docker (amd64/Rosetta) 上で成功を確認済み**（`num_patched 100%`）。
- ptrace 方式が Rosetta で不可であることを実測で確認（`orig_rax=0` + SIGTRAP）。本演習は
  その知見を踏まえプロセス内方式に統一した。
- コード（フック/デモ）は e9patch v1.0.1 の API（`init(argc,argv,envp)`, `struct STATE`,
  `getenv`, `getrandom`, `exit`, `SYS_execve/execveat`）に準拠。
- Docker上での `run_demo.sh` の最終実行確認は受講生環境で実施のこと。
