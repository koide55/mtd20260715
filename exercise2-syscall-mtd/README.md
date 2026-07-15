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
そこで MTD の判定点を**プロセス内**へ移す：フックが**信頼バイナリの `syscall` ゲート**に座り、
per-deployment のシステムコールポリシーを強制する。制御を奪われた実行フローがこのゲートに
funnel してシェルを起動しようとしても、検知・遮断できる。

*e9patch instrumentation runs entirely in user space, so we move the MTD enforcement point
INTO the process. The hook sits on the trusted binary's syscall gates and enforces a
per-deployment policy; a hijacked control flow funnelling into a gate to spawn a shell is blocked.*

### 重要: 動的リンク + インラインsyscall / Dynamic linking + inline syscalls

Rosetta 上では、**静的リンク glibc の起動初期の syscall を計装するとトランポリンがクラッシュ**する
（実測: 数命令ぶん動作後に SIGSEGV）。そこで本演習は次のようにして回避する：

- 対象 `victim` を**動的リンク**でビルド（glibc 起動初期の syscall は libc.so 側＝計装対象外）。
- 監視したい syscall（write / execve）は **`victim` 自身のインラインasm `syscall` 命令**で発行し、
  そこだけを e9patch が計装する。

> **実運用では / In a real deployment:** native x86_64 ホストなら `libc.so` 自体を計装すれば
> 通常の libc 経由の syscall も監視できる（静的バイナリの計装でも可）。本演習は Rosetta/Docker
> で確実に動くようインライン方式にしている。

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
1) Trusted program, benign run, MTD gate active (write allowed):
[mtd] syscall gate active (instance 0x....); blocking: execve(59) execveat(322)
victim: doing benign work (inline write syscall)

2a) 'victim pwn' WITHOUT MTD (shell-spawning path succeeds):
victim: doing benign work (inline write syscall)
### SHELL OBTAINED (uid=0)

2b) 'victim pwn' WITH the MTD syscall gate (execve blocked):
[mtd] syscall gate active (instance 0x....); blocking: execve(59) execveat(322)
victim: doing benign work (inline write syscall)
[mtd] INTRUSION BLOCKED: disallowed system call execve (rax=59) at syscall gate 0x...
(exit 42 from victim.mtd: 42 means the MTD gate blocked the shell.)

3) Diversified policy (MTD_BLOCK): same binary, different gate per run.
[mtd] syscall gate active (instance 0x....); blocking: write(1) execve(59) execveat(322)
[mtd] INTRUSION BLOCKED: disallowed system call write (rax=1) at syscall gate 0x...
```

`instance` の値と `gate` アドレスは実行ごとに変わる。3) では benign な inline write すら
ゲートで止まる（＝ポリシーが実行時に効いていることの確認）。

---

## 構成要素 / Components

| ファイル / File | 役割 / Role |
|---|---|
| `syscall_mtd.c` | e9patchフック。`syscall` ゲートでポリシーを強制し、禁止syscallを検知・遮断 |
| `victim.c` | 動的リンクの被害プログラム。自前インラインsyscallで write/execve を発行（`pwn` でシェル起動）|
| `setup.sh` | ローカル用: e9patch v1.0.1 をビルド（Dockerでは不要）|
| `Makefile`, `run_demo.sh` | ビルド・デモ |

## `victim.c` の解説 / Walkthrough of victim.c

`victim` は「MTDゲートに守らせたい信頼プログラム」を模したもの。ポイントは
**システムコールを自前のインライン `syscall` 命令で発行する**ことにある。

```c
/* プログラム自身の `syscall` 命令でシステムコールを発行する薄いラッパ */
static long sys3(long n, long a, long b, long c)
{
    long r;
    asm volatile ("syscall"          /* ← この命令が e9patch の計装対象になる */
                  : "=a"(r)           /* 返り値 rax → r                        */
                  : "a"(n),           /* rax = システムコール番号 n           */
                    "D"(a),           /* rdi = 第1引数 a                       */
                    "S"(b),           /* rsi = 第2引数 b                       */
                    "d"(c)            /* rdx = 第3引数 c                       */
                  : "rcx", "r11", "memory");  /* syscall命令はrcx/r11を破壊    */
    return r;
}
```

- **なぜインラインか**: 通常のCプログラムは `write()`/`execve()` を **libc 経由**で呼ぶため、
  実際の `syscall` 命令は `libc.so` の中にある（＝メインバイナリを計装しても捕まらない）。
  そこで `victim` は `sys3()` で **自分のコード内に `syscall` 命令を持ち**、e9patch がそこを
  計装できるようにしている（Rosetta で安定動作する理由 = 静的glibc起動初期を触らない）。
- **AT&T記法のレジスタ制約**: `"a"`=rax, `"D"`=rdi, `"S"`=rsi, `"d"`=rdx。x86_64 の
  システムコール規約（番号=rax、引数=rdi,rsi,rdx,r10,r8,r9）に対応。ここでは3引数まで扱う。

```c
int main(int argc, char **argv)
{
    /* (1) 平常時の処理 = 良性の write。ゲートは許可し、そのまま実行される */
    const char *msg = "victim: doing benign work (inline write syscall)\n";
    sys3(SYS_write, 1, (long)msg, (long)strlen(msg));   /* write(1, msg, len) */

    /* (2) `victim pwn` のときだけシェル起動パスに入る                        */
    if (argc > 1 && strcmp(argv[1], "pwn") == 0) {
        static char *a[] = { "/bin/sh", "-c",
                             "echo '### SHELL OBTAINED (uid='$(id -u)')'", 0 };
        static char *e[] = { 0 };
        sys3(SYS_execve, (long)a[0], (long)a, (long)e); /* execve(path,argv,envp) */
        /* ゲートに遮断されなければ以降は実行されない（execveで置き換わる）    */
        const char *f = "execve failed\n";
        sys3(SYS_write, 2, (long)f, (long)strlen(f));
        return 1;
    }
    return 0;
}
```

- **(1) 良性パス**: `write`（番号1）を発行。既定ポリシー（遮断=execve/execveat）では許可 →
  そのままメッセージが表示される。計装済みだが「通す」動作を確認できる。
- **(2) 攻撃パス（`pwn`）**: `execve("/bin/sh", ...)` を発行。これは
  「制御を奪われてシェル起動に至った」状況の**再現**。
  - `./victim pwn`（計装なし）→ execve が実行されシェルが起動（`### SHELL OBTAINED`）。
  - `./victim.mtd pwn`（計装あり）→ `execve` の直前でフックが発火し、ポリシー違反として
    **遮断（`exit(42)`）**。execve は実行されない。
- **攻撃モデルとの対応**: 現実の攻撃では、この `execve` は攻撃者が用意したペイロードが
  プログラムの syscall ゲートに funnel してくることに相当する。`victim` では議論を単純化する
  ため、`pwn` 引数で同じ地点に到達させている。

> `SYS_write=1`, `SYS_execve=59` は x86_64 のシステムコール番号。`victim` は**動的リンク**で
> ビルドするので、libc 起動初期の syscall は `libc.so` 側に残り、計装されるのは `victim` 自身の
> `sys3()` 内 `syscall` 命令だけ（`num_patched = 3/3` = benign write ×2 と execve ×1）。

## e9patch コマンド（v1.0.1 構文）

```console
# フックをコンパイル（動的リンク対象なので -DNO_GLIBC は不要）
$ e9patch/e9compile.sh syscall_mtd.c -I e9patch/examples

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
| ヘッダ | 手書き `stdlib.c` | 同梱 `examples/stdlib.c` |

## ポリシーのカスタマイズ / Customizing the policy

環境変数 `MTD_BLOCK` に遮断したいsyscall番号をカンマ区切りで指定（既定: `59,322` = execve/execveat）。

```console
$ MTD_BLOCK="59,322,257" ./victim.mtd     # openat も遮断
```

デプロイやサーバごとに監視・遮断するsyscall集合を変える／時間で回転させることで、
攻撃者が前提にできるsyscall ABIを不確実にする（＝moving target）。

---

## グループ演習 / Group discussion

1. このプロセス内syscallゲート監視で検出・防御できる攻撃形態／できない形態は何か。
   （ヒント: 計装されるのはバイナリ内の `syscall` 命令のみ。libc.so 経由や実行可能スタック上の
   独自syscallは対象外。実運用でこれらも監視するには何を計装すべきか＝libc.so／静的バイナリ。）
2. 演習1（URL等）のMTDと同時に組み合わせた場合、「平均攻撃成功時間間隔」はどう変化するか。
3. 遮断集合をデプロイ／時間ごとに多様化・回転させることの効果と、正規利用者への影響。
4. **旧版との相違点**: 旧演習はLinuxカーネルのリコンフィグ、その後の版は ptrace を伴った。
   本版（e9patch v1.0.1・プロセス内・カーネル非依存）の相違点・利点・欠点は何か。
5. Rosetta のような翻訳環境では、なぜカーネル境界（ptrace/seccomp）での
   syscall番号MTDが成立しないのか。プロセス内方式はその制約をどう回避しているか。

---

## 検証状況 / Verification status

Docker (amd64/Rosetta) 上で実機検証した結果に基づく：

- e9patch v1.0.1 のビルド・計装は成功（`num_patched 100%`）。
- **ptrace 方式は Rosetta で不可**（実測 `orig_rax=0` + SIGTRAP）→ プロセス内方式へ。
- **静的リンクの計装は Rosetta でトランポリンがクラッシュ**（実測 SIGSEGV）→ 動的リンク採用。
- **動的リンク + 自前インライン `syscall` の計装は Rosetta で正常動作を確認**
  （標準 `print` トランポリンで `syscall` 命令のフック実行→出力→exit 0 を確認）。
  本演習はこの構成に統一。
- フックは e9patch v1.0.1 の API（`init(argc,argv,envp)`, `struct STATE`, `getenv`,
  `getrandom`, `exit`, `SYS_execve/execveat`）に準拠。
- 完成した `run_demo.sh` の最終通し確認は受講生環境で実施のこと。
