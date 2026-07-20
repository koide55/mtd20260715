# MTD (Moving Target Defense)

*ムービング・ターゲット・ディフェンス演習 / Moving Target Defense Hands-on*

更新日 / Updated: 2026-07-15
本資料は 2025-07-27 版スライド (`MTD-prosecit-20250727.pdf`) をMarkdown化し、
演習2 (システムコールMTD) を **e9patch v1.0.1**（2026年6月リリース）向けに全面刷新、
さらに演習1・2を組み合わせた**演習3（多層防御）**を新設したものです。
演習は **Docker Compose** で実行でき、Apple Silicon (Rosetta 2) でも動作します。

---

## 1. サイバー空間の状況 / Situation in Cyberspace

攻撃者に有利な状況が続いている。標的システムは静的（Static）であり、
攻撃者はそれを繰り返し観測・攻撃できる。

**攻撃者に有利な点 / Attackers have advantages:**

- 短時間に攻撃を実行可能 / can execute an attack in a short time
- ひとつ攻撃可能な脆弱性を見つければ良い / need to find only a single vulnerable entry point
- 偵察と準備に多くの時間を掛けることが可能 / unlimited time for reconnaissance and preparation
- 継続的・適応的・目的主導的（最小攻撃コストで最大利益）/ persistent, adaptive and incentive-driven (minimum attack cost with maximum dangerous outcome)

> いままでの（静的な）防御技術では不十分。
> Traditional (static) defense techniques are not sufficient.

---

## 2. 解決したい課題 / Problem Statements

- どうすれば攻撃者側の負荷と、攻撃に要する時間・コストを増やせるか。
  *How can we increase attackers' workload, attack time and costs?*
- どうすれば攻撃成功確率を（時間の経過とともに）減らせるか。
  *How can we reduce the probability of attack success over time?*
- 新しい防御技術を追加してシステムの防御能力を拡張できるか。
  *How can we enhance a system's resiliency with additional defense techniques?*
- 複数の多様化を行う方法が、最小限の防御コストでセキュアな防御システムとなりうるか。
  *Whether hybrid diversification approaches can introduce a secure defense system with minimum defense cost?*

---

## 3. MTDの基本的な考え方 / Moving Target Defense Approach

MTD (Moving Target Defense) の基本的な考え方は、
**複数のシステムの変更を制御することにより、保護したいシステムの情報に関する不確実性を高め、
攻撃側を複雑にすること**である。

*The concept of controlling the alteration of multiple systems with the aim to increase
uncertainty about a protected system's information and give complexity for attackers.*

これにより攻撃者の調査・攻撃にかかるコストを増やし、攻撃の機会（attack window）を減らせる。

*By doing so, we can reduce the attack window of opportunity and increase the costs of
attackers' probing and attack efforts.*

---

## 4. 停まっている標的 vs 動いている標的 / Stationary vs Moving Target

```
停まっている標的を射撃            動いている標的を射撃
Shoot a stationary target        Shoot a moving target

     🎯                                🎯→ →  →  🎯
   ┌─────┐   ← easy                  ┌─────┐  ??? ← hard
   │ hit │                          │miss │
   └─────┘                          └─────┘
```

静的な標的は当てやすい。標的が動けば命中は難しくなる — これがMTDの直感的な比喩。
*A static target is easy to hit; a moving target is hard — the core intuition of MTD.*

---

## 5. この技術の目的 / Goals of This Technique

- 攻撃の不確実性を高める / To increase the uncertainty for the attacker
- 攻撃に必要な努力とコストを増加させる / To increase the attacker's effort and cost
- 保護したいシステムを探査しにくく予測しにくい標的にする / hard to exploit, unpredictable destination
- 攻撃成功確率を（時間の経過とともに）低減する / To reduce the probability of attack success over time

MTD技術によりランダム性が加わるため、特定の瞬間にシステムがどのような構成かわからない。
このランダム性により、攻撃者がゼロデイ攻撃などを成功させるコストが増加しうる。

*MTD techniques introduce randomness, so it is unknown which configuration is in place at any
particular moment. This can increase the cost for an attacker to succeed, e.g. with zero-day attacks.*

**MTDフレームワーク設計時の3つの考慮事項 / Three considerations when designing an MTD framework:**
**(What / When / How)** — 何を、いつ、どのように動かすか。

---

## 6. What to Move ? / 何を動かすか

攻撃者を混乱させるために動的に変更できるシステム構成・属性・構成要素
（すなわち **attack surface**）は何か。

*What system configuration, attribute or element (i.e. attack surface) can be dynamically changed
to confuse attackers.*

例 / For example:

- 命令セット / instruction sets
- アドレス空間のレイアウト / address space layouts
- IPアドレス・ポート番号 / IP addresses, port numbers
- プロキシ・仮想マシン / proxies, virtual machines
- OS / operating systems
- ミドルウェア・フレームワーク・ソフトウェア / middleware, frameworks, software programs
- **システムコール番号 / system call numbering** ← 演習2で扱う / covered in Exercise 2

---

## 7. When to Move ? / いつ動かすか

MTDシステムをある状態から別の状態へ変更する適切なタイミングを決定し、
攻撃者が現在の状態で得た情報や進捗を無効化する。

*Deciding the optimal time to change from the current state to a new state,
invalidating information or progress gained by an attacker.*

- 反応的な適応 / Reactive adaptation
- 積極的な適応 / Proactive adaptation
- ハイブリッド適応 / Hybrid adaptation

---

## 8. How to Move ? / どのように動かすか

どのようにシステムの属性・構成要素（すなわち標的）を変化させ、
攻撃者側の予測不可能性・不確実性を増加させ、攻撃者を混乱させるか。

*How to change the moving attributes or elements (i.e. targets) to increase
unpredictability and/or uncertainty.*

- シャッフリング / shuffling
- 多様性 / diversity
- 冗長性 / redundancy

---

## 9. MTD技術の例 / Examples of MTD Techniques

| 手法 / Technique | 内容 / Description | 効果 / Benefit |
|---|---|---|
| **Shuffling** | 一定周期ごとに仮想IPを X → Y に変更 / change virtual IP every period | 性能と効率 / Performance and Efficiency |
| **Diversity** | 異なる構成（ソフトウェアスタック）/ different configurations (software stack) | 回復力と堅牢性 / Resilience and Robustness |
| **Redundancy** | 1つ以上の追加サーバ / one or more additional servers | 信頼性と可用性 / Reliability and Availability |

正規の利用者に対する **可用性 (Availability)** の確保がトレードオフとして常に重要。
*Ensuring availability for legitimate users is always a key trade-off.*

---

## 10. 異なる階層におけるMTD技術の変更要素 / Elements at Different Layers

| 階層 / Layer | Shuffling | Diversity | Redundancy |
|---|---|---|---|
| **Application** | TCP/UDPポート番号 | Web: Apache, IIS 等 / App: Java, PHP 等 / DB: MySQL, Oracle 等 | Webサービス複製 / アプリ複製 / DB複製 |
| **OS-host** | IPアドレス, **システムコール番号 / System Call Numbering** | Windows/Linux/Unix の各種バージョン | ホストOS・VM複製 |
| **VM-instance** | 仮想IPアドレス, フェイルオーバー | VMware, ESXi, KVM, VirtualBox 等 | ハイパーバイザ複製 |

> 本演習では **Application層のポート番号 (演習1)** と **OS-host層のシステムコール番号 (演習2)**
> を扱い、**演習3**でこの2層を組み合わせた多層防御を体験する。
> This hands-on covers **port numbering at the Application layer (Ex.1)** and
> **system call numbering at the OS-host layer (Ex.2)**, then **Ex.3 combines both layers**.

---

# 演習1: ネットワークレベルMTD（URLシャッフリング）
# Exercise 1: Network-level MTD (URL Shuffling)

---

## 11. ネットワークレベルのMTD / Network-level MTD

Webシステムにおけるネットワーク識別子を常に変更する。
*Continuously change the network identifiers of a web system.*

- IPアドレス, ポート番号, パラメータ識別子 / IP address, port number, parameter identifiers

例 / Example:

```
https://192.168.1.100:8080/kaikei?userid=u1010
        └────┬─────┘ └┬─┘ └─┬─┘  └─┬─┘
        192.168.1.100 8080 kaikei userid   ← いずれも変更可能 / all changeable
```

- **when** : 正規のユーザのアクセス時 / on legitimate user access
- **what** : URLの構成要素 / the components of the URL
- **how** : 正規のユーザ・クライアントのアルゴリズムで / by an algorithm shared with legitimate clients

---

## 12. URLシャッフリングの目的 / Purpose of URL Shuffling

- 組織内にAPT攻撃で使われるようなマルウェアが侵入したとき、それを検出したり
  ハニーポットへ誘導したりするのに利用できる。
  *Detect APT-style malware that has intruded into an organization, or lure it to a honeypot.*
- 大規模組織は単一サービスではなく、複数サービスが互いに通信して構成される。
  *Large organizations consist of multiple services communicating with each other.*
- その通信にURLシャッフリングMTDを適用すれば、不正な通信を行っているものはマルウェアの可能性が高い。
  *If URL shuffling is applied to that traffic, anything communicating incorrectly is likely malware.*

---

## 13. MTD機能を持つWebアプリケーション / A Web App with MTD

クライアント側からポート番号を指定できる機能を追加する。

1. 最初、ユーザは指定されたポート番号（初期値）でアクセス可能。
   *Initially the user accesses on a given (default) port.*
2. `GET /restart?port=<新しいポート番号>` により、次のアクセス時のポート番号に変更可能。
   *`GET /restart?port=<new>` changes the port used for the next access.*
3. それ以外は普通のWebアプリケーションとして動作する。
   *Otherwise it behaves as an ordinary web application.*

```
GET /restart?port=9911     ← 次回から 9911 に移動 / move to 9911 next time
```

---

## 14. ネットワーク／アプリレベルMTDフレームワークの例 / Example Framework

```
                 ┌──────────────────────────────────────┐
  Attackers ───▶ │  MTD Service                          │
   (probing)     │  IP address / Port Number             │──▶ server replica #1
                 │  randomization                        │──▶ server replica #2
                 │                                        │──▶ server replica #3
                 └──────────────────────────────────────┘
```

IPアドレス／ポート番号のランダム化を、複数のサーバレプリカの前段に置く構成。
*IP/port randomization sits in front of multiple server replicas.*

---

## 15. サンプル実装 / Sample Implementation

MTD機能を持つ簡単なWebアプリケーションを Python3 標準ライブラリのみで実装する。
*A simple web app with MTD, implemented using only the Python3 standard library.*

- ソース / Source: `exercise1-network-mtd/mtdnet.py`
- （旧版の Gist 実装 `mtdnet2.py` をベースに、Python 3.12+ でも動くよう更新）
  *Updated from the older gist `mtdnet2.py` to run on Python 3.12+.*

実行例 / Example run:

```console
$ python3 mtdnet.py            # 初期ポート 8123 で起動 / starts on port 8123
$ curl localhost:8123          # → "This is a response. cnt=1000"  （アクセス可 / OK）
$ curl "localhost:8123/restart?port=9999"   # 次回から 9999 に移動 / move to 9999
$ curl localhost:8123          # → 接続不可 / refused
$ curl localhost:9999          # → "This is a response."  （アクセス可 / OK）
$ curl "localhost:9999/restart?port=8111"   # さらに 8111 へ移動 / move to 8111
$ curl localhost:8111          # → アクセス可 / OK
```

---

## 16. MTD演習1 (課題40分) / Exercise 1 Assignment (40 min)

**個人演習 / Individual (hands-on):**

1. `exercise1-network-mtd/` の手順に従い、1つのPythonプログラムだけで試せる
   ネットワークレベルMTDを試してみよ。

**グループ演習 / Group (discuss & present):**

1. 複数サーバがネットワークレベルMTDで互いに通信しあう実験環境を、どう設計・実装すれば実現できるか。
2. その環境でMTD手法を評価するとしたら、評価指標は何か。測定にはどんな実験が必要か。
3. MTDでは正規ユーザの余分な負担が少ないことが必要。この観点で「利用者が次のURLを指定する方法」をどう評価すべきか。
4. 正規ユーザ側から指定してURLを変更するより安全な方法は。
5. 他のMTDと組み合わせると「平均攻撃成功時間間隔 (MTTC)」はどう変化するか。
6. 企業内に多数のサーバがあり互いに通信する状況で、ネットワークレベルMTDが
   マルウェア・ツールの侵入検知に役立つのはなぜか。

---

## 17. 演習1のまとめ / Summary of Exercise 1

- MTD (Moving Target Defense) という技術を紹介した。
- ネットワークレベルのMTD（URL/ポートシャッフリング）を実装・体験した。
- （可用性などとのトレードオフはあるが）攻撃者側の負担を大きくする意味でMTDは有効に利用可能。
- 参考: `/bin/sh` を別名に変えるだけでも多くのシェルコードは失敗する — これも一種の多様化MTD。

---

# 演習2: カーネル／システムコールレベルMTD（システムコール・シャッフリング）
# Exercise 2: Kernel / System-call-level MTD (System Call Shuffling)

> **本演習は 2026-07-15 に全面刷新。** 旧版が前提としていた AWS EC2 の事前構築済み環境
> (`~/work/mtd`, Rust製tracer) を廃し、**e9patch v1.0.1 をソースからビルドして
> 任意のx86_64 Linux上で完結する自己完結型**に作り直した。
> *This exercise was fully reworked on 2026-07-15: the old pre-provisioned AWS EC2 setup
> was removed in favour of a self-contained lab that builds **e9patch v1.0.1** from source
> and runs on any x86_64 Linux.*

---

## 18. 演習の準備 / Preparation

旧版では割り当て済み仮想マシンへ秘密鍵でSSHログインしていたが、本版では不要。
*The old version required SSH login with a private key to a pre-assigned VM. No longer needed.*

**推奨: Docker Compose で実施 / Recommended: use Docker Compose.**
イメージビルド時に e9patch v1.0.1 が組み込まれるので、起動後すぐ演習できる。

```console
# リポジトリのルートで / at the repository root
$ docker compose up -d --build
$ docker compose exec lab ./exercise2-syscall-mtd/run_demo.sh
$ docker compose exec lab bash        # 対話シェル / interactive shell
```

- **Apple Silicon (M1/M2/M3…)** でも動作する。イメージは `platform: linux/amd64` 指定で、
  **Docker Desktop の Rosetta 2** エミュレーションで実行される。
  Docker Desktop 設定の「Use Rosetta for x86/amd64 emulation」を有効にすること。
- ptrace を使わない**プロセス内方式**なので、`SYS_PTRACE` / `seccomp:unconfined` などの
  特別な権限は不要（`docker-compose.yml` でも付与していない）。

Dockerを使わない場合（x86_64 Linux 上）/ Without Docker (on x86_64 Linux):

```console
$ cd exercise2-syscall-mtd
$ ./setup.sh          # e9patch v1.0.1 をソースから取得・ビルド / clone & build e9patch v1.0.1
$ make && ./run_demo.sh
```

---

## 19. e9patch とは / What is e9patch ?

**e9patch** は x86_64 Linux ELFバイナリ用の強力な**静的バイナリ書き換えツール**。
制御フロー復元なしにバイナリを書き換えられる（PLDI'2020）。

*e9patch is a powerful static binary rewriter for x86_64 Linux ELF binaries; it rewrites
without control-flow recovery.*

- 本演習で使うバージョン / Version used here: **v1.0.1**（2026-06-22 リリース）
- リポジトリ / Repo: <https://github.com/GJDuck/e9patch>
- 2つのツールをビルド / builds two tools:
  - `e9patch` … 書き換えバックエンド / rewriter backend
  - `e9tool` … 線形逆アセンブルのフロントエンド / linear-disassembly frontend

**旧版からの主な変更 / Key changes from the old slides:**

| 項目 | 旧版 (~2021) | 新版 (v1.0.1) |
|---|---|---|
| マッチ指定 | `--match 'asm=sys(?:enter|call)'` | `-M 'asm=/syscall/'` |
| パッチ指定 | `--action 'call [before] func(&rax)@func'` | `-P 'before entry(state)@syscall_mtd'` |
| フック標準ヘッダ | 手書きの `stdlib.c` | 同梱の `stdlib.c`（`SYS_*`, `getrandom`, `getenv`, `STATE` 完備）|
| 実行環境 | EC2上の事前構築環境 | Docker Compose（e9patch同梱）/ ローカルは `./setup.sh` |
| 判定点 | ptrace（カーネル境界） | **プロセス内フック**（Rosetta/Docker可・ptrace不要）|

---

## 20. なぜ「プロセス内方式」か / Why an In-process Design

古典的な「syscall番号を実行時に監視・シャッフル」するMTDは、プロセスが実際に発行する
syscall番号を観測する実行時コンポーネントを要する。native x86_64 なら ptrace で作れるが、
**Docker Desktop の Rosetta 2（Apple Silicon）では成立しない**：

- Rosetta は **ARM64 カーネル**上で x86_64 ユーザコードを翻訳実行する。
- x86_64 の `syscall` 命令に来ると Rosetta が対応する **ARM64 の syscall** を発行する。
- ゆえに ptrace/seccomp がカーネル境界で見るのは ARM64 の syscall で、x86_64 の番号
  (`orig_rax`) は存在しない（実測 `orig_rax=0`、Rosetta 由来の SIGTRAP で ptrace が破綻）。

**e9patch の計装はユーザ空間で動く**ので、MTDの判定点を**プロセス内**へ移す。
フックが信頼バイナリの**全 `syscall` ゲート**に座り、ポリシーを強制する。

> **用語**：**計装（instrumentation）**＝e9patch でプログラムの `syscall` の直前に
> 監視コード（フック）を埋め込むこと。本演習では **MTD保護なし = `victim`**、
> **MTD保護あり = `victim.mtd`**（`.mtd` が目印）と呼ぶ。

---

## 21. システムコールMTDフレームワークの概要 / Framework Overview（in-process）

```
   信頼バイナリ victim.mtd（動的リンク・自前 syscall 命令を e9patch で計装）
   ┌────────────────────────────────────────────────────────────┐
   │  ... アプリのコード ...                                      │
   │        │ 自前インライン syscall 命令の直前で                 │
   │        ▼                                                     │
   │   ┌──────────────── syscall_mtd フック（プロセス内）───────┐ │
   │   │ state->rax（syscall番号）をポリシーと照合               │ │
   │   │   許可         → そのまま syscall を実行                 │ │
   │   │   禁止(execve) → 侵入として遮断し exit(42)              │ │
   │   └────────────────────────────────────────────────────────┘ │
   └────────────────────────────────────────────────────────────┘
   遮断集合はデプロイ/実行ごとに多様化（環境変数 MTD_BLOCK）= moving target
```

制御を奪われた実行フローがこのゲートに funnel してシェル（execve）を起動しようとしても、
フックが検知・遮断する。（対象は動的リンク＋自前インライン syscall。理由は次スライド末尾）

---

## 22. フックの中身 / The Hook (excerpt)

```c
// syscall_mtd.c （抜粋）— e9patch v1.0.1 API, in-process
#include "stdlib.c"
static long blocked[16]; static int n_blocked = 0;

void init(int argc, char **argv, char **envp) {
    environ = envp;
    const char *b = getenv("MTD_BLOCK");     // 例 "59,322"
    if (b && *b) parse_block(b);
    else { blocked[n_blocked++] = SYS_execve;    // 59
           blocked[n_blocked++] = SYS_execveat; } // 322
}

void entry(struct STATE *state) {            // 各 syscall 命令の直前
    for (int i = 0; i < n_blocked; i++)
        if (state->rax == blocked[i]) {      // 禁止 syscall を検知
            fprintf(stderr, "[mtd] INTRUSION BLOCKED: ... rax=%ld\n", state->rax);
            exit(42);                        // syscall を実行させず停止
        }
}
```

`e9tool` コマンド（v1.0.1 構文）:

```console
$ E9PATCH/e9compile.sh syscall_mtd.c -I E9PATCH/examples
$ E9PATCH/e9tool -M 'asm=/syscall/' -P 'before entry(state)@syscall_mtd' victim -o victim.mtd
```

> 対象は動的リンクなので `-DNO_GLIBC` 不要。ptrace を使わないので特別な権限も不要。

---

## 23. とりあえず試してみよう！/ Let's Try It

```console
$ docker compose exec lab ./exercise2-syscall-mtd/run_demo.sh
```

```console
# 1) 正規プログラムはゲート有効でも普通に動く（inline write は許可）
[mtd] syscall gate active (instance 0x...); blocking: execve(59) execveat(322)
victim: doing benign work (inline write syscall)

# 2a) MTDなしで victim を攻撃 → シェル奪取
$ ./victim pwn
### SHELL OBTAINED (uid=0)

# 2b) MTDありで victim を攻撃 → execve がゲートで遮断
$ ./victim.mtd pwn
[mtd] INTRUSION BLOCKED: disallowed system call execve (rax=59) at syscall gate 0x...
(exit 42)
```

---

## 23b. なぜ検知できるのか / Why Detection Works

- 信頼バイナリ内の syscall 命令は **書き換え時に既知**なので e9tool がフック済み。
  → プログラムの正規パスも、そこへ **funnel してくる攻撃も同じゲート**を通る。
- ゲート（フック）は現在のポリシーに無い syscall（例: `execve`）を **侵入として遮断**する。
- 遮断集合をデプロイ/時間で多様化すれば、攻撃者が前提にできる syscall ABI が不確実になる。

**計装対象と限界 / Scope & limitation:**
Rosetta 上では静的リンク glibc 起動初期の計装がクラッシュするため、本演習は
**動的リンク + 対象自身のインライン `syscall`** を計装する（起動初期は libc.so 側で対象外）。
libc.so 経由や実行可能スタック上の独自 syscall は計装対象外。実運用で通常の libc 経由 syscall も
監視したい場合は、native x86_64 で **libc.so 自体**（または静的バイナリ）を計装する。

*Because instrumenting a static glibc's early start-up crashes under Rosetta, this lab
instruments a dynamically linked program's OWN inline `syscall` instructions. Syscalls via
libc.so (or on an executable stack) are out of scope; to cover ordinary libc syscalls in a real
deployment, instrument libc.so (or a static binary) on a native x86_64 host.*

---

## 24. MTD演習2 (課題30分) / Exercise 2 Assignment (30 min)

**個人演習 / Individual (hands-on, 30 min):**

1. スライドと `exercise2-syscall-mtd/README.md` に従い、システムコールレベルMTDを試す。

**グループ演習 / Group (discuss & present, 30 min):**

1. このプロセス内syscallゲート監視で検出・防御できる攻撃形態／できない形態は何か。
   （ヒント: 計装されるのはバイナリ内の syscall 命令のみ。libc.so 経由や実行可能スタック上の
   独自syscallは対象外。実運用で通常の libc 経由も監視するには libc.so／静的バイナリを計装。）
2. 演習1のような別のMTD（URL等）と同時に組み合わせた場合、「平均攻撃成功時間間隔」はどう変化するか（→演習3）。
3. 遮断・監視する syscall 集合をデプロイ／時間ごとに多様化・回転させる効果と、正規利用者への影響。
4. **旧版との相違点**: 旧演習はLinuxカーネルのリコンフィグ、その後の版は ptrace を伴った。
   本版（e9patch v1.0.1・プロセス内・カーネル非依存）との相違点・利点・欠点は何か。
5. Rosetta のような翻訳環境で、なぜカーネル境界（ptrace/seccomp）でのsyscall番号MTDが
   成立しないのか。プロセス内方式はその制約をどう回避しているか。

---

# 演習3: 多層防御シナリオ（ネットワークMTD × システムコールMTD）
# Exercise 3: Defense in Depth (network MTD × syscall MTD)

---

## 25. 多層防御の考え方 / Defense in Depth

演習1（ネットワークレベル）と演習2（システムコールレベル）を**独立した2層**として重ねる。

```
             ┌──────────── 第1層 / Layer 1: network MTD ───────────┐
  攻撃者   ─▶│  サービスが秘密のポート系列を移動 :8123→:9001→...    │
 (固定ポート)│  正規クライアントは系列を追従（成功率 高）           │
             │  固定ポートの攻撃者は標的を見失う（成功窓が縮小）    │
             └───────────────────────┬─────────────────────────────┘
                                      │ 万一の侵入・コード実行
                                      ▼
             ┌──────────── 第2層 / Layer 2: syscall MTD ───────────┐
             │  ペイロードの execve("/bin/sh") が syscallゲートで   │
             │  遮断される → [mtd] INTRUSION BLOCKED (exit 42)      │
             └─────────────────────────────────────────────────────┘
```

- **第1層**は攻撃者の**偵察・探索コスト**を増やす（攻撃の入口を動かす）。
- **第2層**は侵入後の**ペイロードを無効化**する（最後の砦）。
- 2層は攻撃キルチェーンの異なる段階に効くため、効果は概ね**掛け算**で積み上がる。

---

## 26. 演習3を試す / Let's Try Exercise 3

```console
$ docker compose exec lab ./exercise3-combined/scenario.sh
```

期待される出力（抜粋）/ expected output:

```console
LAYER 1 -- Network-level MTD (URL / port shuffling)
  round 0: legit client -> :8123  OK     attacker -> :8123  HIT
  round 1: legit client -> :9001  OK     attacker -> :8123  miss
  round 2: legit client -> :9002  OK     attacker -> :8123  miss
  ...
  legitimate client success: 5/5
  attacker (fixed port)     : 1/5   -> 攻撃の成功窓が縮小

LAYER 2 -- Syscall-level MTD (in-process gate)
  [no MTD]      ./victim pwn      -> ### SHELL OBTAINED (uid=0)
  [syscall MTD] ./victim.mtd pwn  -> [mtd] INTRUSION BLOCKED ... execve (rax=59)
```

正規クライアントは常にサービスに到達でき（5/5）、固定ポートの攻撃者はほぼ失敗（1/5）。
仮に侵入されても、シェル起動は第2層で遮断される。

---

## 27. MTD演習3 (課題) / Exercise 3 Assignment

**個人演習 / Individual (hands-on):**

1. `exercise3-combined/scenario.sh` を実行し、2層の効果を観察する。

**グループ演習 / Group (discuss & present):**

1. **平均攻撃成功時間間隔 (MTTC)**: 第1層のみ／第2層のみ／両方で、攻撃コストや MTTC は
   どう変化するか。層は「積」で効くのか「和」で効くのか。
2. 第1層はポート系列の秘密性に依存する。この秘密はどのように漏れうるか
   （タイミング・トラフィック解析・内部者）。漏れても第2層がある価値は。
3. 各層が正規利用者に課す負担（可用性・遅延・運用コスト）。多層化の費用対効果をどう取るか。
4. アプリ層・VM層のMTDを加えた、さらなる多層化の設計案を出せ（スライド10の階層表を参照）。

---

## 28. 議論: 新しいMTDを考えたい / Discussion: New MTD Ideas

**変更できるパラメータ (What?) の例:**

- SQL文の方言を切り替える（`SELECT`→独自語彙）ことで SQLインジェクションを防ぐ。
- `/bin/sh` を `/exec/hoge` のように改名する（多くのシェルコードが失敗）。
- アプリケーションを切り替える（例: Web = Apache / Nginx を切替）。
  - 利点: 攻撃手法が変わり脆弱性を突かれる状況が一時的になる。
  - 欠点: 開発・運用コスト増、バグ混入リスク増。
- コンピュータ内部の数値表現を切替（2進→5進→11進…、仮想計算機）。オーバーヘッドは大。
- エンディアンを切り替える。

**新しいMTD（ネットワーク編）の例:**

- 公開システムでサーバのFQDN/IPアドレスを短時間で変更し、攻撃者の偵察・攻撃コストを高める。
  - 変更パラメータ: 接続先IP（ゲートウェイ側でIPv6の広大なブロックを保持し、数分単位で有効/無効を切替）、
    サーバのFQDN（ホスト名をランダム化し AレコードのTTLを数分に短縮）。
  - 攻撃者に強いる労力: 名前解決・IP指定での probe が失敗し、偵察効率が著しく低下。
  - 正規利用者への配慮: TTL値の工夫など、導線（ログイン〜利用〜ログアウト）に沿ったアクセスを保証するチューニングが必要。

---

## 付録: 参考文献・関連研究 / Appendix: References

- G. J. Duck, X. Gao, A. Roychoudhury, *Binary Rewriting without Control Flow Recovery*, PLDI 2020.
- e9patch v1.0.1 — <https://github.com/GJDuck/e9patch>
- 関連プロジェクト / related: e9syscall（libc.so のsyscall横取り）, e9afl, RedFat。
- Wai Kyi らのMTD評価研究（A New Design for Evaluating MTD, 2018 / IoT向けMTDフレームワーク, 2019 ほか）。
