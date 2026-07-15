# MTD (Moving Target Defense)

*ムービング・ターゲット・ディフェンス演習 / Moving Target Defense Hands-on*

更新日 / Updated: 2026-07-15
本資料は 2025-07-27 版スライド (`MTD-prosecit-20250727.pdf`) をMarkdown化し、
演習2 (システムコールMTD) を **e9patch v1.0.1**（2026年6月リリース）向けに全面刷新したものです。

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

> 本演習では **Application層のポート番号 (演習1)** と **OS-host層のシステムコール番号 (演習2)** を扱う。
> This hands-on covers **port numbering at the Application layer (Ex.1)** and
> **system call numbering at the OS-host layer (Ex.2)**.

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
- ptrace 用の `SYS_PTRACE` / `seccomp:unconfined` は `docker-compose.yml` で付与済み。

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
| 実行環境 | EC2上の事前構築環境 | ローカルで `./setup.sh` により再現 |

---

## 20. システムコールMTDフレームワークの概要 / System Call MTD Framework Overview

```
                 per-run random KEY (環境変数 MTD_KEY で共有)
                          │
        ┌─────────────────┴───────────────────────────────────────┐
        │                                                          │
  ┌───────────────┐        ptrace          ┌──────────────────────────────┐
  │ mtd_tracer     │◀──────(監視)──────────▶│ 被監視プロセス / target        │
  │ (ptrace親)     │                         │  e9patchで各 syscall 命令に    │
  │                │                         │  syscall_mtd フックを挿入済み  │
  │ ・KEYを生成     │                         │                              │
  │ ・KEYをenvで渡す │  syscall entry で:     │  正規syscall: rax += KEY      │
  │ ・翻訳/検知     │   rax>=KEY → 実番号へ翻訳 │  （フックが番号をランダム化）  │
  └───────────────┘   rax<KEY  → 侵入検知!!  └──────────────────────────────┘
```

- **フック (`syscall_mtd.c`)**: e9patchで正規バイナリの各 `syscall` 命令の直前に挿入。
  syscall番号 (`rax`) に per-run のランダムなKEYを加算し「番号をシャッフル」する。
- **tracer (`mtd_tracer.c`)**: ptraceでKEY分を引いて実番号に戻す。
  KEYでランダム化されていない生のsyscall（＝注入シェルコード）を **侵入として検知**する。

---

## 21. ランダム化するシステムコールの仕組み / How Randomization Works

正規バイナリの各 `syscall` 命令には、e9tool が `syscall_mtd` フックを挿入する。
フックは実行時に `rax`（syscall番号）へ per-run のランダムKEYを加える。

*e9tool injects the `syscall_mtd` hook before every `syscall` instruction of the legitimate
binary. At run time the hook adds a per-run random KEY to `rax` (the syscall number).*

```c
// syscall_mtd.c （抜粋 / excerpt）  — e9patch v1.0.1 API
#include "stdlib.c"
static long key = 0;

void init(int argc, char **argv, char **envp) {
    environ = envp;
    const char *k = getenv("MTD_KEY");   // tracer が渡した per-run KEY
    key = (k != NULL ? atoll(k) : 0);
}

void entry(struct STATE *state) {        // 各 syscall 命令の直前 / before every syscall
    state->rax += key;                   // 番号をランダム化 / randomize the syscall number
}
```

`e9tool` コマンド（v1.0.1 構文）/ command (v1.0.1 syntax):

```console
$ E9PATCH/e9compile.sh syscall_mtd.c
$ E9PATCH/e9tool -M 'asm=/syscall/' -P 'before entry(state)@syscall_mtd' hello -o hello.mtd
```

---

## 22. とりあえず試してみよう！/ Let's Try It

```console
$ cd exercise2-syscall-mtd
$ ./setup.sh                       # e9patch v1.0.1 build（初回のみ / first time only）
$ make                             # tracer / hello / smashme をビルド
$ make hello.mtd                   # e9patchでhelloにsyscall MTDを適用

# 正規プログラムはMTD越しでも正しく動く / a legit program still works through MTD
$ ./mtd_tracer ./hello.mtd
[mtd] MTD_KEY = 0x5f3a1c00 (per-run random)
Hello, world
[mtd] 6 syscalls translated, 0 intrusions detected
```

正規バイナリの syscall は KEY で「ずれた」番号になるが、tracer が正しく実番号へ翻訳するため
プログラムは通常どおり動作する。
*Legitimate syscalls arrive "shifted" by KEY; the tracer translates them back, so the program runs normally.*

---

## 23. shellcode を試してみる / Trying a Shellcode

脆弱な `smashme`（`gets()` によるスタックオーバーフロー）に対し、
`jmp rsp` ガジェット経由でスタック上のシェルコードを実行させる古典的攻撃を行う。

*Against the vulnerable `smashme` (stack overflow via `gets()`), we run the classic
attack that jumps to stack shellcode via a `jmp rsp` gadget.*

```console
# MTDなし: シェルが奪われる / without MTD: shell is hijacked
$ python3 exploit.py --no-mtd
[*] Switching to interactive mode
$ id
uid=1000(user) ...          ← 攻撃成功 / attack succeeds

# MTDあり: 注入syscallが検知され防御される / with MTD: injected syscall is detected & blocked
$ python3 exploit.py --mtd
[mtd] Invalid system call detected (raw execve, rax=59): INTRUSION BLOCKED
[*] Stopped process './mtd_tracer' — shell denied
```

---

## 23b. なぜ検知できるのか / Why Detection Works

- 正規バイナリの syscall 命令は **書き換え時に既知**なので、e9tool がすべてフック済み。
  → フックが KEY を加算し、tracer は「KEYでずれた番号」を実番号へ翻訳できる。
- 注入シェルコードは **実行時にスタックへ現れる**ためフックされない。
  → syscall番号は生の実番号（例: `execve = 59`）のまま。
- tracer は「KEYでランダム化されていない生の番号」を検出し **侵入として遮断**する。

*Legitimate syscalls exist at rewrite time, so they are all hooked and KEY-shifted;
injected shellcode appears on the stack at run time, is never hooked, and keeps raw canonical
numbers — which the tracer flags as an intrusion.*

---

## 24. MTD演習2 (課題30分) / Exercise 2 Assignment (30 min)

**個人演習 / Individual (hands-on, 30 min):**

1. スライドと `exercise2-syscall-mtd/README.md` に従い、システムコールレベルMTDを試す。

**グループ演習 / Group (discuss & present, 30 min):**

1. Webサービスに外部からバイナリが送り込まれ実行される未知の脆弱性があると仮定する。
   システムコールレベルMTDが組み込まれていれば攻撃を検出・防御できるか。
2. 演習1のような別のMTD（URL等）と同時に組み合わせた場合、「平均攻撃成功時間間隔」はどう変化するか。
3. 本フレームワークは syscall 番号をランダム化する。これはどのような効果があるか。
   （syscall命令ごとに変える・時間ごとに変える等の拡張も議論せよ。）
4. **旧版との相違点**: 旧演習は Linuxカーネルのリコンフィグを伴っていた。
   本版（e9patch v1.0.1 + ptraceによるユーザ空間実装）との相違点・利点・欠点は何か。

---

## 25. 議論: 新しいMTDを考えたい / Discussion: New MTD Ideas

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
