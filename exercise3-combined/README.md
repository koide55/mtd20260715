# 演習3: 多層防御シナリオ（ネットワークMTD × システムコールMTD）
# Exercise 3: Defense in Depth (network MTD × syscall MTD)

演習1（ネットワークレベルMTD＝URL/ポート・シャッフリング）と演習2（システムコール
レベルMTD＝プロセス内syscallゲート監視）を**組み合わせた多層防御**を体験する。

*Combine Exercise 1 (network-level MTD: URL/port shuffling) and Exercise 2 (syscall-level
MTD: in-process syscall gate) into a layered, defense-in-depth scenario.*

---

## シナリオ / Scenario

Webサービスが「秘密のポート系列」を移動し続ける（**第1層**）。正規クライアントは
その系列を知っているので常に追従できるが、固定ポートに張り付く攻撃者は標的を見失う。

万一、攻撃者がいずれかのサーバでコード実行に到達しても、そのサーバプロセスは
**プロセス内のsyscallゲート（第2層）**の背後で動いているため、シェル起動の
`execve` はゲートで遮断される。

*A web service keeps hopping across a secret port sequence (Layer 1). Legitimate clients know
the sequence and follow it; an attacker fixated on one port loses the target. Even if the
attacker reaches code execution on a server, the process runs behind the in-process syscall
gate (Layer 2), so the shell-spawning execve is blocked.*

```
             ┌──────────────── Layer 1: network MTD ────────────────┐
  attacker ─▶│  service hops :8123 → :9001 → :9002 → ...            │
  (fixed     │  legit client follows the secret sequence (100%)     │
   port)     │  attacker on a fixed port hits only in a small window│
             └───────────────────────────┬──────────────────────────┘
                                          │ (rare) break-in / code exec
                                          ▼
             ┌──────────────── Layer 2: syscall MTD ────────────────┐
             │  payload tries execve("/bin/sh") at a syscall gate   │
             │  → [mtd] INTRUSION BLOCKED (exit 42)                  │
             └──────────────────────────────────────────────────────┘
```

---

## 実行 / Run

Docker（推奨）:

```console
$ docker compose exec lab ./exercise3-combined/scenario.sh
```

ローカル（x86_64 Linux, 事前に演習2の `./setup.sh` を実施）:

```console
$ ./scenario.sh
```

`scenario.sh` は次を行う：

1. 演習2の syscall-MTD バイナリ (`victim.mtd`) をビルド。
2. **第1層**: `mtdnet.py` を起動し、正規クライアントが秘密のポート系列を追従する一方、
   攻撃者は初期ポートに固定で probe。ヒット率を集計（正規 5/5 に対し攻撃者は 1/5 など）。
3. **第2層**: 攻撃者がコード実行に到達したと仮定し、`victim.mtd pwn`（シェル起動）を試行 →
   syscallゲートが `execve` を遮断（exit 42）。

期待される出力（抜粋）:

```
LAYER 1 -- Network-level MTD (URL / port shuffling)
  round 0: legit client -> :8123   OK
           attacker    -> :8123   HIT
  round 1: legit client -> :9001   OK
           attacker    -> :8123   miss
  ...
  legitimate client success: 5/5
  attacker (fixed port)     : 1/5  -> network MTD shrank the attack window

LAYER 2 -- Syscall-level MTD (in-process gate, last line of defense)
  [no MTD]      $ ./victim pwn        -> ### SHELL OBTAINED (uid=0)
  [syscall MTD] $ ./victim.mtd pwn    -> [mtd] INTRUSION BLOCKED ... execve (rax=59)
```

---

## グループ演習 / Group discussion

1. **平均攻撃成功時間間隔 (MTTC)**: 第1層だけ、第2層だけ、両方を組み合わせた場合で、
   攻撃者のコストや MTTC はそれぞれどう変化するか。層は「積」で効くのか「和」で効くのか。
2. 第1層はポート系列の秘密性に依存する。この秘密がどのように漏れうるか（タイミング、
   トラフィック解析、内部者）。漏れても第2層があることの価値は。
3. 正規利用者への負担（可用性・遅延・運用複雑性）は各層でどの程度増えるか。
   多層化のコストと便益のバランスをどう取るか。
4. 第1層（偵察の妨害）と第2層（ペイロードの無効化）は、攻撃キルチェーンの
   どの段階に効くか。他にどの段階を狙うMTDが考えられるか。
5. 本シナリオはネットワーク層とOS-host層を組み合わせた。アプリ層・VM層を加えた
   さらなる多層化の設計案を出せ（演習用スライドの「階層」表を参照）。

---

## 構成要素 / Components

| ファイル / File | 役割 / Role |
|---|---|
| `scenario.sh` | 演習1・2を組み合わせた多層防御シナリオのオーケストレーション |

> 依存: `../exercise1-network-mtd/mtdnet.py` と `../exercise2-syscall-mtd/`（`victim.mtd`）。

## 検証状況 / Verification status

- **第1層（ポートシャッフル）はサンドボックスで実行確認済み**（正規 5/5・攻撃者 1/5）。
- 第2層は演習2と同じ仕組み（Docker/Rosetta 実機で動作確認済み）。
  Docker上での通し実行は受講生環境で確認のこと。
